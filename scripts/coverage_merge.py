#!/usr/bin/env python3
"""coverage_merge.py -- functional and code coverage math (spec 8.3).

Functional coverage, the same rules SystemVerilog tools use by default:
  1. a bin is covered when its hit count >= at_least (default 1)
  2. coverpoint / cross % = covered bins / (total bins - ignored bins)
  3. covergroup % = average of its coverpoints and crosses (weight 1 each)
  4. total functional % = average of the covergroups
  5. merging seeds: hit counts for the same bin are added across runs before step 1

Code coverage (Verilator coverage.dat):
  6. % per type = points with count >= 1 / total points, after removing waived points
"""
from __future__ import annotations

import json
import re
from functools import lru_cache
from pathlib import Path
from typing import Iterable

REPO = Path(__file__).resolve().parent.parent

# --------------------------------------------------------------------------------------
# Functional coverage
# --------------------------------------------------------------------------------------


def load_func(path: str | Path) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def merge_func(runs: Iterable[dict]) -> dict:
    """Rule 5: add hit counts bin by bin across runs. Ignore lists are unioned."""
    merged: dict = {"runs": 0, "groups": {}}
    for run in runs:
        merged["runs"] += 1
        for gname, group in run.get("groups", {}).items():
            mg = merged["groups"].setdefault(gname, {"items": {}})
            for iname, item in group.get("items", {}).items():
                mi = mg["items"].setdefault(
                    iname,
                    {
                        "type": item.get("type", "coverpoint"),
                        "at_least": item.get("at_least", 1),
                        "bins": {},
                        "ignore": [],
                    },
                )
                if "axes" in item:
                    mi["axes"] = item["axes"]
                for b, hits in item.get("bins", {}).items():
                    mi["bins"][b] = mi["bins"].get(b, 0) + int(hits)
                for b in item.get("ignore", []):
                    if b not in mi["ignore"]:
                        mi["ignore"].append(b)
    return merged


def item_stats(item: dict) -> tuple[int, int]:
    """Rules 1-2: (covered, total) for a coverpoint or cross; ignored bins excluded."""
    at_least = item.get("at_least", 1)
    ignore = set(item.get("ignore", []))
    bins = {b: h for b, h in item.get("bins", {}).items() if b not in ignore}
    covered = sum(1 for h in bins.values() if h >= at_least)
    return covered, len(bins)


def item_percent(item: dict) -> float:
    covered, total = item_stats(item)
    return 100.0 * covered / total if total else 100.0


def group_percent(group: dict) -> float:
    """Rule 3: average of the items, weight 1 each."""
    items = group.get("items", {})
    if not items:
        return 100.0
    return sum(item_percent(i) for i in items.values()) / len(items)


def total_percent(cov: dict) -> float:
    """Rule 4: average of the covergroups."""
    groups = cov.get("groups", {})
    if not groups:
        return 0.0
    return sum(group_percent(g) for g in groups.values()) / len(groups)


def holes(cov: dict) -> list[tuple[str, str, str]]:
    """Every uncovered bin as (group, item, bin)."""
    out = []
    for gname, g in cov.get("groups", {}).items():
        for iname, item in g.get("items", {}).items():
            ign = set(item.get("ignore", []))
            for b, h in item.get("bins", {}).items():
                if b not in ign and h < item.get("at_least", 1):
                    out.append((gname, iname, b))
    return out


def summarize_func(cov: dict) -> dict:
    groups = {}
    for gname, g in cov.get("groups", {}).items():
        items = {}
        for iname, item in g.get("items", {}).items():
            c, t = item_stats(item)
            items[iname] = {"covered": c, "total": t, "percent": item_percent(item)}
        groups[gname] = {"percent": group_percent(g), "items": items}
    return {"percent": total_percent(cov), "groups": groups, "holes": len(holes(cov))}


# --------------------------------------------------------------------------------------
# Code coverage (Verilator coverage.dat)
# --------------------------------------------------------------------------------------

_TYPE_OF_PAGE = {"v_line": "line", "v_branch": "line", "v_toggle": "toggle", "v_expr": "expr"}
CODE_TYPES = ("line", "toggle", "expr")


def parse_dat(path: str | Path) -> dict[str, dict]:
    """Return {point_key: point} for a Verilator coverage.dat. Each point has
    type, module, file, line, comment, hier, count."""
    points: dict[str, dict] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            if not raw.startswith("C '"):
                continue
            end = raw.rfind("' ")
            key = raw[3:end]
            count = int(raw[end + 2 :].strip() or 0)
            fields = {}
            for kv in key.split("\x01"):
                if "\x02" in kv:
                    k, v = kv.split("\x02", 1)
                    fields[k] = v
            page = fields.get("page", "")
            ptype = _TYPE_OF_PAGE.get(page.split("/", 1)[0])
            if ptype is None:
                continue
            if key in points:
                points[key]["count"] += count
                continue
            points[key] = {
                "type": ptype,
                "module": page.split("/", 1)[1] if "/" in page else "",
                "file": fields.get("f", ""),
                "line": int(fields.get("l", "0") or 0),
                "comment": fields.get("o", ""),
                "hier": fields.get("h", ""),
                "count": count,
            }
    return points


@lru_cache(maxsize=None)
def _source_lines(file: str) -> tuple[str, ...]:
    p = Path(file) if Path(file).is_absolute() else REPO / file
    try:
        return tuple(p.read_text(encoding="utf-8", errors="replace").splitlines())
    except OSError:
        return ()


def point_desc(pt: dict) -> str:
    """What a waiver regex is searched in: "<file>:<line> <comment> | <source line>".
    Including the source text lets a waiver pin one construct (e.g. a `default:` arm)
    without depending on line numbers, and without matching its neighbours."""
    lines = _source_lines(pt["file"])
    src = lines[pt["line"] - 1].strip() if 0 < pt["line"] <= len(lines) else ""
    return f"{Path(pt['file']).name}:{pt['line']} {pt['comment']} | {src}"


def load_waivers(md_path: str | Path) -> list[dict]:
    """Waivers live in docs/coverage_waivers.md as table rows:
       | <type> | <module regex> | <point regex> | <reason> |
    The point regex is searched in point_desc(): "<file>:<line> <comment> | <source line>"."""
    out: list[dict] = []
    p = Path(md_path)
    if not p.exists():
        return out
    in_table = False
    for line in p.read_text(encoding="utf-8").splitlines():
        if line.strip().lower().startswith("| type"):
            in_table = True
            continue
        if not in_table or not line.strip().startswith("|"):
            in_table = in_table and line.strip().startswith("|")
            continue
        # split on unescaped pipes; a regex cell may contain \| (a literal pipe)
        cells = [c.strip().strip("`") for c in re.split(r"(?<!\\)\|", line.strip()[1:-1])]
        if len(cells) < 4 or set(cells[0]) <= {"-", ":", " "}:
            continue
        out.append({"type": cells[0], "module": re.compile(cells[1]), "point": re.compile(cells[2]), "reason": cells[3]})
    return out


def is_waived(pt: dict, waivers: list[dict]) -> str | None:
    desc = point_desc(pt)
    for w in waivers:
        if w["type"] == pt["type"] and w["module"].fullmatch(pt["module"]) and w["point"].search(desc):
            return w["reason"]
    return None


def summarize_code(points: dict[str, dict], waivers: list[dict]) -> dict:
    """Rule 6, by type and by module."""
    by_type = {t: {"covered": 0, "total": 0, "waived": 0} for t in CODE_TYPES}
    by_module: dict[str, dict] = {}
    uncovered: list[dict] = []
    for pt in points.values():
        t = pt["type"]
        mod = by_module.setdefault(pt["module"], {x: {"covered": 0, "total": 0, "waived": 0} for x in CODE_TYPES})
        if is_waived(pt, waivers):
            by_type[t]["waived"] += 1
            mod[t]["waived"] += 1
            continue
        hit = pt["count"] >= 1
        for agg in (by_type[t], mod[t]):
            agg["total"] += 1
            agg["covered"] += hit
        if not hit:
            uncovered.append(pt)

    def pct(a: dict) -> float | None:
        return 100.0 * a["covered"] / a["total"] if a["total"] else None

    for a in by_type.values():
        a["percent"] = pct(a)
    for m in by_module.values():
        for a in m.values():
            a["percent"] = pct(a)
    uncovered.sort(key=lambda p: (p["type"], p["file"], p["line"], p["comment"]))
    return {"types": by_type, "modules": by_module, "uncovered": uncovered[:400], "uncovered_count": len(uncovered)}
