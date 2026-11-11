// covergroup.hpp -- Covergroup (SV `covergroup`): owns coverpoints and crosses.
// Group % = average of its items (weight-0 coverpoints excluded), spec 8.3.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "coverage/cross.hpp"

class Covergroup {
public:
    explicit Covergroup(std::string name) : name_(std::move(name)) {}

    Coverpoint& coverpoint(const std::string& n, int at_least = 1) {
        auto cp = std::make_unique<Coverpoint>(n, at_least);
        Coverpoint& ref = *cp;
        items_.push_back(std::move(cp));
        return ref;
    }
    Cross& cross(const std::string& n, const Coverpoint& a, const Coverpoint& b, int at_least = 1) {
        auto cr = std::make_unique<Cross>(n, &a, &b, at_least);
        Cross& ref = *cr;
        items_.push_back(std::move(cr));
        return ref;
    }

    double percent() const {
        double sum = 0, n = 0;
        for (const auto& it : items_) {
            if (it->weight() <= 0) continue;
            sum += it->percent() * it->weight();
            n += it->weight();
        }
        return n > 0 ? sum / n : 100.0;
    }
    const std::string& name() const { return name_; }

    void to_json(JsonWriter& w) const {
        w.begin_obj();
        w.key("percent").val(percent());
        w.key("items").begin_obj();
        for (const auto& it : items_) {
            if (it->weight() <= 0) continue;
            w.key(it->name());
            it->to_json(w);
        }
        w.end_obj();
        w.end_obj();
    }

private:
    std::string name_;
    std::vector<std::unique_ptr<CoverItem>> items_;
};
