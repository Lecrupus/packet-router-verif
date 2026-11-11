// component.hpp -- Component (uvm_component): a named node in the testbench tree that
// owns its children (std::unique_ptr, R-TB-01) and runs phases top-down (R-TB-03):
// build -> connect -> run (cycle loop) -> check -> report.
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Component {
public:
    Component(std::string name, Component* parent) : name_(std::move(name)), parent_(parent) {}
    virtual ~Component() = default;
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    // Phase hooks
    virtual void build() {}
    virtual void connect() {}
    virtual void run_cycle() {}
    virtual void check() {}
    virtual void report() {}

    // Create an owned child: create<T>("name", args...) constructs T("name", this, args...)
    template <class T, class... Args>
    T* create(const std::string& name, Args&&... args) {
        auto p = std::make_unique<T>(name, this, std::forward<Args>(args)...);
        T* raw = p.get();
        children_.push_back(std::move(p));
        return raw;
    }

    // Phase runners: parent first, then children in creation order
    void build_phase() {
        build();
        for (size_t i = 0; i < children_.size(); ++i) children_[i]->build_phase();
    }
    void connect_phase() {
        connect();
        for (auto& c : children_) c->connect_phase();
    }
    void run_cycle_phase() {
        run_cycle();
        for (auto& c : children_) c->run_cycle_phase();
    }
    void check_phase() {
        check();
        for (auto& c : children_) c->check_phase();
    }
    void report_phase() {
        report();
        for (auto& c : children_) c->report_phase();
    }

    const std::string& name() const { return name_; }
    Component* parent() const { return parent_; }
    std::string full_name() const {
        return parent_ ? parent_->full_name() + "." + name_ : name_;
    }
    const std::vector<std::unique_ptr<Component>>& children() const { return children_; }

protected:
    std::string name_;
    Component* parent_;  // non-owning
    std::vector<std::unique_ptr<Component>> children_;
};
