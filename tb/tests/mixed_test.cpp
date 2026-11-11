// mixed_test: MixedVirtualSeq (a random sequence per input), random backpressure
// pattern per output. Pattern interactions and coverage closure.
#include "tests/base_test.hpp"

class MixedTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 2000;
        cfg.err_rate = 0.05;
        cfg.hotspot_port = ctx_.rng->range(0, N_PORTS - 1);
        cfg.hotspot_pct = 0.8 + 0.2 * ctx_.rng->uniform();
        cfg.perm = random_permutation();
        for (auto& b : cfg.bp) {
            b.pattern = static_cast<BpPattern>(ctx_.rng->range(0, 3));
            b.p = 0.3 + 0.6 * ctx_.rng->uniform();
        }
    }
    void start_sequences() override {
        std::array<Sequencer<Packet>*, N_PORTS> seqrs{};
        std::array<uint16_t*, N_PORTS> flows{};
        for (int i = 0; i < N_PORTS; ++i) {
            seqrs[i] = env_->in_agent[i]->seqr;
            flows[i] = &env_->in_agent[i]->flow_seq;
        }
        MixedVirtualSeq::start(seqrs, flows, per_input(cfg_.num_pkts), cfg_, *ctx_.rng);
    }
};
REGISTER_TEST(MixedTest, "mixed_test");
