#include "router/agents.hpp"

void ReadyResponder::drive() {
    bool ready = true;
    switch (cfg_.pattern) {
        case BpPattern::ALWAYS:
            ready = true;
            break;
        case BpPattern::RANDOM:
            ready = rng_->chance(cfg_.p);
            break;
        case BpPattern::BURSTY:
            if (stall_left_ > 0) {
                --stall_left_;
                ready = false;
            } else if (rng_->chance(cfg_.burst_start)) {
                stall_left_ = rng_->range(cfg_.burst_min, cfg_.burst_max) - 1;
                ready = false;
            }
            break;
        case BpPattern::HOLD_THEN_RELEASE:
            ready = cycles_ >= static_cast<uint64_t>(cfg_.hold_cycles);
            break;
    }
    ++cycles_;
    dif_->set_out_ready(port_, ready);
}

void OutAgent::build() {
    if (is_active_) rsp = create<ReadyResponder>("rsp", port_, &sim_->dif(), rng_);
    mon = create<OutMonitor>("mon", port_, sim_);
}

void OutAgent::connect() {
    if (is_active_) sim_->add_driver(rsp);
    sim_->add_monitor(mon);
}
