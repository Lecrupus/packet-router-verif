// sim.hpp -- Sim: owns the Verilator model, advances the clock, and calls drivers then
// monitors in a fixed order each cycle (spec 5.3) so monitors see exactly the transfers
// the DUT sees.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/driver.hpp"
#include "base/monitor.hpp"
#include "router/dut_if.hpp"

class VerilatedContext;
class VerilatedVcdC;

class Sim {
public:
    Sim(int argc, char** argv, bool trace, const std::string& out_dir);
    ~Sim();

    DutIf& dif() { return *dif_; }
    Vrouter_top* dut() { return dut_.get(); }
    uint64_t now() const { return cycle_; }
    const uint64_t* cycle_ptr() const { return &cycle_; }

    void add_driver(Drivable* d) { drivers_.push_back(d); }
    void add_monitor(Sampler* m) { monitors_.push_back(m); }

    // Assert rst_n, release it, check R-DUT-14. zero_cov: clear code coverage afterwards
    // (used for the power-on reset, whose random-to-0 transitions are not real toggles).
    void reset(bool zero_cov, int cycles = 5);
    void cycle();
    // Flush the model, write coverage.dat (R-COV-02) and close the trace
    void finish(const std::string& cov_path);

private:
    void tick_half(bool clk);

    std::unique_ptr<VerilatedContext> ctx_;
    std::unique_ptr<Vrouter_top> dut_;
    std::unique_ptr<DutIf> dif_;
    std::unique_ptr<VerilatedVcdC> tfp_;
    std::vector<Drivable*> drivers_;   // non-owning, in registration order
    std::vector<Sampler*> monitors_;   // non-owning
    uint64_t cycle_ = 0;
};
