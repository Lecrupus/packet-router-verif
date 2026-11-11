// router_coverage.hpp -- RouterCoverage (uvm_subscriber + covergroups), spec 8.2.
// Packet-level groups sample from the input monitors (write) and the scoreboard's
// matched port (latency); cycle-level groups sample every cycle (Sampler).
#pragma once
#include <array>
#include <string>

#include "base/analysis_port.hpp"
#include "base/monitor.hpp"
#include "coverage/covergroup.hpp"
#include "router/packet.hpp"
#include "router/scoreboard.hpp"
#include "router/sim.hpp"

class RouterCoverage : public Subscriber<Packet>, public Sampler {
public:
    RouterCoverage(const std::string& name, Component* parent, Sim* sim);

    void write(const Packet& p) override;    // input packets (cg_pkt, cg_timing)
    void write_match(const MatchInfo& m);    // scoreboard matches (cg_egress latency)
    void sample() override;                  // every cycle (cg_fifo, cg_arb, cg_egress, cg_timing)
    void report() override;

    double percent() const;
    std::string to_json(const std::string& test, uint64_t seed) const;
    const std::array<const Covergroup*, 5> groups() const {
        return {&cg_pkt_, &cg_arb_, &cg_fifo_, &cg_egress_, &cg_timing_};
    }

private:
    Sim* sim_;
    Covergroup cg_pkt_{"cg_pkt"}, cg_arb_{"cg_arb"}, cg_fifo_{"cg_fifo"}, cg_egress_{"cg_egress"},
        cg_timing_{"cg_timing"};

    // cg_pkt
    Coverpoint *len_, *src_, *dst_, *err_;
    Cross *src_x_dst_, *len_x_dst_;
    // cg_arb
    Coverpoint *req_n_, *arb_out_, *prev_in_, *cur_in_;
    Cross* req_x_out_;
    std::array<Cross*, N_PORTS> trans_{};
    std::array<int, N_PORTS> last_grant_{-1, -1, -1, -1};
    // cg_fifo
    Coverpoint *level_, *fifo_port_, *in_stall_, *bad_hdr_full_;
    Cross* level_x_port_;
    std::array<bool, N_PORTS> in_pkt_{};  // header tracking for "bad header while full"
    // cg_egress
    Coverpoint *stall_len_, *eg_port_, *latency_;
    Cross* stall_x_port_;
    std::array<int64_t, N_PORTS> stall_run_{};
    // cg_timing
    Coverpoint *gap_, *bubble_, *same_cycle_eops_;
};
