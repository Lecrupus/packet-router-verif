// min_len_test: MinLenSeq (1-word payloads, zero gaps), out_ready ALWAYS.
// Fast re-grant and header overhead.
#include "tests/base_test.hpp"

class MinLenTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        cfg.set_bp_all(BpPattern::ALWAYS);
    }
    void start_sequences() override { start_on_all<MinLenSeq>(); }
};
REGISTER_TEST(MinLenTest, "min_len_test");
