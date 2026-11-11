#include "router/scoreboard.hpp"

#include <algorithm>
#include <numeric>

#include "base/logger.hpp"

Scoreboard::Scoreboard(const std::string& name, Component* parent)
    : Component(name, parent), cmp_(std::make_unique<PacketComparator>()) {}

// Algorithm step 1: an input packet seen on port i
void Scoreboard::write_in(int port, const Packet& p) {
    Prediction pr = ref_.predict(port, p);
    if (pr.drop) {
        ++expected_drops_[port];
        ++stats_.in_drop[port];
        LOG_DEBUG(full_name(), "expect drop on in[", port, "] ", p.to_string());
        return;
    }
    ++stats_.in_good[port];
    exp_.at(port, pr.pkt.dst).push_back({pr.pkt, p.eop_cycle});
}

// Algorithm step 2: an output packet seen on port o
void Scoreboard::write_out(int port, const Packet& p) {
    ++stats_.out_pkts[port];
    stats_.out_words[port] += p.payload.size() + 1;
    const int src = p.src_tag;

    if (p.dst != port) {  // (a)
        ++stats_.errors_misrouted;
        LOG_ERROR(full_name(), "misrouted: out[", port, "] got packet for dst ", int(p.dst), " ", p.to_string());
        return;
    }
    auto& q = exp_.at(src, port);
    if (q.empty()) {  // (b)
        ++stats_.errors_unexpected;
        LOG_ERROR(full_name(), "unexpected packet on out[", port, "] tagged src ", src, " ", p.to_string());
        return;
    }
    std::string diff;
    if (!cmp_->compare(q.front().pkt, p, diff)) {  // (c)
        // Does it match a later entry? Then the router reordered a flow (R-DUT-10).
        for (size_t k = 1; k < q.size(); ++k) {
            std::string d2;
            if (cmp_->compare(q[k].pkt, p, d2)) {
                ++stats_.errors_order;
                LOG_ERROR(full_name(), "out of order on flow in[", src, "]->out[", port, "]: got expected entry #", k,
                          " first; head was ", q.front().pkt.to_string());
                q.erase(q.begin() + static_cast<long>(k));
                return;
            }
        }
        ++stats_.errors_mismatch;
        LOG_ERROR(full_name(), "data mismatch on out[", port, "] flow in[", src, "]: ", diff);
        q.pop_front();
        return;
    }
    // (d) match: pop and record latency (output sop cycle - input eop cycle)
    const uint64_t lat = p.sop_cycle - q.front().in_eop_cycle;
    stats_.add_latency(lat);
    q.pop_front();
    matched_ap.write({src, port, lat});
}

// Algorithm step 3: end of test
void Scoreboard::check() {
    if (!exp_.empty()) {
        for (int s = 0; s < N_PORTS; ++s)
            for (int d = 0; d < N_PORTS; ++d) {
                const auto& q = exp_.at(s, d);
                if (q.empty()) continue;
                LOG_ERROR(full_name(), "leftover: ", q.size(), " packet(s) in[", s, "]->out[", d,
                          "] never left the router, first ", q.front().pkt.to_string());
            }
    }
    if (!probe_) return;
    const DutCounters c = probe_();
    for (int p = 0; p < N_PORTS; ++p) {
        const auto m = [](uint64_t v) { return static_cast<uint16_t>(v & 0xFFFF); };  // mod 2^16
        if (c.pkt_in[p] != m(stats_.in_good[p]))
            LOG_ERROR(full_name(), "R-DUT-13 pkt_in_cnt[", p, "]=", c.pkt_in[p], " expected ", m(stats_.in_good[p]));
        if (c.drop[p] != m(expected_drops_[p]))
            LOG_ERROR(full_name(), "R-DUT-13 drop_cnt[", p, "]=", c.drop[p], " expected ", m(expected_drops_[p]));
        if (c.pkt_out[p] != m(stats_.out_pkts[p]))
            LOG_ERROR(full_name(), "R-DUT-13 pkt_out_cnt[", p, "]=", c.pkt_out[p], " expected ", m(stats_.out_pkts[p]));
    }
}

void Scoreboard::report() {
    uint64_t in = 0, drop = 0, out = 0;
    for (int p = 0; p < N_PORTS; ++p) {
        in += stats_.in_good[p];
        drop += stats_.in_drop[p];
        out += stats_.out_pkts[p];
    }
    const auto& L = stats_.latencies;
    const double avg = L.empty() ? 0.0 : std::accumulate(L.begin(), L.end(), 0.0) / L.size();
    LOG_INFO(full_name(), "packets in=", in, " dropped=", drop, " out=", out, " matched=", L.size(),
             " latency avg=", avg, " max=", L.empty() ? 0 : *std::max_element(L.begin(), L.end()));
}

void SbStats::to_json(JsonWriter& w, uint64_t cycles) const {
    auto arr = [&](const char* k, const std::array<uint64_t, N_PORTS>& a) {
        w.key(k).begin_arr();
        for (auto v : a) w.val(v);
        w.end_arr();
    };
    w.begin_obj();
    arr("in_good", in_good);
    arr("in_drop", in_drop);
    arr("out_pkts", out_pkts);
    arr("out_words", out_words);
    w.key("throughput").begin_arr();
    for (auto v : out_words) w.val(cycles ? static_cast<double>(v) / static_cast<double>(cycles) : 0.0);
    w.end_arr();
    // Latency histogram, bins match cg_egress latency: <=3, 4-10, 11-50, 51-200, >200
    std::array<uint64_t, 5> hist{};
    uint64_t mn = UINT64_MAX, mx = 0;
    double sum = 0;
    for (auto l : latencies) {
        hist[l <= 3 ? 0 : l <= 10 ? 1 : l <= 50 ? 2 : l <= 200 ? 3 : 4]++;
        mn = std::min(mn, l);
        mx = std::max(mx, l);
        sum += static_cast<double>(l);
    }
    w.key("latency").begin_obj();
    w.key("count").val(latencies.size());
    w.key("min").val(latencies.empty() ? 0 : mn);
    w.key("max").val(mx);
    w.key("avg").val(latencies.empty() ? 0.0 : sum / static_cast<double>(latencies.size()));
    w.key("hist").begin_obj();
    const char* names[5] = {"<=3", "4-10", "11-50", "51-200", ">200"};
    for (int i = 0; i < 5; ++i) w.key(names[i]).val(hist[i]);
    w.end_obj();
    w.end_obj();
    w.key("errors").begin_obj();
    w.key("misrouted").val(errors_misrouted);
    w.key("unexpected").val(errors_unexpected);
    w.key("mismatch").val(errors_mismatch);
    w.key("out_of_order").val(errors_order);
    w.end_obj();
    w.end_obj();
}
