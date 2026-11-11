// router_env.hpp -- RouterEnv (uvm_env): 4 InAgent, 4 OutAgent, scoreboard (which owns
// the reference model), protocol checker, coverage; connects the analysis ports.
#pragma once
#include <array>

#include "base/env.hpp"
#include "router/agents.hpp"
#include "router/protocol_checker.hpp"
#include "router/router_coverage.hpp"
#include "router/scoreboard.hpp"

class RouterEnv : public Env {
public:
    RouterEnv(const std::string& name, Component* parent, Sim* sim, Rng* rng) : Env(name, parent), sim_(sim), rng_(rng) {}
    void build() override;
    void connect() override;

    std::array<InAgent*, N_PORTS> in_agent{};
    std::array<OutAgent*, N_PORTS> out_agent{};
    Scoreboard* sb = nullptr;
    ProtocolChecker* checker = nullptr;
    RouterCoverage* cov = nullptr;

    bool stimulus_done() const;  // all sequences finished and drivers idle
    void stop_sequences();

private:
    Sim* sim_;
    Rng* rng_;
};
