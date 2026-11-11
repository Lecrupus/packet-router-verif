#include "router/router_coverage.hpp"

#include <climits>

#include "base/logger.hpp"

RouterCoverage::RouterCoverage(const std::string& name, Component* parent, Sim* sim)
    : Subscriber<Packet>(name, parent), sim_(sim) {
    // ---- cg_pkt: each input packet. 5 + 4 + 4 + 3 + 16 + 20 bins ----
    len_ = &cg_pkt_.coverpoint("len").bin("1", 1).bin("2-7", 2, 7).bin("8-15", 8, 15).bin("16-30", 16, 30).bin("31", 31);
    src_ = &cg_pkt_.coverpoint("src").values(0, 3);
    dst_ = &cg_pkt_.coverpoint("dst").values(0, 3);
    err_ = &cg_pkt_.coverpoint("err").bin("good", 0).bin("len0", 1).bin("len_over", 2);
    src_x_dst_ = &cg_pkt_.cross("src_x_dst", *src_, *dst_);
    len_x_dst_ = &cg_pkt_.cross("len_x_dst", *len_, *dst_);

    // ---- cg_arb: each grant. requesters x output (16) + a->b per output (16 each) ----
    req_n_ = &cg_arb_.coverpoint("requesters").values(1, 4).set_weight(0);
    arb_out_ = &cg_arb_.coverpoint("output").values(0, 3).set_weight(0);
    prev_in_ = &cg_arb_.coverpoint("prev_input").values(0, 3).set_weight(0);
    cur_in_ = &cg_arb_.coverpoint("cur_input").values(0, 3).set_weight(0);
    req_x_out_ = &cg_arb_.cross("requesters_x_output", *req_n_, *arb_out_);
    for (int o = 0; o < N_PORTS; ++o) trans_[o] = &cg_arb_.cross("grant_seq_out" + std::to_string(o), *prev_in_, *cur_in_);

    // ---- cg_fifo: every cycle per input. level x port (16) + stall (4) + bad hdr while full (4) ----
    level_ = &cg_fifo_.coverpoint("level").bin("empty", 0).bin("1-31", 1, 31).bin("32-63", 32, 63).bin("full", 64).set_weight(0);
    fifo_port_ = &cg_fifo_.coverpoint("port").values(0, 3).set_weight(0);
    level_x_port_ = &cg_fifo_.cross("level_x_port", *level_, *fifo_port_);
    in_stall_ = &cg_fifo_.coverpoint("valid_not_ready").values(0, 3);
    bad_hdr_full_ = &cg_fifo_.coverpoint("bad_hdr_while_full").values(0, 3);

    // ---- cg_egress: each output stall end / packet. stall x port (20) + latency (5) ----
    stall_len_ = &cg_egress_.coverpoint("stall_len").bin("1", 1).bin("2-3", 2, 3).bin("4-15", 4, 15).bin("16-63", 16, 63).bin(">=64", 64, LLONG_MAX).set_weight(0);
    eg_port_ = &cg_egress_.coverpoint("port").values(0, 3).set_weight(0);
    stall_x_port_ = &cg_egress_.cross("stall_len_x_port", *stall_len_, *eg_port_);
    latency_ = &cg_egress_.coverpoint("latency").bin("<=3", 0, 3).bin("4-10", 4, 10).bin("11-50", 11, 50).bin("51-200", 51, 200).bin(">200", 201, LLONG_MAX);

    // ---- cg_timing: each input packet / cycle. 3 + 2 + 4 ----
    gap_ = &cg_timing_.coverpoint("gap").bin("0", 0).bin("1-3", 1, 3).bin(">=4", 4, LLONG_MAX);
    bubble_ = &cg_timing_.coverpoint("bubble").bin("no", 0).bin("yes", 1);
    same_cycle_eops_ = &cg_timing_.coverpoint("inputs_eop_same_cycle").values(1, 4);
}

void RouterCoverage::write(const Packet& p) {
    const int err = !p.bad_len ? 0 : (p.len == 0 ? 1 : 2);
    len_->sample(p.len);
    src_->sample(p.src);
    dst_->sample(p.dst);
    err_->sample(err);
    src_x_dst_->sample(p.src, p.dst);
    len_x_dst_->sample(p.len, p.dst);
    if (p.gap_seen >= 0) gap_->sample(p.gap_seen);
    bubble_->sample(p.bubble_seen ? 1 : 0);
}

void RouterCoverage::write_match(const MatchInfo& m) { latency_->sample(static_cast<int64_t>(m.latency)); }

void RouterCoverage::sample() {
    DutIf& d = sim_->dif();
    int eops = 0;
    for (int p = 0; p < N_PORTS; ++p) {
        const bool v = d.in_valid(p), r = d.in_ready(p);
        const int level = d.fifo_count(p);
        level_x_port_->sample(level, p);
        if (v && !r) in_stall_->sample(p);
        const bool is_hdr = v && (d.in_sop(p) || !in_pkt_[p]);
        if (is_hdr && r && level == FIFO_DEPTH && !Packet::len_legal(static_cast<int>(d.in_data(p) & 0xFF)))
            bad_hdr_full_->sample(p);
        if (v && r) {
            in_pkt_[p] = !d.in_eop(p);
            eops += d.in_eop(p);
        }
    }
    if (eops > 0) same_cycle_eops_->sample(eops);

    for (int o = 0; o < N_PORTS; ++o) {
        if (d.gnt_valid(o)) {
            const int g = d.gnt_idx(o);
            req_x_out_->sample(d.req_count(o), o);
            if (last_grant_[o] >= 0) trans_[o]->sample(last_grant_[o], g);
            last_grant_[o] = g;
        }
        if (d.out_valid(o) && !d.out_ready(o)) {
            ++stall_run_[o];
        } else if (stall_run_[o] > 0) {
            stall_x_port_->sample(stall_run_[o], o);
            stall_run_[o] = 0;
        }
    }
}

double RouterCoverage::percent() const {
    double s = 0;
    for (const auto* g : groups()) s += g->percent();
    return s / static_cast<double>(groups().size());
}

void RouterCoverage::report() {
    std::string line;
    for (const auto* g : groups()) line += cat(g->name(), "=", g->percent(), "% ");
    LOG_INFO(full_name(), "functional coverage ", percent(), "%  [", line, "]");
}

std::string RouterCoverage::to_json(const std::string& test, uint64_t seed) const {
    JsonWriter w;
    w.begin_obj();
    w.key("test").val(test);
    w.key("seed").val(seed);
    w.key("percent").val(percent());
    w.key("groups").begin_obj();
    for (const auto* g : groups()) {
        w.key(g->name());
        g->to_json(w);
    }
    w.end_obj();
    w.end_obj();
    return w.str();
}
