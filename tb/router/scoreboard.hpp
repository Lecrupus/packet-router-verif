// scoreboard.hpp -- Scoreboard (uvm_scoreboard) and its four helpers (spec 7.1):
//   RefModel           owned       predicts the output packet, or a drop
//   ExpectedQueueBank  owned       4x4 deques indexed [src][dst]
//   PacketComparator   owned,      field-by-field compare with a readable diff
//                      swappable   (strategy pattern)
//   SbStats            owned       counts, latency histogram, throughput; summary.json
#pragma once
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/analysis_port.hpp"
#include "base/component.hpp"
#include "base/json.hpp"
#include "router/ref_model.hpp"

struct Expected {
    Packet pkt;
    uint64_t in_eop_cycle;
};

class ExpectedQueueBank {
public:
    std::deque<Expected>& at(int src, int dst) { return q_[src][dst]; }
    const std::deque<Expected>& at(int src, int dst) const { return q_[src][dst]; }
    size_t total() const {
        size_t n = 0;
        for (const auto& row : q_)
            for (const auto& d : row) n += d.size();
        return n;
    }
    bool empty() const { return total() == 0; }

private:
    std::array<std::array<std::deque<Expected>, N_PORTS>, N_PORTS> q_;
};

class PacketComparator {
public:
    virtual ~PacketComparator() = default;
    virtual bool compare(const Packet& exp, const Packet& act, std::string& diff) const {
        return exp.compare(act, diff);
    }
};

struct DutCounters {
    std::array<uint16_t, N_PORTS> pkt_in{}, pkt_out{}, drop{};
};

// Emitted for every matched packet; coverage subscribes for the latency bins.
struct MatchInfo {
    int src, dst;
    uint64_t latency;
};

class SbStats {
public:
    std::array<uint64_t, N_PORTS> in_good{}, in_drop{}, out_pkts{}, out_words{};
    std::vector<uint64_t> latencies;
    uint64_t errors_misrouted = 0, errors_unexpected = 0, errors_mismatch = 0, errors_order = 0;

    void add_latency(uint64_t l) { latencies.push_back(l); }
    void to_json(JsonWriter& w, uint64_t cycles) const;
};

class Scoreboard : public Component {
public:
    Scoreboard(const std::string& name, Component* parent);

    void write_in(int port, const Packet& p);   // from InMonitor[port]
    void write_out(int port, const Packet& p);  // from OutMonitor[port]
    void check() override;                      // end-of-test checks
    void report() override;

    void set_comparator(std::unique_ptr<PacketComparator> c) { cmp_ = std::move(c); }
    void set_counter_probe(std::function<DutCounters()> f) { probe_ = std::move(f); }
    bool drained() const { return exp_.empty(); }
    size_t pending() const { return exp_.total(); }
    const SbStats& stats() const { return stats_; }
    void set_cycle_source(const uint64_t* c) { now_ = c; }

    AnalysisPort<MatchInfo> matched_ap;

private:
    RefModel ref_;
    ExpectedQueueBank exp_;  // exp_[src][dst]
    std::unique_ptr<PacketComparator> cmp_;
    SbStats stats_;
    std::array<uint64_t, N_PORTS> expected_drops_{};
    std::function<DutCounters()> probe_;
    const uint64_t* now_ = nullptr;
};
