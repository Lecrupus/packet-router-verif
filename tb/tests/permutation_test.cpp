// permutation_test: PermutationSeq (each input to a different output, no collisions),
// zero gaps, out_ready ALWAYS. Full throughput and <= 1 idle cycle between packets
// (R-DUT-12), checked cycle-exactly by the protocol checker's P-07 rules.
#include "tests/base_test.hpp"

class PermutationTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        cfg.perm = random_permutation();
        cfg.gap_dist = WeightedDist::constant(0);
        cfg.bubble_rate = 0.0;
        cfg.set_bp_all(BpPattern::ALWAYS);
    }
    void start_sequences() override { start_on_all<PermutationSeq>(); }
    void setup_checks() override {
        for (int o = 0; o < N_PORTS; ++o) env_->checker->enable_perf_checks(o, true);
    }
};
REGISTER_TEST(PermutationTest, "permutation_test");
