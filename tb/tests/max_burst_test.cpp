// max_burst_test: MaxBurstSeq (back-to-back 31-word packets, zero gaps), out_ready BURSTY.
// FIFO full, ingress backpressure, store-and-forward (R-DUT-05, R-DUT-06).
#include "tests/base_test.hpp"

class MaxBurstTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 1000;
        cfg.set_bp_all(BpPattern::BURSTY);
    }
    void start_sequences() override { start_on_all<MaxBurstSeq>(); }
};
REGISTER_TEST(MaxBurstTest, "max_burst_test");
