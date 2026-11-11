#include "tests/base_test.hpp"

#include <fstream>

#include "base/logger.hpp"

void BaseTest::build() {
    configure(cfg_);
    cfg_.apply_plusargs(*ctx_.args);
    LOG_INFO(name(), "seed=", ctx_.seed, " config: ", cfg_.to_string());  // R-STIM-03
    env_ = create<RouterEnv>("env", ctx_.sim, ctx_.rng);
}

std::array<int, N_PORTS> BaseTest::per_input(int total) const {
    std::array<int, N_PORTS> n{};
    for (int i = 0; i < N_PORTS; ++i) n[i] = total / N_PORTS + (i < total % N_PORTS ? 1 : 0);
    return n;
}

std::array<int, N_PORTS> BaseTest::random_permutation() {
    std::array<int, N_PORTS> p{0, 1, 2, 3};
    for (int i = N_PORTS - 1; i > 0; --i) std::swap(p[i], p[ctx_.rng->range(0, i)]);
    return p;
}

void BaseTest::random_bp(TrafficCfg& cfg, double lo, double hi) {
    for (auto& b : cfg.bp) {
        b.pattern = BpPattern::RANDOM;
        b.p = lo + (hi - lo) * ctx_.rng->uniform();
    }
}

void BaseTest::start_sequences() { start_on_all<RandomTrafficSeq>(); }

bool BaseTest::run_test() {
    Sim& sim = *ctx_.sim;
    build_phase();
    connect_phase();
    for (int o = 0; o < N_PORTS; ++o) env_->out_agent[o]->rsp->configure(cfg_.bp[o]);
    setup_checks();

    sim.reset(/*zero_cov=*/true);
    start_sequences();

    // run phase: until every sequence is finished and every driver is idle
    const uint64_t max_cycles = static_cast<uint64_t>(
        ctx_.args->num("MAX_CYCLES", 200000 + static_cast<long long>(cfg_.num_pkts) * 400));
    while (!env_->stimulus_done() && stim_cycles_ < max_cycles) {
        sim.cycle();
        run_cycle_phase();
        ++stim_cycles_;
    }
    if (!env_->stimulus_done()) LOG_ERROR(name(), "stimulus did not finish within ", max_cycles, " cycles");

    // R-TB-08: stop sequences, drain until every expected queue is empty or DRAIN_CYCLES pass
    env_->stop_sequences();
    while (!env_->sb->drained() && drain_cycles_ < DRAIN_CYCLES) {
        sim.cycle();
        run_cycle_phase();
        ++drain_cycles_;
    }
    for (int i = 0; i < 8; ++i) sim.cycle();  // let the last counters settle
    LOG_INFO(name(), "stimulus ", stim_cycles_, " cycles, drain ", drain_cycles_, " cycles");

    check_phase();
    report_phase();

    // R-DUT-14 with a dirty design: reset after traffic must empty every FIFO, clear every
    // counter and raise every in_ready (checked inside Sim::reset).
    env_->stop_sequences();
    sim.reset(/*zero_cov=*/false);
    return true;
}

void BaseTest::write_outputs(const std::string& dir, bool passed, double wall_s) {
    {
        std::ofstream f(dir + "/func_cov.json");
        f << env_->cov->to_json(name(), ctx_.seed) << "\n";
    }
    const Logger& L = Logger::get();
    JsonWriter w;
    w.begin_obj();
    w.key("test").val(name());
    w.key("seed").val(ctx_.seed);
    w.key("status").val(passed ? "PASS" : "FAIL");
    w.key("errors").val(L.errors());
    w.key("warnings").val(L.warnings());
    w.key("first_error").val(L.first_error());
    w.key("cycles").val(ctx_.sim->now());
    w.key("stim_cycles").val(stim_cycles_);
    w.key("drain_cycles").val(drain_cycles_);
    w.key("wall_time_s").val(wall_s);
    w.key("config").val(cfg_.to_string());
    w.key("func_cov").val(env_->cov->percent());
    w.key("protocol_checks").val(env_->checker->checks());
    w.key("scoreboard");
    env_->sb->stats().to_json(w, ctx_.sim->now());
    w.end_obj();
    std::ofstream f(dir + "/summary.json");
    f << w.str() << "\n";
}
