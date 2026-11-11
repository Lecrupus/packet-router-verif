// error_test: ErrorInjectSeq (10% bad len), out_ready RANDOM. Drop path and drop_cnt (R-DUT-02).
#include "tests/base_test.hpp"

class ErrorTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        cfg.err_rate = 0.10;
        random_bp(cfg);
    }
    void start_sequences() override { start_on_all<ErrorInjectSeq>(); }
};
REGISTER_TEST(ErrorTest, "error_test");
