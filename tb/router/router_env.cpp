#include "router/router_env.hpp"

void RouterEnv::build() {
    for (int i = 0; i < N_PORTS; ++i) {
        in_agent[i] = create<InAgent>("in_agent" + std::to_string(i), i, true, sim_, rng_);
        out_agent[i] = create<OutAgent>("out_agent" + std::to_string(i), i, true, sim_, rng_);
    }
    sb = create<Scoreboard>("sb");
    checker = create<ProtocolChecker>("checker", sim_);
    cov = create<RouterCoverage>("cov", sim_);
}

void RouterEnv::connect() {
    // Monitors broadcast; the scoreboard and coverage subscribe without the monitors
    // knowing they exist (R-TB-05).
    for (int i = 0; i < N_PORTS; ++i) {
        in_agent[i]->mon->ap.connect([this, i](const Packet& p) { sb->write_in(i, p); });
        out_agent[i]->mon->ap.connect([this, i](const Packet& p) { sb->write_out(i, p); });
        cov->subscribe(in_agent[i]->mon->ap);
    }
    sb->matched_ap.connect([this](const MatchInfo& m) { cov->write_match(m); });
    sb->set_counter_probe([this]() {
        DutCounters c;
        for (int p = 0; p < N_PORTS; ++p) {
            c.pkt_in[p] = sim_->dif().pkt_in_cnt(p);
            c.pkt_out[p] = sim_->dif().pkt_out_cnt(p);
            c.drop[p] = sim_->dif().drop_cnt(p);
        }
        return c;
    });
    sim_->add_monitor(checker);
    sim_->add_monitor(cov);
}

bool RouterEnv::stimulus_done() const {
    for (auto* a : in_agent)
        if (a->is_active() && (!a->seqr->finished() || a->drv->busy())) return false;
    return true;
}

void RouterEnv::stop_sequences() {
    for (auto* a : in_agent)
        if (a->is_active()) a->seqr->stop();
}
