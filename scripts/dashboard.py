#!/usr/bin/env python3
"""dashboard.py -- build out/<run>/dashboard/index.html from the files in a run folder.

  python3 scripts/dashboard.py out/<run>

One self-contained HTML file that opens offline (R-PY-02): charts are inline SVG drawn
here to exact scale, fonts are embedded (Ubuntu Sans, Ubuntu Font Licence), nothing is
fetched. Every number comes from a file in the run folder (R-PY-04): results.json,
func_cov_merged.json, code_cov.json, closure.json and jobs/*/summary.json; each panel
names its source as a numbered note.
"""
from __future__ import annotations

import base64
import json
import math
import sys
from html import escape
from pathlib import Path

import jinja2

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coverage_merge as cm  # noqa: E402

HERE = Path(__file__).resolve().parent
FONTS = HERE / "templates" / "fonts"
LAT_BINS = ["<=3", "4-10", "11-50", "51-200", ">200"]
SIGNOFF = {"func": 100.0, "line": 95.0, "toggle": 90.0, "expr": 85.0}


# ----------------------------------------------------------------------------- fonts


def font_faces() -> str:
    """@font-face rules with the subset fonts inlined as data URIs (no network)."""
    out = []
    for family, stem, weight in (("Sheet Sans", "UbuntuSans", "100 800"), ("Sheet Mono", "UbuntuSansMono", "400 800")):
        for ext, fmt in (("woff2", "woff2"), ("woff", "woff")):
            p = FONTS / f"{stem}.{ext}"
            if p.exists():
                data = base64.b64encode(p.read_bytes()).decode()
                stretch = "font-stretch: 75% 100%;" if stem == "UbuntuSans" else ""
                out.append(
                    f"@font-face{{font-family:'{family}';src:url(data:font/{fmt};base64,{data}) format('{fmt}');"
                    f"font-weight:{weight};{stretch}font-display:block;}}"
                )
                break
    return "\n".join(out)


# ----------------------------------------------------------------------------- charts (inline SVG)


def closure_points(curve: list[dict]) -> dict:
    """First run at which each metric reached its final value."""
    out = {}
    for key in ("func", "line"):
        vals = [c[key] for c in curve if c[key] is not None]
        if vals:
            final = vals[-1]
            out[key] = {"final": final, "run": next(c["run"] for c in curve if c[key] is not None and c[key] >= final)}
    return out


def closure_svg(curve: list[dict], targets: dict) -> str | None:
    """Cumulative functional and line coverage vs runs, log x axis, with leader-line callouts."""
    if not curve:
        return None
    W, H, L, R, T, B = 980, 300, 52, 24, 20, 40
    n = max(c["run"] for c in curve)
    lx = lambda run: L + (W - L - R) * (math.log10(run) / math.log10(max(n, 2)))
    ly = lambda pct: T + (H - T - B) * (1 - pct / 100.0)
    parts = [f'<svg class="chart" viewBox="0 0 {W} {H}" style="max-width:{W}px" role="img" aria-labelledby="closure-t">',
             '<title id="closure-t">Cumulative functional and line coverage versus number of runs, log scale</title>']
    for pct in (0, 25, 50, 75, 100):
        y = ly(pct)
        parts.append(f'<line class="grid" x1="{L}" x2="{W - R}" y1="{y:.1f}" y2="{y:.1f}"/>'
                     f'<text class="ax" x="{L - 8}" y="{y + 4:.1f}" text-anchor="end">{pct}</text>')
    tick = 1
    while tick <= n:
        x = lx(tick)
        parts.append(f'<line class="tick" x1="{x:.1f}" x2="{x:.1f}" y1="{H - B}" y2="{H - B + 5}"/>'
                     f'<text class="ax" x="{x:.1f}" y="{H - B + 18}" text-anchor="middle">{tick}</text>')
        tick *= 10
    parts.append(f'<text class="ax" x="{W - R}" y="{H - B + 18}" text-anchor="end">{n} runs</text>')
    for key, target, cls in (("func", targets.get("func"), "s-func"), ("line", targets.get("line"), "s-line")):
        pts = [(lx(c["run"]), ly(c[key])) for c in curve if c[key] is not None]
        if not pts:
            continue
        if target is not None:
            parts.append(f'<line class="limit {cls}" x1="{L}" x2="{W - R}" y1="{ly(target):.1f}" y2="{ly(target):.1f}"/>')
        d = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
        parts.append(f'<polyline class="series {cls}" points="{d}"/>')
    # Callouts in the empty lower band, leaders never crossing: the leftmost closure
    # point takes the lowest label row.
    cps = closure_points(curve)
    marks = sorted(((lx(cps[k]["run"]), ly(cps[k]["final"]), k) for k in cps), key=lambda m: m[0])
    label_x = L + 0.42 * (W - L - R)
    rows = [ly(28), ly(46)]
    for (x, y, key), ty in zip(marks, rows):
        cp = cps[key]
        name = "functional" if key == "func" else "line"
        parts.append(f'<circle class="mark" cx="{x:.1f}" cy="{y:.1f}" r="4"/>'
                     f'<line class="leader" x1="{x:.1f}" y1="{y + 4:.1f}" x2="{label_x - 6:.1f}" y2="{ty - 4:.1f}"/>'
                     f'<text class="callout" x="{label_x:.1f}" y="{ty:.1f}">{name} {cp["final"]:.1f}% at run {cp["run"]}</text>')
    parts.append("</svg>")
    return "".join(parts)


def latency_svg(hist: dict) -> str | None:
    total = sum(hist.values())
    if not total:
        return None
    W, H, L, B, T = 420, 240, 12, 34, 22
    mx = max(hist.values())
    bw = (W - 2 * L) / len(LAT_BINS)
    parts = [f'<svg class="chart" viewBox="0 0 {W} {H}" style="max-width:{W + 80}px" role="img" aria-labelledby="lat-t">',
             '<title id="lat-t">Packet latency histogram in cycles</title>',
             f'<line class="base" x1="{L}" x2="{W - L}" y1="{H - B}" y2="{H - B}"/>']
    for i, k in enumerate(LAT_BINS):
        v = hist.get(k, 0)
        h = (H - B - T) * v / mx
        x = L + i * bw + 10
        parts.append(f'<rect class="bar" x="{x:.1f}" y="{H - B - h:.1f}" width="{bw - 20:.1f}" height="{h:.1f}"/>'
                     f'<text class="val" x="{x + (bw - 20) / 2:.1f}" y="{H - B - h - 6:.1f}" text-anchor="middle">{v:,}</text>'
                     f'<text class="ax" x="{x + (bw - 20) / 2:.1f}" y="{H - B + 18}" text-anchor="middle">{escape(k)}</text>')
    parts.append("</svg>")
    return "".join(parts)


# ----------------------------------------------------------------------------- data shaping


# Five tint steps like drafting screens. The middle of the ramp is skipped on purpose:
# numbers sit in dark ink on the three light steps and in sheet colour on the two dark
# ones, so every cell keeps >= 4.5:1 contrast.
HEAT_LEVELS = (0.08, 0.20, 0.34, 0.80, 1.00)


def heat_level(frac: float) -> float:
    t = frac ** 0.35
    return HEAT_LEVELS[min(int(t * len(HEAT_LEVELS)), len(HEAT_LEVELS) - 1)]


def cross_grid(item: dict) -> dict:
    """Turn a cross's {"a,b": hits} bins into a row/column grid for the heat map."""
    rows: list[str] = []
    cols: list[str] = []
    for name in item["bins"]:
        a, b = name.split(",", 1)
        if a not in rows:
            rows.append(a)
        if b not in cols:
            cols.append(b)
    mx = max(item["bins"].values() or [1]) or 1
    grid = []
    for a in rows:
        cells = []
        for b in cols:
            h = item["bins"].get(f"{a},{b}")
            cells.append({"hits": h, "shade": 0 if not h else heat_level(h / mx)})
        grid.append({"label": a, "cells": cells})
    return {"cols": cols, "rows": grid, "axes": item.get("axes", ["", ""])}


def short(n: int | None) -> str:
    if n is None:
        return ""
    if n >= 1_000_000:
        return f"{n / 1e6:.1f}M"
    if n >= 10_000:
        return f"{n / 1e3:.0f}k"
    return f"{n:,}"


def build(run_dir: str | Path) -> Path:
    run_dir = Path(run_dir).resolve()
    res = json.loads((run_dir / "results.json").read_text())
    # Display against the spec's sign-off limits (section 8). A run may lower its own
    # exit gates (inject_bugs.py passes --min-func 0), but the sheet never does.
    targets = {k: max(v, SIGNOFF[k]) for k, v in {**SIGNOFF, **res.get("thresholds", {})}.items()}
    res["thresholds"] = targets

    func = json.loads((run_dir / "func_cov_merged.json").read_text()) if (run_dir / "func_cov_merged.json").exists() else {"groups": {}}
    code = json.loads((run_dir / "code_cov.json").read_text()) if (run_dir / "code_cov.json").exists() else {}
    curve = json.loads((run_dir / "closure.json").read_text()) if (run_dir / "closure.json").exists() else []

    # Performance numbers aggregated from each job's summary.json
    hist = {k: 0 for k in LAT_BINS}
    tput: dict[str, list[list[float]]] = {}
    lat_max, lat_sum, lat_n = 0, 0.0, 0
    for r in res["results"]:
        sp = run_dir / r["dir"] / "summary.json"
        if not sp.exists():
            continue
        s = json.loads(sp.read_text())
        sb = s.get("scoreboard", {})
        lat = sb.get("latency", {})
        for k, v in lat.get("hist", {}).items():
            hist[k] = hist.get(k, 0) + v
        lat_max = max(lat_max, lat.get("max", 0))
        lat_sum += lat.get("avg", 0) * lat.get("count", 0)
        lat_n += lat.get("count", 0)
        tput.setdefault(r["test"], []).append(sb.get("throughput", [0, 0, 0, 0]))
    tput_mean = {t: [sum(v[o] for v in vs) / len(vs) for o in range(4)] for t, vs in tput.items()}

    groups = []
    for gname, g in func.get("groups", {}).items():
        items = []
        for iname, item in g["items"].items():
            c, t = cm.item_stats(item)
            ign = set(item.get("ignore", []))
            entry = {"name": iname, "type": item.get("type"), "covered": c, "total": t, "percent": cm.item_percent(item),
                     "bins": [{"name": b, "hits": h, "short": short(h)} for b, h in item["bins"].items() if b not in ign]}
            if item.get("type") == "cross":
                entry["grid"] = cross_grid(item)
            items.append(entry)
        groups.append({"name": gname, "percent": cm.group_percent(g), "items": items})

    modules = [{"name": m, **{t: v[t] for t in cm.CODE_TYPES}} for m, v in sorted((code.get("modules") or {}).items())]
    closure = closure_points(curve)

    # Revision notes: everything that is not a clean pass is redlined on the sheet
    revisions = []
    failed = res["runs"] - res["passed"]
    if failed:
        revisions.append(f"{failed} of {res['runs']} runs failed in {len(res['buckets'])} bucket(s); repro commands in failed.txt.")
    if res["func"]["holes"]:
        revisions.append(f"{res['func']['holes']} functional coverage bin(s) never hit.")
    for t in ("line", "toggle", "expr"):
        a = res["code"].get(t) or {}
        if a.get("percent") is None:
            why = "this Verilator has no --coverage-expr" if t == "expr" and not res.get("expr_supported") else "no points"
            revisions.append(f"{'Expression' if t == 'expr' else t.capitalize()} coverage n/a: {why}.")
        elif a["percent"] < res["thresholds"][t] - 1e-9:
            revisions.append(f"{t.capitalize()} coverage {a['percent']:.2f}% is below the {res['thresholds'][t]}% target.")
    if not res.get("git_commit") or res["git_commit"] == "n/a":
        revisions.append("Commit not recorded: the project is not a git repository, so this run is not tied to a source revision.")
    waived = sum((res["code"].get(t) or {}).get("waived", 0) for t in cm.CODE_TYPES)
    if waived:
        revisions.append(f"{waived} code-coverage point(s) waived, each with a reason in docs/coverage_waivers.md.")

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(HERE / "templates"), autoescape=True)
    env.filters["short"] = short
    html = env.get_template("dashboard.html.j2").render(
        r=res,
        groups=groups,
        code=code,
        modules=modules,
        fonts=font_faces(),
        closure_svg=closure_svg(curve, {"func": res["thresholds"]["func"], "line": res["thresholds"]["line"]}),
        latency_svg=latency_svg(hist),
        lat={"max": lat_max, "avg": lat_sum / lat_n if lat_n else 0, "n": lat_n, "hist": hist},
        tput=tput_mean,
        closure=closure,
        curve_len=len(curve),
        revisions=revisions,
        has_genhtml=(run_dir / "code_cov_html" / "index.html").exists(),
        has_annotated=(run_dir / "annotated").exists(),
        run_folder=run_dir.name,
    )
    out = run_dir / "dashboard" / "index.html"
    out.parent.mkdir(exist_ok=True)
    out.write_text(html, encoding="utf-8")
    return out


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    print(build(sys.argv[1]))
