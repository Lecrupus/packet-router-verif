// random_test: RandomTrafficSeq, 2,000 packets, out_ready RANDOM.
// General correctness and per-flow ordering (R-DUT-10).
#include "tests/base_test.hpp"

class RandomTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        random_bp(cfg);
    }
};
REGISTER_TEST(RandomTest, "random_test");
