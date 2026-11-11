// registry.hpp -- TestRegistry (a simplified uvm_factory): tests register by name and
// +TEST=<name> creates one (R-TB-07). Templated on the context the tests are built with
// so the framework stays independent of the router.
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/test.hpp"

template <class Ctx>
class TestRegistry {
public:
    using Factory = std::function<std::unique_ptr<Test>(Ctx&)>;

    static TestRegistry& inst() {
        static TestRegistry r;
        return r;
    }
    void add(const std::string& name, Factory f) { factories_[name] = std::move(f); }
    bool has(const std::string& name) const { return factories_.count(name) != 0; }
    std::unique_ptr<Test> create(const std::string& name, Ctx& ctx) const {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : it->second(ctx);
    }
    std::vector<std::string> names() const {
        std::vector<std::string> v;
        for (const auto& kv : factories_) v.push_back(kv.first);
        return v;
    }

private:
    std::map<std::string, Factory> factories_;  // ordered: stable listing
};

template <class Ctx, class T>
struct TestRegistrar {
    explicit TestRegistrar(const std::string& name) {
        TestRegistry<Ctx>::inst().add(name, [name](Ctx& c) { return std::make_unique<T>(name, c); });
    }
};
