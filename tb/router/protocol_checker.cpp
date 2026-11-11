#include "router/protocol_checker.hpp"

#include "base/logger.hpp"
#include "router/packet.hpp"

void ProtocolChecker::check_stream(const char* dir, int p, Stream& s, bool valid, bool ready, bool sop, bool eop,
                                   uint32_t data, bool is_input) {
    ++checks_;
    // P-01: valid && !ready -> next cycle valid = 1 and data/sop/eop unchanged
    if (s.prev_stall) {
        if (!valid) LOG_ERROR(full_name(), "P-01 ", dir, "[", p, "] valid dropped while stalled (valid && !ready)");
        else if (data != s.prev_data || sop != s.prev_sop || eop != s.prev_eop)
            LOG_ERROR(full_name(), "P-01 ", dir, "[", p, "] data/sop/eop changed while stalled (valid && !ready)");
    }
    s.prev_stall = valid && !ready;
    s.prev_data = data;
    s.prev_sop = sop;
    s.prev_eop = eop;

    if (!(valid && ready)) return;
    // P-02 / P-04: sop only on the first word, never inside a packet; eop closes it
    if (sop) {
        if (s.in_pkt) LOG_ERROR(full_name(), "P-02 ", dir, "[", p, "] sop inside a packet (interleaving)");
        s.in_pkt = true;
        s.words = 0;
        s.len = static_cast<int>(data & 0xFF);
        s.dst = static_cast<int>((data >> 8) & 3);
        s.bad = !Packet::len_legal(s.len);
    } else if (!s.in_pkt) {
        LOG_ERROR(full_name(), "P-02 ", dir, "[", p, "] word without sop");
        s.in_pkt = true;
        s.words = 0;
        s.len = -1;
        s.bad = true;
    }
    ++s.words;
    if (eop) {
        // P-03: packet length = header len + 1 words (outputs); R-DUT-03 framing on inputs
        if (!s.bad && s.words != s.len + 1)
            LOG_ERROR(full_name(), is_input ? "R-DUT-03 " : "P-03 ", dir, "[", p, "] packet has ", s.words,
                      " words, header len ", s.len, " requires ", s.len + 1);
        if (!is_input && s.bad) LOG_ERROR(full_name(), "P-03 ", dir, "[", p, "] bad-length packet left the router");
        s.in_pkt = false;
    } else if (!s.bad && s.words > s.len) {
        LOG_ERROR(full_name(), is_input ? "R-DUT-03 " : "P-03 ", dir, "[", p, "] missing eop on word ", s.words);
    } else if (s.words > MAX_LEN + 1) {
        LOG_ERROR(full_name(), "P-03 ", dir, "[", p, "] packet longer than ", MAX_LEN + 1, " words");
    }
}

void ProtocolChecker::sample() {
    DutIf& d = sim_->dif();
    const int64_t now = static_cast<int64_t>(sim_->now());

    for (int p = 0; p < N_PORTS; ++p) {
        const bool v = d.in_valid(p), r = d.in_ready(p), sop = d.in_sop(p);
        const uint32_t data = d.in_data(p);
        // P-06: in_ready = 0 only when that input's FIFO holds 64 words
        if (!r && d.fifo_count(p) != FIFO_DEPTH)
            LOG_ERROR(full_name(), "P-06 in_ready[", p, "]=0 with FIFO count ", d.fifo_count(p));
        // R-DUT-02: in_ready stays 1 while a bad packet (header .. eop) is dropped
        const bool bad_hdr = v && (sop || !in_[p].in_pkt) && !Packet::len_legal(static_cast<int>(data & 0xFF));
        const bool dropping = v && in_[p].in_pkt && in_[p].bad;
        if ((bad_hdr || dropping) && !r)
            LOG_ERROR(full_name(), "R-DUT-02 in_ready[", p, "]=0 while dropping a bad-length packet");
        // P-07 latency (R-DUT-12): a good packet whose eop is written while its output is
        // idle with nothing else eligible must have its header leave <= 3 cycles later.
        const bool closes_good = v && r && d.in_eop(p) && in_[p].in_pkt && !in_[p].bad;
        const int dst = in_[p].dst;
        check_stream("in", p, in_[p], v, r, sop, d.in_eop(p), data, true);
        if (closes_good && perf_[dst] && !d.out_valid(dst) && d.req_count(dst) == 0 && pending_deadline_[dst] < 0)
            pending_deadline_[dst] = now + 3;
    }

    for (int o = 0; o < N_PORTS; ++o) {
        const bool v = d.out_valid(o), r = d.out_ready(o), sop = d.out_sop(o), eop = d.out_eop(o);
        check_stream("out", o, out_[o], v, r, sop, eop, d.out_data(o), false);

        // R-DUT-12: each output sustains 1 word/cycle -- no valid gaps inside a packet
        if (mid_pkt_out_[o] && !v) LOG_ERROR(full_name(), "P-07 out[", o, "] valid gap inside a packet");
        if (v && r) mid_pkt_out_[o] = !eop;

        // P-05 fairness: an eligible input is served before 4 other packets leave o
        if (d.gnt_valid(o)) {
            const int g = d.gnt_idx(o);
            for (int i = 0; i < N_PORTS; ++i) {
                if (i == g) {
                    waited_[o][i] = 0;
                } else if (d.req(o, i)) {
                    if (++waited_[o][i] > 3)
                        LOG_ERROR(full_name(), "P-05 in[", i, "] eligible for out[", o, "] but passed over ",
                                  waited_[o][i], " times");
                }
            }
        }
        for (int i = 0; i < N_PORTS; ++i)
            if (!d.req(o, i)) waited_[o][i] = 0;

        // P-07 performance (R-DUT-12), enabled per output by the test
        if (perf_[o]) {
            if (v && r && sop) {
                if (pending_deadline_[o] >= 0 && now > pending_deadline_[o])
                    LOG_ERROR(full_name(), "P-07 out[", o, "] header left ", now - pending_deadline_[o],
                              " cycle(s) late (R-DUT-12)");
                pending_deadline_[o] = -1;
            }
            if (v && r && eop) last_out_eop_[o] = now;
            // Back-to-back: a packet eligible the cycle after an eop starts within 1 idle cycle
            if (last_out_eop_[o] == now - 1 && d.req_count(o) > 0 && pending_deadline_[o] < 0)
                pending_deadline_[o] = now + 1;
        }
    }

}

void ProtocolChecker::check() {
    for (int p = 0; p < N_PORTS; ++p)
        if (out_[p].in_pkt) LOG_ERROR(full_name(), "P-02 out[", p, "] ended mid-packet");
}
