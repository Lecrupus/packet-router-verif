// traffic_cfg.hpp -- TrafficCfg: every stimulus knob as a weighted distribution or
// value (spec 6), the C++ equivalent of SV `constraint`/`dist` blocks. Each test sets
// its defaults, then apply_plusargs() lets the command line override any knob (R-STIM-01).
#pragma once
#include <array>
#include <sstream>
#include <string>

#include "base/rng.hpp"
#include "router/router_defs.hpp"
#include "router/plusargs.hpp"

enum class DstMode { UNIFORM, HOTSPOT, PERMUTATION };
enum class BpPattern { ALWAYS, RANDOM, BURSTY, HOLD_THEN_RELEASE };

inline const char* to_str(DstMode m) {
    switch (m) {
        case DstMode::UNIFORM: return "uniform";
        case DstMode::HOTSPOT: return "hotspot";
        default: return "permutation";
    }
}
inline const char* to_str(BpPattern b) {
    switch (b) {
        case BpPattern::ALWAYS: return "ALWAYS";
        case BpPattern::RANDOM: return "RANDOM";
        case BpPattern::BURSTY: return "BURSTY";
        default: return "HOLD_THEN_RELEASE";
    }
}
inline BpPattern parse_bp(const std::string& s) {
    if (s == "RANDOM") return BpPattern::RANDOM;
    if (s == "BURSTY") return BpPattern::BURSTY;
    if (s == "HOLD_THEN_RELEASE" || s == "HOLD") return BpPattern::HOLD_THEN_RELEASE;
    return BpPattern::ALWAYS;
}

struct BpCfg {
    BpPattern pattern = BpPattern::ALWAYS;
    double p = 0.7;              // RANDOM: P(out_ready = 1)
    double burst_start = 0.03;   // BURSTY: P(a stall starts) per ready cycle
    int burst_min = 16, burst_max = 64;
    int hold_cycles = 500;       // HOLD_THEN_RELEASE
};

struct TrafficCfg {
    int num_pkts = 2000;  // total over all inputs
    // len dist {1:=10, [2:7]:/30, [8:15]:/30, [16:30]:/20, 31:=10}
    WeightedDist len_dist{{1, 1, 10}, {2, 7, 30}, {8, 15, 30}, {16, 30, 20}, {31, 31, 10}};
    DstMode dst_mode = DstMode::UNIFORM;
    int hotspot_port = 0;
    double hotspot_pct = 0.9;
    std::array<int, N_PORTS> perm{0, 1, 2, 3};  // PERMUTATION: src -> dst
    double err_rate = 0.0;                      // bad_len probability
    // gap dist {0:=50, [1:3]:/30, [4:20]:/20}
    WeightedDist gap_dist{{0, 0, 50}, {1, 3, 30}, {4, 20, 20}};
    double bubble_rate = 0.10;   // per word: chance of 1-2 idle cycles first
    bool payload_count = false;  // +PAYLOAD=count: counting pattern for debug
    std::array<BpCfg, N_PORTS> bp{};

    int pick_dst(int src, Rng& rng) const {
        switch (dst_mode) {
            case DstMode::HOTSPOT:
                return rng.chance(hotspot_pct) ? hotspot_port : rng.range(0, N_PORTS - 1);
            case DstMode::PERMUTATION:
                return perm[src];
            default:
                return rng.range(0, N_PORTS - 1);
        }
    }

    void set_bp_all(BpPattern pat) {
        for (auto& b : bp) b.pattern = pat;
    }

    // R-STIM-01: command-line overrides for every knob
    void apply_plusargs(const PlusArgs& a) {
        num_pkts = static_cast<int>(a.num("NUM_PKTS", num_pkts));
        err_rate = a.real("ERR_RATE", err_rate);
        bubble_rate = a.real("BUBBLE_RATE", bubble_rate);
        hotspot_pct = a.real("HOTSPOT_PCT", hotspot_pct);
        hotspot_port = static_cast<int>(a.num("HOTSPOT_PORT", hotspot_port));
        if (a.has("LEN")) len_dist = WeightedDist::constant(static_cast<int>(a.num("LEN", 1)));
        if (a.has("LEN_MIN") || a.has("LEN_MAX"))
            len_dist = WeightedDist{{static_cast<int>(a.num("LEN_MIN", 1)), static_cast<int>(a.num("LEN_MAX", 31)), 1}};
        if (a.has("GAP")) gap_dist = WeightedDist::constant(static_cast<int>(a.num("GAP", 0)));
        if (a.has("DST_MODE")) {
            auto m = a.str("DST_MODE");
            dst_mode = m == "hotspot" ? DstMode::HOTSPOT : m == "permutation" ? DstMode::PERMUTATION : DstMode::UNIFORM;
        }
        if (a.str("PAYLOAD") == "count") payload_count = true;
        for (int p = 0; p < N_PORTS; ++p) {
            if (a.has("BP")) bp[p].pattern = parse_bp(a.str("BP"));
            bp[p].p = a.real("BP_P", bp[p].p);
            bp[p].hold_cycles = static_cast<int>(a.num("HOLD_CYCLES", bp[p].hold_cycles));
        }
    }

    // R-STIM-03: log the effective config so a seed's traffic can be explained
    std::string to_string() const {
        std::ostringstream os;
        os << "num_pkts=" << num_pkts << " len=" << len_dist.to_string() << " dst_mode=" << to_str(dst_mode);
        if (dst_mode == DstMode::HOTSPOT) os << "(port=" << hotspot_port << ",pct=" << hotspot_pct << ")";
        if (dst_mode == DstMode::PERMUTATION)
            os << "(" << perm[0] << perm[1] << perm[2] << perm[3] << ")";
        os << " err_rate=" << err_rate << " gap=" << gap_dist.to_string() << " bubble_rate=" << bubble_rate
           << " payload=" << (payload_count ? "count" : "random") << " bp=[";
        for (int p = 0; p < N_PORTS; ++p) {
            if (p) os << ",";
            os << to_str(bp[p].pattern);
            if (bp[p].pattern == BpPattern::RANDOM) os << "(p=" << bp[p].p << ")";
            if (bp[p].pattern == BpPattern::HOLD_THEN_RELEASE) os << "(" << bp[p].hold_cycles << ")";
        }
        os << "]";
        return os.str();
    }
};
