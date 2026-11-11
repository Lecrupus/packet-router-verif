// dut_if.hpp -- per-port accessors for the Verilated router (the C++ "virtual interface").
// Hides Verilator's packed-array encoding: N_PORTS x 32-bit buses are VlWide<4>, the
// 16-bit counter vectors are a 64-bit QData, flag vectors are one CData bit per port.
#pragma once
#include <cstdint>

#include "Vrouter_top.h"
#include "Vrouter_top___024root.h"
#include "router/router_defs.hpp"

class DutIf {
public:
    explicit DutIf(Vrouter_top* dut) : d_(dut) {}
    Vrouter_top* dut() const { return d_; }

    // ---- ingress (driven by PktDriver) ----
    void set_in(int p, bool valid, bool sop, bool eop, uint32_t data) {
        set_bit(d_->in_valid, p, valid);
        set_bit(d_->in_sop, p, sop);
        set_bit(d_->in_eop, p, eop);
        d_->in_data[p] = data;
    }
    bool in_valid(int p) const { return bit(d_->in_valid, p); }
    bool in_sop(int p) const { return bit(d_->in_sop, p); }
    bool in_eop(int p) const { return bit(d_->in_eop, p); }
    uint32_t in_data(int p) const { return d_->in_data[p]; }
    bool in_ready(int p) const { return bit(d_->in_ready, p); }

    // ---- egress ----
    void set_out_ready(int p, bool r) { set_bit(d_->out_ready, p, r); }
    bool out_ready(int p) const { return bit(d_->out_ready, p); }
    bool out_valid(int p) const { return bit(d_->out_valid, p); }
    bool out_sop(int p) const { return bit(d_->out_sop, p); }
    bool out_eop(int p) const { return bit(d_->out_eop, p); }
    uint32_t out_data(int p) const { return d_->out_data[p]; }

    // ---- statistics ----
    uint16_t pkt_in_cnt(int p) const { return field16(d_->pkt_in_cnt, p); }
    uint16_t pkt_out_cnt(int p) const { return field16(d_->pkt_out_cnt, p); }
    uint16_t drop_cnt(int p) const { return field16(d_->drop_cnt, p); }

    // ---- debug signals (verilator public_flat_rd), read-only ----
    int fifo_count(int p) const { return (d_->rootp->router_top__DOT__dbg_fifo_count >> (7 * p)) & 0x7F; }
    // Input i requests output o (its complete head packet targets o)
    bool req(int o, int i) const { return (d_->rootp->router_top__DOT__dbg_req >> (4 * o + i)) & 1; }
    int req_count(int o) const {
        int n = 0;
        for (int i = 0; i < N_PORTS; ++i) n += req(o, i);
        return n;
    }
    bool gnt_valid(int o) const { return (d_->rootp->router_top__DOT__dbg_gnt_valid >> o) & 1; }
    int gnt_idx(int o) const { return (d_->rootp->router_top__DOT__dbg_gnt_idx >> (2 * o)) & 3; }

private:
    template <class T>
    static void set_bit(T& v, int p, bool b) {
        v = static_cast<T>(b ? (v | (1u << p)) : (v & ~(1u << p)));
    }
    template <class T>
    static bool bit(const T& v, int p) { return (v >> p) & 1; }
    static uint16_t field16(uint64_t v, int p) { return static_cast<uint16_t>(v >> (16 * p)); }

    Vrouter_top* d_;
};
