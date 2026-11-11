# Test plan

Every test reuses the same `RouterEnv`; tests differ only in sequences, the output
backpressure pattern and knobs. Seeds per test are for the full suite
(`scripts/suites.yaml`); the quick suite runs one seed of each of the first eleven.

## Tests

| Test | Sequences | out_ready | Proves | Coverage it contributes | Seeds |
|------|-----------|-----------|--------|-------------------------|------:|
| `smoke_test` | `DirectedSeq`: 1 packet per input to each output in turn | ALWAYS | Basic path, tagging, counters (R-DUT-04, 13) | `src_x_dst` diagonal-free walk, basic `cg_pkt` | 1 |
| `random_test` | `RandomTrafficSeq`, 2,000 packets | RANDOM (p 0.3–0.9 per output) | General correctness, per-flow ordering (R-DUT-10) | Bulk of `cg_pkt`, `cg_timing`, short stalls in `cg_egress` | 200 |
| `hotspot_test` | `HotspotSeq`, 80–100% to one output (random per seed) | RANDOM | Arbitration, fairness (R-DUT-07, P-05) | `cg_arb` 3–4 requesters, all grant transitions; FIFO full | 50 |
| `permutation_test` | `PermutationSeq`, random permutation, zero gaps | ALWAYS | Full throughput, ≤ 1 idle cycle between packets, header ≤ 3 cycles after eop (R-DUT-12, P-07 checks on) | `latency <= 3`, single-requester grants | 20 |
| `max_burst_test` | `MaxBurstSeq`: back-to-back 31-word packets | BURSTY (16–64-cycle stalls) | FIFO full, ingress backpressure, store-and-forward (R-DUT-05, 06) | `level full`, `valid_not_ready`, `stall 16-63` | 30 |
| `min_len_test` | `MinLenSeq`: 1-word payloads, zero gaps | ALWAYS | Fast re-grant, header overhead | `inputs_eop_same_cycle` 3–4, `len 1` | 20 |
| `error_test` | `ErrorInjectSeq`, 10% bad len | RANDOM | Drop path and `drop_cnt` (R-DUT-02) | `err len0 / len_over` | 50 |
| `backpressure_test` | `RandomTrafficSeq` | HOLD_THEN_RELEASE (0 for 500 cycles) | Every FIFO full at once, then a clean drain (R-DUT-09) | `stall >= 64`, `latency > 200` | 30 |
| `bad_while_full_test` | `ErrorInjectSeq`, 30% bad, good packets 31 words | HOLD_THEN_RELEASE | Dropping into a full FIFO keeps `in_ready` = 1 (R-DUT-02) | `bad_hdr_while_full` per port | 20 |
| `mixed_test` | `MixedVirtualSeq`: a random sequence per input | random pattern per output | Pattern interactions, coverage closure | Cross-pattern corners | 100 |
| `soak_test` | `RandomTrafficSeq`, 20,000 packets | RANDOM | Long runs, no leaks in expected queues | Counter bits up to 2^12 | 5 |
| `counter_wrap_test` *(directed, beyond the spec's 11)* | `MinLenSeq`, 140,000 packets per port, 50% bad, permutation | ALWAYS | All three counters wrap at 16 bits (R-DUT-13) | Toggle of counter bits 13–15 | 1 |

`smoke_test` exercises tagging on every input/output pair; `bad_while_full_test` uses
31-word packets because two of them fill the 64-word FIFO exactly at a packet boundary,
so the next header is guaranteed to arrive while the FIFO is full.

Every test ends with a second reset after the scoreboard checks: the design is reset
while dirty, then `Sim::reset` checks R-DUT-14 (FIFOs empty, counters 0, `in_ready` = 1).

## Requirement traceability (definition of done: all 14 have a check)

| Requirement | Checked by |
|-------------|------------|
| R-DUT-01 Handshake | Protocol checker P-01 (inputs and outputs); drivers hold data while `valid && !ready` |
| R-DUT-02 Header check / drop | Scoreboard (`expected_drops`, no bad packet may leave: P-03); checker "R-DUT-02 in_ready=0 while dropping"; `error_test`, `bad_while_full_test` |
| R-DUT-03 Framing assumption | Protocol checker "R-DUT-03" on inputs (eop exactly on payload word `len`) |
| R-DUT-04 Tagging | Scoreboard: reference model sets `src_tag`; field diff on header; `smoke_test` |
| R-DUT-05 Rate adaptation | Checker P-06 (`in_ready` = 0 only with 64 words, via the `dbg_fifo_count` debug signal); scoreboard (no word lost); A-01 |
| R-DUT-06 Store-and-forward | Checker P-07 "valid gap inside a packet" on every output in every test; A-01 (never read empty) |
| R-DUT-07 Arbitration | Checker P-05 (served before 4 other packets); A-02 one-hot grant; A-05 pointer = k+1 |
| R-DUT-08 No interleaving | Checker P-02/P-04 (no sop inside a packet on an output); A-03 grant held sop→eop |
| R-DUT-09 Egress backpressure | Checker P-01 on outputs; A-04 `$stable` while `out_valid && !out_ready` |
| R-DUT-10 Ordering | Scoreboard 4×4 `[src][dst]` queues, "out of order" error |
| R-DUT-11 Head-of-line blocking | Accepted limitation; visible in `hotspot_test` latency, not a failure |
| R-DUT-12 Performance | Checker P-07 in `permutation_test`: header ≤ 3 cycles after eop when idle, ≤ 1 idle cycle back-to-back, no gaps inside packets (all tests) |
| R-DUT-13 Counters | Scoreboard end-of-test comparison mod 2^16; `counter_wrap_test` wraps all three |
| R-DUT-14 Reset | `Sim::reset` checks after power-on reset and after an end-of-test reset of a dirty design; `--x-initial unique` + per-seed `+verilator+seed` randomize initial state |

## Checker validation (injected bugs)

`make bugs` (`scripts/inject_bugs.py`) builds the RTL with each `ifdef BUG_*` mutation
and runs the quick suite; every mutation must fail at least one test.

| Define | Bug | Expected to be caught by |
|--------|-----|--------------------------|
| `BUG_SRC_TAG` | wrong src tag | scoreboard (unexpected packet / header diff) |
| `BUG_ARB_NO_ROTATE` | priority pointer never rotates | A-05, checker P-05 |
| `BUG_DROP_STALL` | `in_ready` = 0 while dropping into a full FIFO | checker R-DUT-02 rule |
| `BUG_CUT_THROUGH` | packet eligible at header, not eop | A-01 (read empty), checker P-07, scoreboard |
| `BUG_NO_DROP_CNT` | `drop_cnt` never increments | scoreboard counter check |
| `BUG_SLOW_GRANT` | extra cycle between grant and header | checker P-07 (back-to-back gap) in `permutation_test` |

## Regression policy

- Quick suite before every commit (`make quick`, 11 runs).
- Full suite (`make full`) for sign-off: 100% pass, 100% functional, line ≥ 95%,
  toggle ≥ 90%, expression ≥ 85% after `docs/coverage_waivers.md`.
- R-TP-04: a bug found by a random seed gets its seed added to `fixed_seeds` in
  `scripts/suites.yaml` (or a directed test), so it is re-run forever.
