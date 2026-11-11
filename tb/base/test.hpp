// test.hpp -- Test (uvm_test): the root of the component tree. Picks configuration,
// sequences and backpressure, then runs the phases.
#pragma once
#include "base/component.hpp"

class Test : public Component {
public:
    explicit Test(const std::string& name) : Component(name, nullptr) {}
    // Runs build -> connect -> run -> check -> report. Returns true when the run phase
    // completed (errors are counted by the Logger).
    virtual bool run_test() = 0;
    // Write per-run result files (summary.json, func_cov.json) into dir.
    virtual void write_outputs(const std::string& dir, bool passed, double wall_s) = 0;
};
