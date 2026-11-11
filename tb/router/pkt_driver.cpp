#include "base/logger.hpp"
#include "router/agents.hpp"

void PktDriver::drive() {
    if (!cur_ && seqr_) {
        cur_ = seqr_->get_next_item();
        if (cur_) {
            words_ = cur_->words();
            idx_ = 0;
            wait_ = cur_->gap;
            LOG_DEBUG(full_name(), "start ", cur_->to_string(), " gap=", cur_->gap);
        }
    }
    if (cur_ && wait_ == 0) {
        driving_ = true;
        dif_->set_in(port_, true, idx_ == 0, idx_ + 1 == words_.size(), words_[idx_]);
    } else {
        driving_ = false;
        if (wait_ > 0) --wait_;
        dif_->set_in(port_, false, false, false, 0);
    }
}

void PktDriver::post_eval() {
    // R-DUT-01: the word moves on this edge iff valid && ready; otherwise everything
    // is held as-is for the next cycle.
    if (!driving_ || !dif_->in_ready(port_)) return;
    ++idx_;
    if (idx_ == words_.size()) {
        cur_.reset();
        ++sent_;
    } else if (rng_->chance(cur_->bubble_rate)) {
        wait_ = rng_->range(1, 2);  // valid bubble before the next word
    }
}

void InAgent::build() {
    if (is_active_) {
        seqr = create<Sequencer<Packet>>("seqr");
        seqr->set_rng(rng_);
        drv = create<PktDriver>("drv", port_, &sim_->dif(), rng_);
    }
    mon = create<InMonitor>("mon", port_, sim_);
}

void InAgent::connect() {
    if (is_active_) {
        drv->set_sequencer(seqr);
        sim_->add_driver(drv);
    }
    sim_->add_monitor(mon);
}
