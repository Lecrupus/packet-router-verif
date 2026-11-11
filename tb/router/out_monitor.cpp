#include "base/logger.hpp"
#include "router/agents.hpp"

void OutMonitor::sample() {
    DutIf& d = sim_->dif();
    if (!(d.out_valid(port_) && d.out_ready(port_))) return;

    const uint64_t now = sim_->now();
    ++words_total_;
    if (d.out_sop(port_) || !in_pkt_) {
        words_.clear();
        in_pkt_ = true;
        sop_cycle_ = now;
    }
    words_.push_back(d.out_data(port_));
    if (d.out_eop(port_)) {
        Packet p = Packet::from_words(words_);
        p.src = p.src_tag;  // the router's tag says where it came from
        p.port = port_;
        p.sop_cycle = sop_cycle_;
        p.eop_cycle = now;
        in_pkt_ = false;
        ++packets_;
        LOG_DEBUG(full_name(), "out ", p.to_string());
        ap.write(p);
    }
}
