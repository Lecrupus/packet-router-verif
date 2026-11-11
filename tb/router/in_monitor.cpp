#include "base/logger.hpp"
#include "router/agents.hpp"

// Samples after inputs settle, before the rising edge: a word transfers iff valid && ready.
// Packets are framed by sop/eop as seen; framing errors are the protocol checker's job.
void InMonitor::sample() {
    DutIf& d = sim_->dif();
    const bool v = d.in_valid(port_);
    if (in_pkt_ && !v) bubble_ = true;
    if (!(v && d.in_ready(port_))) return;

    const uint64_t now = sim_->now();
    if (d.in_sop(port_) || !in_pkt_) {
        words_.clear();
        in_pkt_ = true;
        bubble_ = false;
        sop_cycle_ = now;
        gap_ = last_eop_ < 0 ? -1 : static_cast<int>(now - static_cast<uint64_t>(last_eop_) - 1);
    }
    words_.push_back(d.in_data(port_));
    if (d.in_eop(port_)) {
        Packet p = Packet::from_words(words_);
        p.src = static_cast<uint8_t>(port_);
        p.port = port_;
        p.sop_cycle = sop_cycle_;
        p.eop_cycle = now;
        p.gap_seen = gap_;
        p.bubble_seen = bubble_;
        in_pkt_ = false;
        last_eop_ = static_cast<int64_t>(now);
        ++packets_;
        LOG_DEBUG(full_name(), "in  ", p.to_string());
        ap.write(p);
    }
}
