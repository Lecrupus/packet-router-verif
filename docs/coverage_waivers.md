# Coverage waivers

Every code-coverage hole that cannot be hit is listed here with a reason (R-COV-04).
`scripts/coverage_merge.py` reads the table below, so the dashboard and the exit-code
gates exclude exactly these points and nothing else. Each waived point is still counted
in the dashboard's "waived" figure, so nothing disappears silently.

How a row matches: `type` is `line`, `toggle` or `expr`; `module` is a regex that must
match the whole Verilator module name (parameterized instances get suffixes such as
`in_port__Pz1`); `point` is a regex searched in `<file>:<line> <comment> | <source line>`,
e.g. `in_port.sv:71 case | default: state <= IDLE;`. Matching on the source text pins a
waiver to one construct without depending on line numbers, and keeps it from swallowing
neighbouring points (an earlier version matched every `case` arm; the live arms are now
counted, only the `default:` arm is waived).

| type | module | point | reason |
|------|--------|-------|--------|
| line | `in_port(__Pz\d+)?` | `in_port\.sv:\d+ case \| default:` | `default:` arm of the parser FSM `unique case`. `state` is a 2-bit enum with three legal values (IDLE/PASS/DROP), so the fourth encoding is unreachable from reset; the arm exists so synthesis has a safe recovery state. Assertion A-06 and the parser's reset make the state observable. |

## Holes considered and not waived

- **Statistics counter high bits** (`pkt_in_cnt`, `pkt_out_cnt`, `drop_cnt` bits 6–15):
  random tests never send 65,536 packets through one port, so instead of waiving them,
  the directed `counter_wrap_test` wraps all three counters on every port.
- **Reset branches**: coverage is zeroed after the power-on reset (whose random-to-0
  transitions are not real toggles). Each test then resets the dirty design at the end
  and re-checks R-DUT-14, which covers the reset branches and reset-net toggles honestly.
- **FIFO storage array**: Verilator does not toggle-instrument the unpacked `mem` array
  inside `sync_fifo` (its 93 toggle points are the ports, pointers and count), so the
  100% toggle figure does not include the RAM bits. Their contents are checked end to
  end instead: every payload word is random 32-bit data and the scoreboard compares
  every word of every packet.
- **Expression coverage**: Verilator 5.032 (the version installed from Ubuntu 26.04) has
  no `--coverage-expr`. The build enables it automatically on a Verilator that supports
  it; until then the dashboard reports expression coverage as "n/a", not as a pass.
