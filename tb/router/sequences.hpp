// sequences.hpp -- the sequence library (spec 6.2). Each sequence shapes a private copy
// of the test's TrafficCfg, then produces `count` packets for one input port.
#pragma once
#include <array>
#include <memory>
#include <vector>

#include "base/logger.hpp"
#include "base/sequencer.hpp"
#include "router/packet.hpp"

class PacketSeq : public Sequence<Packet> {
public:
    PacketSeq(const std::string& name, int src, int count, const TrafficCfg& cfg, uint16_t* flow_seq)
        : Sequence<Packet>(name), src_(src), count_(count), cfg_(cfg), flow_seq_(flow_seq) {}

    std::unique_ptr<Packet> next(Rng& rng) override {
        if (sent_ >= count_) return nullptr;
        auto p = std::make_unique<Packet>();
        p->src = static_cast<uint8_t>(src_);
        // flow_id = {src[1:0], seq[13:0]}, seq increments per source
        p->flow_id = static_cast<uint16_t>((src_ << 14) | (*flow_seq_ & 0x3FFF));
        ++*flow_seq_;
        p->randomize(rng, cfg_);
        shape(*p, rng);
        ++sent_;
        return p;
    }
    const TrafficCfg& cfg() const { return cfg_; }

protected:
    virtual void shape(Packet&, Rng&) {}  // per-packet tweak after randomize()
    int src_, count_, sent_ = 0;
    TrafficCfg cfg_;
    uint16_t* flow_seq_;
};

// Default distributions, uniform destinations: general mixing
class RandomTrafficSeq : public PacketSeq {
public:
    RandomTrafficSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs)
        : PacketSeq("RandomTrafficSeq", src, count, cfg, fs) {}
};

// 80-100% of packets to one output: contention, fairness, FIFO full
class HotspotSeq : public PacketSeq {
public:
    HotspotSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs) : PacketSeq("HotspotSeq", src, count, cfg, fs) {
        cfg_.dst_mode = DstMode::HOTSPOT;
    }
};

// Each input to a different output, no collisions: full throughput (R-DUT-12)
class PermutationSeq : public PacketSeq {
public:
    PermutationSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs)
        : PacketSeq("PermutationSeq", src, count, cfg, fs) {
        cfg_.dst_mode = DstMode::PERMUTATION;
    }
};

// Back-to-back 31-word packets, zero gaps: FIFO depth, store-and-forward
class MaxBurstSeq : public PacketSeq {
public:
    MaxBurstSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs) : PacketSeq("MaxBurstSeq", src, count, cfg, fs) {
        cfg_.len_dist = WeightedDist::constant(MAX_LEN);
        cfg_.gap_dist = WeightedDist::constant(0);
        cfg_.bubble_rate = 0.0;
    }
};

// 1-word payloads, zero gaps: header overhead, arbiter re-grant speed
class MinLenSeq : public PacketSeq {
public:
    MinLenSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs) : PacketSeq("MinLenSeq", src, count, cfg, fs) {
        cfg_.len_dist = WeightedDist::constant(1);
        cfg_.gap_dist = WeightedDist::constant(0);
        cfg_.bubble_rate = 0.0;
    }

protected:
    // Bad-length packets stay minimal too (1 payload word, still legal framing per R-STIM-02)
    void shape(Packet& p, Rng&) override {
        if (p.bad_len) p.payload.resize(1);
    }
};

// Mix of good and bad-length packets: drop path, drop_cnt
class ErrorInjectSeq : public PacketSeq {
public:
    ErrorInjectSeq(int src, int count, const TrafficCfg& cfg, uint16_t* fs)
        : PacketSeq("ErrorInjectSeq", src, count, cfg, fs) {
        if (cfg_.err_rate <= 0.0) cfg_.err_rate = 0.10;
    }
};

// Directed: one packet to each listed destination, in order (smoke test)
class DirectedSeq : public PacketSeq {
public:
    DirectedSeq(int src, std::vector<int> dsts, const TrafficCfg& cfg, uint16_t* fs)
        : PacketSeq("DirectedSeq", src, static_cast<int>(dsts.size()), cfg, fs), dsts_(std::move(dsts)) {}

protected:
    void shape(Packet& p, Rng&) override { p.dst = static_cast<uint8_t>(dsts_[sent_]); }

private:
    std::vector<int> dsts_;
};

// Virtual sequence: runs one of the above per input, chosen at random (pattern interactions)
class MixedVirtualSeq {
public:
    static constexpr int N_KINDS = 6;
    static const char* kind_name(int k) {
        static const char* n[N_KINDS] = {"RandomTrafficSeq", "HotspotSeq",  "PermutationSeq",
                                         "MaxBurstSeq",      "MinLenSeq",   "ErrorInjectSeq"};
        return n[k];
    }
    static std::unique_ptr<PacketSeq> make(int kind, int src, int count, const TrafficCfg& cfg, uint16_t* fs) {
        switch (kind) {
            case 1: return std::make_unique<HotspotSeq>(src, count, cfg, fs);
            case 2: return std::make_unique<PermutationSeq>(src, count, cfg, fs);
            case 3: return std::make_unique<MaxBurstSeq>(src, count, cfg, fs);
            case 4: return std::make_unique<MinLenSeq>(src, count, cfg, fs);
            case 5: return std::make_unique<ErrorInjectSeq>(src, count, cfg, fs);
            default: return std::make_unique<RandomTrafficSeq>(src, count, cfg, fs);
        }
    }
    // Start a randomly chosen sequence on every input's sequencer.
    static void start(std::array<Sequencer<Packet>*, N_PORTS>& seqrs, std::array<uint16_t*, N_PORTS>& flow_seqs,
                      const std::array<int, N_PORTS>& counts, const TrafficCfg& cfg, Rng& rng) {
        for (int i = 0; i < N_PORTS; ++i) {
            int kind = rng.range(0, N_KINDS - 1);
            LOG_INFO("MixedVirtualSeq", "input ", i, " runs ", kind_name(kind), " (", counts[i], " packets)");
            seqrs[i]->start(make(kind, i, counts[i], cfg, flow_seqs[i]));
        }
    }
};
