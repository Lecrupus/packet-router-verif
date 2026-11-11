// backpressure_test: RandomTrafficSeq, out_ready HOLD_THEN_RELEASE (0 for 500 cycles).
// Every FIFO full at once, then a clean drain (R-DUT-09).
#include "tests/base_test.hpp"

class BackpressureTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 1000;
        cfg.set_bp_all(BpPattern::HOLD_THEN_RELEASE);
    }
};
REGISTER_TEST(BackpressureTest, "backpressure_test");
