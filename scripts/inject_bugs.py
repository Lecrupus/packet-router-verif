#!/usr/bin/env python3
"""inject_bugs.py -- prove the checkers work: build the RTL with each deliberately
injected bug (`ifdef BUG_*` in rtl/), run the quick suite, and report which checker
caught it. Every bug must make at least one test fail.

  python3 scripts/inject_bugs.py            # all bugs
  python3 scripts/inject_bugs.py BUG_SRC_TAG
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

BUGS = {
    "BUG_SRC_TAG": "in_port tags the header with the wrong ingress port (R-DUT-04)",
    "BUG_ARB_NO_ROTATE": "rr_arbiter priority pointer never rotates (R-DUT-07)",
    "BUG_DROP_STALL": "in_ready drops to 0 while discarding a bad packet into a full FIFO (R-DUT-02)",
    "BUG_CUT_THROUGH": "packet eligible when its header is written, not its eop (R-DUT-06)",
    "BUG_NO_DROP_CNT": "drop_cnt never increments (R-DUT-13)",
    "BUG_SLOW_GRANT": "extra cycle between grant and header: 2 idle cycles between packets (R-DUT-12)",
}


def main() -> int:
    wanted = sys.argv[1:] or list(BUGS)
    rows, missed = [], []
    for bug in wanted:
        out = ROOT / "out" / bug.lower()
        cmd = [sys.executable, str(ROOT / "scripts" / "regress.py"), "--suite", "quick", "--define", bug,
               "--min-func", "0", "--min-line", "0", "--out", str(out)]
        print(f"== {bug}: {BUGS.get(bug, '')}", flush=True)
        subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        res = json.loads((out / "results.json").read_text())
        failed = [r for r in res["results"] if r["status"] != "PASS"]
        tests = sorted({r["test"] for r in failed})
        print(f"   {len(failed)}/{res['runs']} runs failed; tests: {', '.join(tests) or '-'}")
        for b in res["buckets"][:4]:
            print(f"   [{b['count']:>2}] {b['signature'][:150]}")
        rows.append({"bug": bug, "what": BUGS.get(bug, ""), "failed": len(failed), "runs": res["runs"],
                     "tests": tests, "first_signature": res["buckets"][0]["signature"] if res["buckets"] else ""})
        if not failed:
            missed.append(bug)
    (ROOT / "out" / "injected_bugs.json").write_text(json.dumps(rows, indent=1))
    print("\nMISSED:" if missed else "\nAll injected bugs were caught.", " ".join(missed))
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
