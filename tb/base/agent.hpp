// agent.hpp -- Agent (uvm_agent): groups the components for one interface. Takes a port
// index and an is_active flag (R-TB-06); passive agents build only their monitor.
#pragma once
#include "base/component.hpp"

class Agent : public Component {
public:
    Agent(const std::string& name, Component* parent, int port, bool is_active)
        : Component(name, parent), port_(port), is_active_(is_active) {}
    int port() const { return port_; }
    bool is_active() const { return is_active_; }

protected:
    int port_;
    bool is_active_;
};
