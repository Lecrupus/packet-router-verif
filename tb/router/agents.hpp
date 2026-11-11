// agents.hpp -- router agents and their parts:
//   PktDriver (uvm_driver), ReadyResponder (reactive driver), InMonitor / OutMonitor
//   (uvm_monitor), InAgent / OutAgent (uvm_agent).
#pragma once
#include <memory>
#include <vector>

#include "base/agent.hpp"
#include "base/driver.hpp"
#include "base/monitor.hpp"
#include "base/sequencer.hpp"
#include "router/packet.hpp"
#include "router/sim.hpp"

// Turns a Packet into word-level valid/sop/eop/data, obeys in_ready, inserts random
// idle gaps and valid bubbles. Holds data/sop/eop stable while valid && !ready.
class PktDriver : public Driver<Packet> {
public:
    PktDriver(const std::string& name, Component* parent, int port, DutIf* dif, Rng* rng)
        : Driver<Packet>(name, parent), port_(port), dif_(dif), rng_(rng) {}
    void drive() override;
    void post_eval() override;
    bool busy() const override { return cur_ != nullptr; }
    uint64_t packets_sent() const { return sent_; }

private:
    int port_;
    DutIf* dif_;
    Rng* rng_;
    std::unique_ptr<Packet> cur_;
    std::vector<uint32_t> words_;
    size_t idx_ = 0;
    int wait_ = 0;
    bool driving_ = false;
    uint64_t sent_ = 0;
};

// Drives out_ready with a backpressure pattern (spec 6.3)
class ReadyResponder : public Component, public Drivable {
public:
    ReadyResponder(const std::string& name, Component* parent, int port, DutIf* dif, Rng* rng)
        : Component(name, parent), port_(port), dif_(dif), rng_(rng) {}
    void configure(const BpCfg& cfg) { cfg_ = cfg; }
    void drive() override;

private:
    int port_;
    DutIf* dif_;
    Rng* rng_;
    BpCfg cfg_;
    uint64_t cycles_ = 0;
    int stall_left_ = 0;
};

// Rebuild input packets from accepted words and publish them
class InMonitor : public Monitor<Packet> {
public:
    InMonitor(const std::string& name, Component* parent, int port, Sim* sim) : Monitor<Packet>(name, parent), port_(port), sim_(sim) {}
    void sample() override;
    uint64_t packets() const { return packets_; }

private:
    int port_;
    Sim* sim_;
    bool in_pkt_ = false;
    bool bubble_ = false;
    std::vector<uint32_t> words_;
    uint64_t sop_cycle_ = 0;
    int64_t last_eop_ = -1;
    int gap_ = -1;
    uint64_t packets_ = 0;
};

// Rebuild output packets from transferred words and publish them
class OutMonitor : public Monitor<Packet> {
public:
    OutMonitor(const std::string& name, Component* parent, int port, Sim* sim) : Monitor<Packet>(name, parent), port_(port), sim_(sim) {}
    void sample() override;
    uint64_t packets() const { return packets_; }
    uint64_t words() const { return words_total_; }

private:
    int port_;
    Sim* sim_;
    bool in_pkt_ = false;
    std::vector<uint32_t> words_;
    uint64_t sop_cycle_ = 0;
    uint64_t packets_ = 0, words_total_ = 0;
};

// Sequencer + driver + monitor for one input port; passive builds the monitor only
class InAgent : public Agent {
public:
    InAgent(const std::string& name, Component* parent, int port, bool is_active, Sim* sim, Rng* rng)
        : Agent(name, parent, port, is_active), sim_(sim), rng_(rng) {}
    void build() override;
    void connect() override;

    Sequencer<Packet>* seqr = nullptr;
    PktDriver* drv = nullptr;
    InMonitor* mon = nullptr;
    uint16_t flow_seq = 0;  // per-source sequence number for flow_id

private:
    Sim* sim_;
    Rng* rng_;
};

// ReadyResponder + monitor for one output port
class OutAgent : public Agent {
public:
    OutAgent(const std::string& name, Component* parent, int port, bool is_active, Sim* sim, Rng* rng)
        : Agent(name, parent, port, is_active), sim_(sim), rng_(rng) {}
    void build() override;
    void connect() override;

    ReadyResponder* rsp = nullptr;
    OutMonitor* mon = nullptr;

private:
    Sim* sim_;
    Rng* rng_;
};
