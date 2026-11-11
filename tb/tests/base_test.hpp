// base_test.hpp -- BaseTest: every test reuses the same env and differs only in
// sequences, backpressure pattern and knobs (spec 9).
#pragma once
#include <array>
#include <string>

#include "base/registry.hpp"
#include "base/test.hpp"
#include "router/plusargs.hpp"
#include "router/router_env.hpp"
#include "router/sequences.hpp"
#include "router/traffic_cfg.hpp"

struct TestContext {
    Sim* sim;
    Rng* rng;
    const PlusArgs* args;
    std::string out_dir;
    uint64_t seed;
};

using RouterTestRegistry = TestRegistry<TestContext>;

#define REGISTER_TEST(cls, name) static TestRegistrar<TestContext, cls> registrar_##cls(name)

class BaseTest : public Test {
public:
    static constexpr uint64_t DRAIN_CYCLES = 10000;  // R-TB-08

    BaseTest(const std::string& name, TestContext& ctx) : Test(name), ctx_(ctx) {}

    void build() override;
    bool run_test() override;
    void write_outputs(const std::string& dir, bool passed, double wall_s) override;

protected:
    // Hooks for derived tests
    virtual void configure(TrafficCfg& cfg) { (void)cfg; }  // knobs before +arg overrides
    virtual void start_sequences();                         // default: RandomTrafficSeq per input
    virtual void setup_checks() {}                          // e.g. enable perf checks

    // Helpers
    std::array<int, N_PORTS> per_input(int total) const;
    std::array<int, N_PORTS> random_permutation();
    void random_bp(TrafficCfg& cfg, double lo = 0.3, double hi = 0.9);
    template <class SeqT>
    void start_on_all() {
        const auto n = per_input(cfg_.num_pkts);
        for (int i = 0; i < N_PORTS; ++i)
            env_->in_agent[i]->seqr->start(std::make_unique<SeqT>(i, n[i], cfg_, &env_->in_agent[i]->flow_seq));
    }

    TestContext& ctx_;
    RouterEnv* env_ = nullptr;
    TrafficCfg cfg_;
    uint64_t stim_cycles_ = 0, drain_cycles_ = 0;
};
