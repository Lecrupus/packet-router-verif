// packet.hpp -- Packet (uvm_sequence_item): header fields + payload, randomize(),
// compare(). Also carries stimulus knobs (gap, bubbles) and what monitors observed.
#pragma once
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/object.hpp"
#include "base/rng.hpp"
#include "router/traffic_cfg.hpp"

struct Packet : SeqItem {
    // ---- header fields (spec 2.2) ----
    uint8_t len = 0;       // [7:0]   (illegal values when bad_len)
    uint8_t dst = 0;       // [9:8]
    uint8_t src_tag = 0;   // [11:10] sender sets 0; router writes the ingress port
    uint8_t reserved = 0;  // [15:12] passed through
    uint16_t flow_id = 0;  // [31:16] {src[1:0], seq[13:0]}
    // ---- testbench knowledge ----
    uint8_t src = 0;       // ingress port
    bool bad_len = false;  // error injection
    std::vector<uint32_t> payload;
    // ---- stimulus knobs (used by the driver) ----
    int gap = 0;               // idle cycles before the header
    double bubble_rate = 0.0;  // per-word chance of 1-2 idle cycles
    // ---- observed by monitors ----
    int port = -1;
    uint64_t sop_cycle = 0, eop_cycle = 0;
    int gap_seen = -1;          // idle cycles since the previous eop on this port (-1: first)
    bool bubble_seen = false;   // valid dropped inside the packet

    Packet() : SeqItem("pkt") {}

    static bool len_legal(int l) { return l >= 1 && l <= MAX_LEN; }

    uint32_t header() const {
        return static_cast<uint32_t>(len) | (static_cast<uint32_t>(dst & 3) << 8) |
               (static_cast<uint32_t>(src_tag & 3) << 10) | (static_cast<uint32_t>(reserved & 0xF) << 12) |
               (static_cast<uint32_t>(flow_id) << 16);
    }
    void set_header(uint32_t h) {
        len = h & 0xFF;
        dst = (h >> 8) & 3;
        src_tag = (h >> 10) & 3;
        reserved = (h >> 12) & 0xF;
        flow_id = static_cast<uint16_t>(h >> 16);
        bad_len = !len_legal(len);
    }
    // Rebuild from transferred words (monitors)
    static Packet from_words(const std::vector<uint32_t>& words) {
        Packet p;
        if (!words.empty()) {
            p.set_header(words[0]);
            p.payload.assign(words.begin() + 1, words.end());
        }
        return p;
    }
    std::vector<uint32_t> words() const {
        std::vector<uint32_t> w;
        w.reserve(payload.size() + 1);
        w.push_back(header());
        w.insert(w.end(), payload.begin(), payload.end());
        return w;
    }

    // Constrained randomization (spec 6.1). src and flow_id are set by the sequence.
    void randomize(Rng& rng, const TrafficCfg& cfg) {
        len = static_cast<uint8_t>(cfg.len_dist.pick(rng));
        dst = static_cast<uint8_t>(cfg.pick_dst(src, rng));
        reserved = static_cast<uint8_t>(rng.range(0, 15));
        src_tag = 0;
        bad_len = rng.chance(cfg.err_rate);
        if (bad_len) len = static_cast<uint8_t>(rng.chance(0.5) ? 0 : rng.range(32, 255));
        // R-STIM-02: real word count stays 1..31 even for bad lengths; eop placed correctly
        payload.resize(bad_len ? rng.range(1, MAX_LEN) : len);
        for (size_t k = 0; k < payload.size(); ++k)
            payload[k] = cfg.payload_count
                             ? (static_cast<uint32_t>(flow_id) << 16) | static_cast<uint32_t>(k + 1)
                             : rng.u32();
        gap = cfg.gap_dist.pick(rng);
        bubble_rate = cfg.bubble_rate;
    }

    bool compare(const Packet& o, std::string& why) const {
        std::ostringstream os;
        if (header() != o.header())
            os << "header exp=0x" << std::hex << std::setw(8) << std::setfill('0') << header() << " act=0x"
               << std::setw(8) << o.header() << std::dec << " ";
        if (payload.size() != o.payload.size()) os << "payload words exp=" << payload.size() << " act=" << o.payload.size() << " ";
        int shown = 0;
        for (size_t k = 0; k < payload.size() && k < o.payload.size() && shown < 3; ++k)
            if (payload[k] != o.payload[k]) {
                os << "payload[" << k << "] exp=0x" << std::hex << payload[k] << " act=0x" << o.payload[k] << std::dec << " ";
                ++shown;
            }
        why = os.str();
        return why.empty();
    }

    std::string to_string() const override {
        std::ostringstream os;
        os << "pkt{src=" << int(src) << " dst=" << int(dst) << " len=" << int(len) << (bad_len ? "(BAD)" : "")
           << " tag=" << int(src_tag) << " flow=0x" << std::hex << flow_id << std::dec << " words=" << payload.size() + 1
           << "}";
        return os.str();
    }
    std::unique_ptr<Object> clone() const override { return std::make_unique<Packet>(*this); }
};
