// analysis_port.hpp -- AnalysisPort (uvm_analysis_port) and Subscriber (uvm_subscriber).
// Observer pattern: a publisher never knows who listens, so new subscribers such as
// coverage need no change to monitors or the scoreboard (R-TB-05).
#pragma once
#include <functional>
#include <vector>

#include "base/component.hpp"

template <class T>
class AnalysisPort {
public:
    void connect(std::function<void(const T&)> f) { subs_.push_back(std::move(f)); }
    void write(const T& t) const {
        for (auto& f : subs_) f(t);
    }
    size_t size() const { return subs_.size(); }

private:
    std::vector<std::function<void(const T&)>> subs_;
};

template <class T>
class Subscriber : public Component {
public:
    using Component::Component;
    virtual void write(const T& t) = 0;
    // Convenience: subscribe this object to a port
    void subscribe(AnalysisPort<T>& ap) {
        ap.connect([this](const T& t) { write(t); });
    }
};
