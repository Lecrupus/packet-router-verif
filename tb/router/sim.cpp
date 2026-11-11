#include "router/sim.hpp"

#include <verilated.h>
#include <verilated_cov.h>
#include <verilated_vcd_c.h>

#include "base/logger.hpp"

// DPI-C target of the SVA action blocks (rtl/sva/router_sva.sv): an assertion failure
// becomes a testbench ERROR carrying the assertion ID.
extern "C" void sva_fail(const char* msg) { LOG_ERROR("sva", msg); }

Sim::Sim(int argc, char** argv, bool trace, const std::string& out_dir) {
    ctx_ = std::make_unique<VerilatedContext>();
    ctx_->commandArgs(argc, argv);  // +verilator+seed+<n>, +verilator+rand+reset+2
    ctx_->fatalOnError(false);
    if (trace) ctx_->traceEverOn(true);
    dut_ = std::make_unique<Vrouter_top>(ctx_.get(), "TOP");
    dif_ = std::make_unique<DutIf>(dut_.get());
    if (trace) {
        tfp_ = std::make_unique<VerilatedVcdC>();
        dut_->trace(tfp_.get(), 99);
        tfp_->open((out_dir + "/waves.vcd").c_str());
    }
}

Sim::~Sim() = default;

void Sim::tick_half(bool clk) {
    dut_->clk = clk;
    dut_->eval();
    ctx_->timeInc(1);
    if (tfp_) tfp_->dump(ctx_->time());
}

void Sim::reset(bool zero_cov, int cycles) {
    // Quiet inputs, assert the asynchronous reset, clock a few cycles, release.
    for (int p = 0; p < N_PORTS; ++p) {
        dif_->set_in(p, false, false, false, 0);
        dif_->set_out_ready(p, false);
    }
    // With --x-initial unique every flop and input starts random, so make a real
    // falling edge on rst_n: the asynchronous reset must act before the first clock.
    dut_->clk = 0;
    dut_->rst_n = 1;
    dut_->eval();
    dut_->rst_n = 0;
    tick_half(false);
    for (int i = 0; i < cycles; ++i) {
        tick_half(true);
        tick_half(false);
    }
    dut_->rst_n = 1;
    // The DUT's reset synchronizer needs 2 edges; give it a few more.
    for (int i = 0; i < 4; ++i) {
        tick_half(true);
        tick_half(false);
    }
    // R-DUT-14: after reset every input is ready and every counter is 0
    for (int p = 0; p < N_PORTS; ++p) {
        if (!dif_->in_ready(p)) LOG_ERROR("sim", "R-DUT-14 in_ready[", p, "] is 0 after reset");
        if (dif_->pkt_in_cnt(p) || dif_->pkt_out_cnt(p) || dif_->drop_cnt(p))
            LOG_ERROR("sim", "R-DUT-14 counters for port ", p, " not 0 after reset");
        if (dif_->fifo_count(p) != 0) LOG_ERROR("sim", "R-DUT-14 FIFO ", p, " not empty after reset");
    }
#if VM_COVERAGE
    // At power-on every flop starts random (--x-initial unique) and reset drives it to 0,
    // which Verilator would count as a toggle. Zero the counters so code coverage measures
    // traffic; reset logic is covered by the end-of-test reset instead.
    if (zero_cov) ctx_->coveragep()->zero();
#else
    (void)zero_cov;
#endif
}

void Sim::cycle() {
    for (auto* d : drivers_) d->drive();      // set inputs for this cycle
    dut_->eval();                             // settle in_ready / out_valid
    for (auto* d : drivers_) d->post_eval();  // driver notes if its word was accepted
    for (auto* m : monitors_) m->sample();    // record valid && ready transfers at this edge
    tick_half(true);                          // rising edge: DUT moves data
    tick_half(false);
    ++cycle_;
}

void Sim::finish(const std::string& cov_path) {
    dut_->final();
    if (tfp_) {
        tfp_->close();
        tfp_.reset();
    }
#if VM_COVERAGE
    ctx_->coveragep()->write(cov_path.c_str());
#else
    (void)cov_path;
#endif
}
