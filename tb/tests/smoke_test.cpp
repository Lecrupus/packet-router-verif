// smoke_test: 1 packet per input to each output in turn, out_ready ALWAYS.
// Proves the basic path, tagging and counters (R-DUT-04, R-DUT-13).
#include "tests/base_test.hpp"

class SmokeTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = N_PORTS * N_PORTS;
        cfg.set_bp_all(BpPattern::ALWAYS);
    }
    void start_sequences() override {
        for (int i = 0; i < N_PORTS; ++i) {
            std::vector<int> dsts;
            for (int k = 0; k < N_PORTS; ++k) dsts.push_back((i + k) % N_PORTS);
            env_->in_agent[i]->seqr->start(std::make_unique<DirectedSeq>(i, dsts, cfg_, &env_->in_agent[i]->flow_seq));
        }
    }
};
REGISTER_TEST(SmokeTest, "smoke_test");
