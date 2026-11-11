// bad_while_full_test: ErrorInjectSeq while the outputs are held (HOLD_THEN_RELEASE).
// Dropping a bad packet while the FIFO is full keeps in_ready = 1 (R-DUT-02).
//
// Good packets are 31 words of payload (32 words), so two of them fill a 64-word FIFO
// exactly at a packet boundary; the next header then arrives while the FIFO is full,
// and with a 30% error rate it is often a bad one that must be accepted and dropped.
#include "tests/base_test.hpp"

class BadWhileFullTest : public BaseTest {
public:
    using BaseTest::BaseTest;

protected:
    void configure(TrafficCfg& cfg) override {
        cfg.num_pkts = 400;
        cfg.err_rate = 0.30;
        cfg.len_dist = WeightedDist::constant(MAX_LEN);
        cfg.set_bp_all(BpPattern::HOLD_THEN_RELEASE);
    }
    void start_sequences() override { start_on_all<ErrorInjectSeq>(); }
};
REGISTER_TEST(BadWhileFullTest, "bad_while_full_test");
