// hotspot_test: HotspotSeq (80-100% of packets to one output, target random per seed),
// out_ready RANDOM. Arbitration and fairness (R-DUT-07, P-05).
#include "tests/base_test.hpp"

class HotspotTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        cfg.hotspot_port = ctx_.rng->range(0, N_PORTS - 1);
        cfg.hotspot_pct = 0.8 + 0.2 * ctx_.rng->uniform();
        random_bp(cfg);
    }
    void start_sequences() override { start_on_all<HotspotSeq>(); }
};
REGISTER_TEST(HotspotTest, "hotspot_test");
