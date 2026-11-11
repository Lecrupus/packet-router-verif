// monitor.hpp -- Monitor<T> (uvm_monitor). Monitors only read signals (R-TB-04) and
// publish what they see on an analysis port.
#pragma once
#include "base/analysis_port.hpp"
#include "base/component.hpp"

// Anything the scheduler calls in the "sample" slot of a cycle (after inputs settle,
// before the rising edge): monitors, the protocol checker, cycle-level coverage.
class Sampler {
public:
    virtual ~Sampler() = default;
    virtual void sample() = 0;
};

template <class T>
class Monitor : public Component, public Sampler {
public:
    using Component::Component;
    AnalysisPort<T> ap;
};
