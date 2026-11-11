"""R-PY-06: unit tests for the coverage-merge math, using the spec 8.3 worked example."""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import coverage_merge as cm  # noqa: E402


def cp(bins, ignore=()):
    return {"type": "coverpoint", "at_least": 1, "bins": dict(bins), "ignore": list(ignore)}


def cross(n_hit, n_total, prefix="x"):
    return {"type": "cross", "at_least": 1, "bins": {f"{prefix}{k}": (1 if k < n_hit else 0) for k in range(n_total)}, "ignore": []}


def seed1():
    """cg_pkt after one seed, exactly the spec's worked example."""
    return {
        "test": "hotspot_test",
        "seed": 1234,
        "groups": {
            "cg_pkt": {
                "items": {
                    "len": cp({"1": 12, "2-7": 40, "8-15": 0, "16-30": 7, "31": 3}),
                    "src": cp({str(i): 5 for i in range(4)}),
                    "dst": cp({str(i): 5 for i in range(4)}),
                    "err": cp({"good": 60, "len0": 2, "len_over": 0}),
                    "src_x_dst": cross(14, 16),
                    "len_x_dst": cross(15, 20),
                }
            }
        },
    }


def test_item_percentages_match_worked_example():
    items = seed1()["groups"]["cg_pkt"]["items"]
    assert cm.item_percent(items["len"]) == pytest.approx(80.0)
    assert cm.item_percent(items["src"]) == pytest.approx(100.0)
    assert cm.item_percent(items["dst"]) == pytest.approx(100.0)
    assert cm.item_percent(items["err"]) == pytest.approx(66.667, abs=1e-3)
    assert cm.item_percent(items["src_x_dst"]) == pytest.approx(87.5)
    assert cm.item_percent(items["len_x_dst"]) == pytest.approx(75.0)


def test_group_is_average_of_items_84_9():
    g = seed1()["groups"]["cg_pkt"]
    assert round(cm.group_percent(g), 1) == 84.9


def test_merge_adds_hits_before_thresholding():
    # A second seed hits only len 8-15 and len_over: those bins become >= 1, so len and
    # err rise to 100% without re-running the first seed.
    s2 = {
        "groups": {
            "cg_pkt": {
                "items": {
                    "len": cp({"1": 0, "2-7": 0, "8-15": 4, "16-30": 0, "31": 0}),
                    "err": cp({"good": 0, "len0": 0, "len_over": 1}),
                }
            }
        }
    }
    merged = cm.merge_func([seed1(), s2])
    items = merged["groups"]["cg_pkt"]["items"]
    assert items["len"]["bins"]["8-15"] == 4
    assert items["len"]["bins"]["2-7"] == 40  # summed, not overwritten
    assert cm.item_percent(items["len"]) == 100.0
    assert cm.item_percent(items["err"]) == 100.0
    assert merged["runs"] == 2
    # src/dst/crosses unchanged: (100 + 100 + 100 + 100 + 87.5 + 75) / 6
    assert cm.group_percent(merged["groups"]["cg_pkt"]) == pytest.approx(93.75)


def test_ignored_bins_leave_numerator_and_denominator():
    item = cp({"a": 1, "b": 0, "c": 0}, ignore=["c"])
    assert cm.item_stats(item) == (1, 2)
    assert cm.item_percent(item) == 50.0


def test_at_least_threshold():
    item = cp({"a": 1, "b": 2})
    item["at_least"] = 2
    assert cm.item_percent(item) == 50.0


def test_total_is_average_of_groups():
    cov = {"groups": {"g1": {"items": {"i": cp({"a": 1})}}, "g2": {"items": {"i": cp({"a": 1, "b": 0})}}}}
    assert cm.total_percent(cov) == pytest.approx(75.0)


def test_holes_lists_uncovered_bins():
    hs = cm.holes(seed1())
    assert ("cg_pkt", "len", "8-15") in hs
    assert ("cg_pkt", "err", "len_over") in hs


def test_code_summary_with_waiver(tmp_path):
    md = tmp_path / "w.md"
    md.write_text("| type | module | point | reason |\n|---|---|---|---|\n| toggle | router_top | cnt\\[15\\] | never reached |\n")
    waivers = cm.load_waivers(md)
    pts = {
        "a": {"type": "line", "module": "m", "file": "rtl/m.sv", "line": 1, "comment": "block", "hier": "t", "count": 3},
        "b": {"type": "line", "module": "m", "file": "rtl/m.sv", "line": 2, "comment": "block", "hier": "t", "count": 0},
        "c": {"type": "toggle", "module": "router_top", "file": "rtl/t.sv", "line": 5, "comment": "cnt[15]", "hier": "t", "count": 0},
        "d": {"type": "toggle", "module": "router_top", "file": "rtl/t.sv", "line": 5, "comment": "cnt[0]", "hier": "t", "count": 9},
    }
    s = cm.summarize_code(pts, waivers)
    assert s["types"]["line"]["percent"] == 50.0
    assert s["types"]["toggle"]["percent"] == 100.0  # cnt[15] waived
    assert s["types"]["toggle"]["waived"] == 1
    assert s["types"]["expr"]["percent"] is None


def test_repo_waivers_pin_only_the_default_arm():
    """The real docs/coverage_waivers.md must waive the FSM default arm and nothing near it."""
    waivers = cm.load_waivers(cm.REPO / "docs" / "coverage_waivers.md")
    assert waivers, "waiver table did not parse"
    src = (cm.REPO / "rtl" / "in_port.sv").read_text().splitlines()
    lines = {txt.strip().split(":")[0]: i + 1 for i, txt in enumerate(src) if txt.strip().startswith(("IDLE:", "PASS:", "DROP:", "default:"))}
    mk = lambda ln: {"type": "line", "module": "in_port__Pz1", "file": "rtl/in_port.sv", "line": ln, "comment": "case", "hier": "t", "count": 0}
    assert cm.is_waived(mk(lines["default"]), waivers)
    for arm in ("IDLE", "PASS", "DROP"):
        assert not cm.is_waived(mk(lines[arm]), waivers), f"{arm} arm must not be waived"
