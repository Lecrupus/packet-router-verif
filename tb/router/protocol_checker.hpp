// protocol_checker.hpp -- ProtocolChecker: cycle-level handshake and framing rules on
// every port, every cycle (spec 7.4, P-01..P-06), plus R-DUT-12 performance checks (P-07).
#pragma once
#include <array>
#include <cstdint>

#include "base/component.hpp"
#include "base/monitor.hpp"
#include "router/sim.hpp"

class ProtocolChecker : public Component, public Sampler {
public:
    ProtocolChecker(const std::string& name, Component* parent, Sim* sim) : Component(name, parent), sim_(sim) {}
    void sample() override;
    void check() override;
    // R-DUT-12 latency / re-grant checks are exact only without contention and with
    // out_ready = 1, so tests switch them on per output (permutation_test).
    void enable_perf_checks(int out_port, bool on) { perf_[out_port] = on; }
    uint64_t checks() const { return checks_; }

private:
    struct Stream {
        bool prev_stall = false;  // valid && !ready last cycle
        bool prev_sop = false, prev_eop = false;
        uint32_t prev_data = 0;
        bool in_pkt = false;
        int words = 0;
        int len = 0;
        int dst = 0;
        bool bad = false;
    };
    void check_stream(const char* dir, int p, Stream& s, bool valid, bool ready, bool sop, bool eop, uint32_t data,
                      bool is_input);

    Sim* sim_;
    std::array<Stream, N_PORTS> in_{}, out_{};
    // P-05: grants to other inputs on output o while input i was eligible for it
    std::array<std::array<int, N_PORTS>, N_PORTS> waited_{};
    // P-07 performance
    std::array<bool, N_PORTS> perf_{};
    std::array<int64_t, N_PORTS> last_out_eop_{-9, -9, -9, -9};       // last eop transfer per output
    std::array<int64_t, N_PORTS> pending_deadline_{-1, -1, -1, -1};  // header must leave by (-1 none)
    std::array<bool, N_PORTS> mid_pkt_out_{};
    uint64_t checks_ = 0;
};
