#!/usr/bin/env python3
"""regress.py -- one command: compile once, run every (test, seed) job in parallel,
classify, bucket failures, merge functional and code coverage, write the dashboard.

  python3 scripts/regress.py --suite quick                       # 11 runs, before every commit
  python3 scripts/regress.py --suite full --jobs 8 --timeout 300 # ~525 runs
  python3 scripts/regress.py --test hotspot_test --seeds 1-50    # one test, seed range
  python3 scripts/regress.py --rerun out/<run>/failed.txt        # replay only failures
  python3 scripts/regress.py --build-only                        # just compile

Exit code 0 only if every job passed and coverage meets --min-func / --min-line.
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import csv
import datetime as dt
import json
import os
import random
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))
import coverage_merge as cm  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RTL = [
    "rtl/router_pkg.sv",
    "rtl/sync_fifo.sv",
    "rtl/in_port.sv",
    "rtl/rr_arbiter.sv",
    "rtl/out_port.sv",
    "rtl/router_top.sv",
    "rtl/sva/router_sva.sv",
    "rtl/sva/bind_router.sv",
]
BINARY = "Vrouter_tb"
WAIVERS = ROOT / "docs" / "coverage_waivers.md"

# ----------------------------------------------------------------------------- build


def verilator_supports(flag: str) -> bool:
    try:
        out = subprocess.run(["verilator", "--help"], capture_output=True, text=True, timeout=60).stdout
    except (OSError, subprocess.TimeoutExpired):
        return False
    return flag in out


def build_cmd(obj_dir: Path, defines: list[str]) -> list[str]:
    tb_sources = sorted(str(p.relative_to(ROOT)) for p in (ROOT / "tb").rglob("*.cpp"))
    cov = ["--coverage-line", "--coverage-toggle"]
    if verilator_supports("--coverage-expr"):
        cov.append("--coverage-expr")
    cmd = [
        "verilator", "--cc", "--exe", "--build", "-j", "0",
        "--top-module", "router_top", "--assert", *cov,
        "--x-assign", "unique", "--x-initial", "unique", "--trace",
        "-Wno-fatal" if defines else "-Wall",
        # The bound SVA's `disable iff (!rst_n)` samples the async reset synchronously;
        # that is the only synchronous use, so this warning is about the checker, not the RTL.
        "-Wno-SYNCASYNCNET",
        "--Mdir", str(obj_dir),
        "-CFLAGS", f"-std=c++17 -O2 -I{ROOT / 'tb'}",
        "-o", BINARY,
    ]
    cmd += [f"+define+{d}" for d in defines]
    return cmd + RTL + tb_sources


def build(obj_dir: Path, defines: list[str]) -> Path:
    """Step 1. A failed compile stops the run with the log shown; no jobs start (R-PY-03)."""
    obj_dir.mkdir(parents=True, exist_ok=True)
    cmd = build_cmd(obj_dir, defines)
    log_path = obj_dir / "build.log"
    print(f"[build] {' '.join(cmd[:14])} ...  (log: {log_path.relative_to(ROOT)})", flush=True)
    t0 = time.time()
    with open(log_path, "w") as log:
        log.write(" ".join(cmd) + "\n\n")
        log.flush()
        rc = subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT).returncode
    if rc != 0:
        print(log_path.read_text(errors="replace")[-8000:])
        print(f"[build] FAILED (exit {rc}); no jobs started")
        sys.exit(3)
    print(f"[build] ok in {time.time() - t0:.1f}s", flush=True)
    return obj_dir / BINARY


# ----------------------------------------------------------------------------- jobs


def load_suites() -> dict:
    with open(ROOT / "scripts" / "suites.yaml") as f:
        return yaml.safe_load(f)


def derive_seeds(master: int, test: str, n: int) -> list[int]:
    """Seeds come from the master seed, so the job list itself reproduces (R-PY-05).
    random.Random(str) is deterministic across runs and platforms."""
    rng = random.Random(f"{master}:{test}")
    seeds: list[int] = []
    while len(seeds) < n:
        s = rng.randrange(1, 2**31)
        if s not in seeds:
            seeds.append(s)
    return seeds


def parse_seed_list(spec: str) -> list[int]:
    seeds: list[int] = []
    for part in spec.split(","):
        part = part.strip()
        if "-" in part:
            lo, hi = part.split("-", 1)
            seeds.extend(range(int(lo), int(hi) + 1))
        elif part:
            seeds.append(int(part))
    return seeds


def expand_jobs(args, cfg) -> list[tuple[str, int]]:
    """Step 2."""
    if args.rerun:
        jobs = []
        for line in Path(args.rerun).read_text().splitlines():
            t = re.search(r"\+TEST=(\S+)", line)
            s = re.search(r"\+SEED=(\d+)", line)
            if t and s:
                jobs.append((t.group(1), int(s.group(1))))
        return jobs
    if args.test:
        seeds = parse_seed_list(args.seeds) if args.seeds else derive_seeds(args.master_seed, args.test, 1)
        return [(args.test, s) for s in seeds]
    suite = cfg["suites"][args.suite]
    jobs = []
    for test, n in suite.items():
        jobs += [(test, s) for s in derive_seeds(args.master_seed, test, int(n))]
    for test, seeds in (cfg.get("fixed_seeds") or {}).items():
        if args.suite == "full" or test in suite:
            jobs += [(test, int(s)) for s in seeds if (test, int(s)) not in jobs]
    return jobs


def job_cmd(binary: Path, test: str, seed: int, out_dir: Path, extra: list[str]) -> list[str]:
    return [
        str(binary), f"+TEST={test}", f"+SEED={seed}", f"+verilator+seed+{seed}",
        "+verilator+rand+reset+2", f"+OUT={out_dir}", *extra,
    ]


ERR_RE = re.compile(r"\bERROR\b|%Error|\bFATAL\b")


def run_job(binary: Path, run_dir: Path, test: str, seed: int, timeout: int, extra: list[str]) -> dict:
    """Step 3 + 4: run one job in its own folder and classify it."""
    jdir = run_dir / "jobs" / f"{test}_{seed}"
    if jdir.exists():
        shutil.rmtree(jdir)
    jdir.mkdir(parents=True)
    cmd = job_cmd(binary, test, seed, jdir, extra)
    t0 = time.time()
    status, rc = "PASS", 0
    log_path = jdir / "sim.log"
    try:
        with open(log_path, "w") as log:
            rc = subprocess.run(cmd, cwd=jdir, stdout=log, stderr=subprocess.STDOUT, timeout=timeout).returncode
    except subprocess.TimeoutExpired:
        status, rc = "TIMEOUT", None
    wall = time.time() - t0
    text = log_path.read_text(errors="replace")
    first_error = next((ln.strip() for ln in text.splitlines() if ERR_RE.search(ln)), "")
    if status != "TIMEOUT":
        if rc is not None and rc < 0:
            status = "CRASH"  # killed by a signal
        elif rc == 0 and "TEST PASSED" in text and not first_error:
            status = "PASS"
        else:
            status = "FAIL"
    if status == "TIMEOUT":
        first_error = first_error or f"TIMEOUT after {timeout}s"
    elif status == "CRASH":
        first_error = first_error or f"CRASH: killed by signal {-rc}"
    summary = {}
    if (jdir / "summary.json").exists():
        try:
            summary = json.loads((jdir / "summary.json").read_text())
        except json.JSONDecodeError:
            pass
    return {
        "test": test,
        "seed": seed,
        "status": status,
        "exit_code": rc,
        "wall_s": round(wall, 3),
        "cycles": summary.get("cycles"),
        "func_cov": summary.get("func_cov"),
        "first_error": first_error,
        "dir": str(jdir.relative_to(run_dir)),
        "repro": " ".join(job_cmd(Path("./" + BINARY), test, seed, Path("."), extra)),
    }


# ----------------------------------------------------------------------------- analysis


def signature(line: str) -> str:
    """Step 5: normalize the first error line (numbers -> N) so identical failures group."""
    s = re.sub(r"^\[\s*\d+\]\s*", "", line)
    s = re.sub(r"\b[a-z0-9_]+_test\.", "", s)  # same bug in different tests -> same bucket
    # Keep check IDs (A-01, P-05, R-DUT-02, a05_ptr_rotates) intact; normalize other numbers
    ids = re.findall(r"\b(?:R-DUT-\d+|[AP]-\d+|a\d+_\w+)", s)
    s = re.sub(r"\b(?:R-DUT-\d+|[AP]-\d+|a\d+_\w+)", "\x00", s)
    s = re.sub(r"0x[0-9a-fA-F]+", "N", s)
    s = re.sub(r"\d+", "N", s)
    for i in ids:  # placeholders restored in order
        s = s.replace("\x00", i, 1)
    return re.sub(r"\s+", " ", s).strip()


def bucket_failures(results: list[dict]) -> list[dict]:
    buckets: dict[str, dict] = {}
    for r in results:
        if r["status"] == "PASS":
            continue
        sig = f"{r['status']}: {signature(r['first_error'])}"
        b = buckets.setdefault(sig, {"signature": sig, "count": 0, "tests": set(), "first": r})
        b["count"] += 1
        b["tests"].add(r["test"])
        if r["seed"] < b["first"]["seed"]:
            b["first"] = r
    out = []
    for b in sorted(buckets.values(), key=lambda x: -x["count"]):
        out.append({
            "signature": b["signature"], "count": b["count"], "tests": sorted(b["tests"]),
            "first_test": b["first"]["test"], "first_seed": b["first"]["seed"], "repro": b["first"]["repro"],
        })
    return out


def merge_code_coverage(run_dir: Path, dats: list[Path]) -> dict:
    """Step 6 (code): verilator_coverage merge, lcov export, annotation; per-type numbers
    are computed from merged.dat with waivers removed."""
    merged = run_dir / "merged.dat"
    if not dats:
        return {}
    # Merge in chunks to keep command lines short
    chunk, partials = 200, []
    for i in range(0, len(dats), chunk):
        part = run_dir / f".merged_part{i // chunk}.dat"
        subprocess.run(["verilator_coverage", "--write", str(part), *map(str, dats[i : i + chunk])], check=True,
                       capture_output=True)
        partials.append(part)
    subprocess.run(["verilator_coverage", "--write", str(merged), *map(str, partials)], check=True, capture_output=True)
    for p in partials:
        p.unlink()
    subprocess.run(["verilator_coverage", "--write-info", str(run_dir / "merged.info"), str(merged)],
                   capture_output=True)
    ann = run_dir / "annotated"
    subprocess.run(["verilator_coverage", "--annotate", str(ann), "--annotate-min", "1", str(merged)],
                   cwd=ROOT, capture_output=True)
    if shutil.which("genhtml"):
        # Relative output paths and --prefix keep the report free of local absolute paths,
        # so it can be published (GitHub Pages) as is.
        shutil.rmtree(run_dir / "code_cov_html", ignore_errors=True)
        rel = run_dir.relative_to(ROOT) if run_dir.is_relative_to(ROOT) else run_dir
        subprocess.run(["genhtml", "-q", "--ignore-errors", "source,unmapped,category", "--prefix", str(ROOT),
                        "-o", str(rel / "code_cov_html"), str(rel / "merged.info")],
                       cwd=ROOT, capture_output=True)
        (run_dir / "code_cov_html" / "cmd_line").unlink(missing_ok=True)
        for page in (run_dir / "code_cov_html").rglob("*.html"):  # genhtml tooltips carry abs dirs
            text = page.read_text(errors="replace")
            if str(ROOT) in text:
                page.write_text(text.replace(str(ROOT) + "/", "").replace(str(ROOT), "."))
    summary = cm.summarize_code(cm.parse_dat(merged), cm.load_waivers(WAIVERS))
    summary["expr_supported"] = verilator_supports("--coverage-expr")
    return summary


def closure_curve(results: list[dict], run_dir: Path) -> list[dict]:
    """Step 7: cumulative functional and line coverage after each run, in seed order."""
    ordered = sorted(results, key=lambda r: (r["seed"], r["test"]))
    merged: dict = {"groups": {}}
    line_hit: set[str] = set()
    line_all: set[str] = set()
    waivers = cm.load_waivers(WAIVERS)
    curve = []
    for k, r in enumerate(ordered, 1):
        jdir = run_dir / r["dir"]
        fc = jdir / "func_cov.json"
        if fc.exists():
            merged = cm.merge_func([merged, cm.load_func(fc)])
        dat = jdir / "coverage.dat"
        if dat.exists():
            for key, pt in cm.parse_dat(dat).items():
                if pt["type"] != "line" or cm.is_waived(pt, waivers):
                    continue
                line_all.add(key)
                if pt["count"] > 0:
                    line_hit.add(key)
        curve.append({
            "run": k, "test": r["test"], "seed": r["seed"],
            "func": round(cm.total_percent(merged), 3),
            "line": round(100.0 * len(line_hit) / len(line_all), 3) if line_all else None,
        })
    return curve


def git_commit() -> str:
    try:
        return subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, capture_output=True, text=True,
                              timeout=10).stdout.strip() or "n/a"
    except (OSError, subprocess.TimeoutExpired):
        return "n/a"


# ----------------------------------------------------------------------------- main


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--suite", default="quick")
    ap.add_argument("--test")
    ap.add_argument("--seeds", help="e.g. 1-50 or 3,7,9")
    ap.add_argument("--rerun", help="failed.txt from a previous run")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--master-seed", type=int)
    ap.add_argument("--min-func", type=float, default=100.0)
    ap.add_argument("--min-line", type=float, default=95.0)
    ap.add_argument("--min-toggle", type=float, default=90.0)
    ap.add_argument("--min-expr", type=float, default=85.0)
    ap.add_argument("--define", action="append", default=[], help="RTL define, e.g. BUG_SRC_TAG")
    ap.add_argument("--out", help="run folder (default out/<suite>_<timestamp>)")
    ap.add_argument("--build-only", action="store_true")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--plusarg", action="append", default=[], help="extra +ARG for every job")
    args = ap.parse_args()

    cfg = load_suites()
    if args.master_seed is None:
        args.master_seed = int(cfg.get("master_seed", 1))

    obj_dir = ROOT / ("obj_dir" if not args.define else "obj_dir_" + "_".join(args.define).lower())
    binary = obj_dir / BINARY
    if not args.no_build or not binary.exists():
        binary = build(obj_dir, args.define)
    if args.build_only:
        return 0

    jobs = expand_jobs(args, cfg)
    label = "rerun" if args.rerun else (args.test or args.suite)
    run_dir = Path(args.out) if args.out else ROOT / "out" / f"{label}_{dt.datetime.now():%Y%m%d_%H%M%S}"
    run_dir = run_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    print(f"[run] {len(jobs)} jobs, {args.jobs} workers -> {run_dir.relative_to(ROOT) if run_dir.is_relative_to(ROOT) else run_dir}", flush=True)
    (run_dir / "jobs.json").write_text(json.dumps({"master_seed": args.master_seed, "jobs": jobs}, indent=1))

    t0 = time.time()
    results: list[dict] = []
    with cf.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        futs = {ex.submit(run_job, binary, run_dir, t, s, args.timeout, args.plusarg): (t, s) for t, s in jobs}
        for k, fut in enumerate(cf.as_completed(futs), 1):
            r = fut.result()
            results.append(r)
            mark = "." if r["status"] == "PASS" else r["status"][0]
            print(mark, end="" if k % 80 else "\n", flush=True)
    wall = time.time() - t0
    print(f"\n[run] done in {wall:.1f}s", flush=True)
    order = {j: i for i, j in enumerate(jobs)}
    results.sort(key=lambda r: order[(r["test"], r["seed"])])

    # ---- analysis ----
    buckets = bucket_failures(results)
    func_files = [run_dir / r["dir"] / "func_cov.json" for r in results if (run_dir / r["dir"] / "func_cov.json").exists()]
    func_merged = cm.merge_func(cm.load_func(p) for p in func_files)
    func_summary = cm.summarize_func(func_merged)
    dats = [run_dir / r["dir"] / "coverage.dat" for r in results if (run_dir / r["dir"] / "coverage.dat").exists()]
    print("[cov] merging code coverage ...", flush=True)
    code = merge_code_coverage(run_dir, dats)
    curve = closure_curve(results, run_dir)

    per_test: dict[str, dict] = {}
    for r in results:
        t = per_test.setdefault(r["test"], {"runs": 0, "PASS": 0, "FAIL": 0, "TIMEOUT": 0, "CRASH": 0, "failing": []})
        t["runs"] += 1
        t[r["status"]] += 1
        if r["status"] != "PASS":
            t["failing"].append({"seed": r["seed"], "repro": r["repro"]})

    n_pass = sum(r["status"] == "PASS" for r in results)
    types = code.get("types", {})

    def pct(t: str):
        return (types.get(t) or {}).get("percent")

    gates = {
        "all_pass": n_pass == len(results) and len(results) > 0,
        "func": func_summary["percent"] >= args.min_func - 1e-9,
        "line": pct("line") is not None and pct("line") >= args.min_line - 1e-9,
    }
    overall_ok = all(gates.values())

    # ---- step 8: outputs ----
    with open(run_dir / "results.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["test", "seed", "status", "exit_code", "wall_s", "cycles", "func_cov", "first_error", "dir", "repro"])
        w.writeheader()
        w.writerows(results)
    (run_dir / "failed.txt").write_text("".join(f"{r['repro']}\n" for r in results if r["status"] != "PASS"))
    (run_dir / "func_cov_merged.json").write_text(json.dumps(func_merged, indent=1))
    (run_dir / "code_cov.json").write_text(json.dumps(code, indent=1))
    (run_dir / "closure.json").write_text(json.dumps(curve))
    report = {
        "label": label,
        "suite": args.suite if not (args.test or args.rerun) else None,
        "master_seed": args.master_seed,
        "defines": args.define,
        "git_commit": git_commit(),
        "date": dt.datetime.now().isoformat(timespec="seconds"),
        "wall_time_s": round(wall, 1),
        "jobs": args.jobs,
        "runs": len(results),
        "passed": n_pass,
        "pass_rate": round(100.0 * n_pass / len(results), 2) if results else 0.0,
        "per_test": per_test,
        "buckets": buckets,
        "func": func_summary,
        "code": {t: types.get(t) for t in cm.CODE_TYPES},
        "expr_supported": code.get("expr_supported"),
        "thresholds": {"func": args.min_func, "line": args.min_line, "toggle": args.min_toggle, "expr": args.min_expr},
        "gates": gates,
        "ok": overall_ok,
        "results": results,
    }
    (run_dir / "results.json").write_text(json.dumps(report, indent=1))

    import dashboard  # local module

    dash = dashboard.build(run_dir)
    latest = ROOT / "out" / "latest"
    try:
        if latest.is_symlink() or latest.exists():
            latest.unlink()
        latest.symlink_to(run_dir, target_is_directory=True)
    except OSError:
        pass

    # ---- console summary ----
    print(f"\nRuns {len(results)}  pass {n_pass}  ({report['pass_rate']}%)   wall {wall:.1f}s")
    print(f"Functional {func_summary['percent']:.2f}%  " + "  ".join(
        f"{g} {v['percent']:.1f}%" for g, v in func_summary["groups"].items()))
    print("Code       " + "  ".join(
        f"{t} {pct(t):.2f}%" if pct(t) is not None else f"{t} n/a" for t in cm.CODE_TYPES))
    for b in buckets:
        print(f"  [{b['count']:>3}] {b['signature']}\n        repro: {b['repro']}")
    print(f"Dashboard  {dash}")
    print("RESULT", "PASS" if overall_ok else "FAIL", {k: v for k, v in gates.items()})
    return 0 if overall_ok else 1


if __name__ == "__main__":
    sys.exit(main())
