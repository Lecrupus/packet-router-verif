// counter_wrap_test (directed, added beyond the spec's 11): R-DUT-13 says every counter
// wraps at 16 bits, but no random test sends 65,536 packets through one port. This test
// sends 140,000 minimum-length packets per port, half of them bad-length, on a fixed
// permutation with out_ready ALWAYS, so pkt_in_cnt, pkt_out_cnt and drop_cnt all wrap
// and the scoreboard checks them mod 2^16.
#include "tests/base_test.hpp"

class CounterWrapTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 140000 * N_PORTS;
        cfg.err_rate = 0.5;
        cfg.perm = random_permutation();
        cfg.dst_mode = DstMode::PERMUTATION;
        cfg.set_bp_all(BpPattern::ALWAYS);
    }
    void start_sequences() override { start_on_all<MinLenSeq>(); }
};
REGISTER_TEST(CounterWrapTest, "counter_wrap_test");
