"""Unit tests for regress.py's pure helpers: failure signatures and seed derivation."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import regress  # noqa: E402


def test_signature_groups_same_bug_across_tests_and_keeps_ids():
    a = regress.signature("[      1234] ERROR random_test.env.sb: unexpected packet on out[2] tagged src 3")
    b = regress.signature("[        77] ERROR soak_test.env.sb: unexpected packet on out[0] tagged src 1")
    assert a == b == "ERROR env.sb: unexpected packet on out[N] tagged src N"
    s = regress.signature("[ 9] ERROR sva: A-05 TOP.router_top.g_out[1].u_arb.u_arb_sva.a05_ptr_rotates: x 0x1f")
    assert "A-05" in s and "a05_ptr_rotates" in s and "g_out[N]" in s and "x N" in s
    assert "R-DUT-02" in regress.signature("[1] ERROR t_test.env.checker: R-DUT-02 in_ready[3]=0")


def test_seed_derivation_is_reproducible_and_unique():
    s1 = regress.derive_seeds(20261111, "random_test", 200)
    s2 = regress.derive_seeds(20261111, "random_test", 200)
    assert s1 == s2 and len(set(s1)) == 200
    assert regress.derive_seeds(20261111, "hotspot_test", 5) != s1[:5]


def test_parse_seed_list():
    assert regress.parse_seed_list("1-3,7") == [1, 2, 3, 7]
