// soak_test: RandomTrafficSeq, 20,000 packets, out_ready RANDOM.
// Long runs: no leaks in expected queues, counters stay consistent.
#include "tests/base_test.hpp"

class SoakTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 20000;
        random_bp(cfg);
    }
};
REGISTER_TEST(SoakTest, "soak_test");
