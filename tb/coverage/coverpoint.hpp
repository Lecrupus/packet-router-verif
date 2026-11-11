// coverpoint.hpp -- Bin (SV `bins`) and Coverpoint (SV `coverpoint`).
// Reusable, not router-specific. Percent rules match spec 8.3:
//   a bin is covered when hits >= at_least; item % = covered / (total - ignored).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "base/json.hpp"

struct Bin {
    std::string name;
    int64_t lo, hi;      // inclusive value range
    uint64_t hits = 0;
    bool ignore = false;  // SV ignore_bins: matched values are dropped, bin not counted
};

class CoverItem {
public:
    virtual ~CoverItem() = default;
    virtual const std::string& name() const = 0;
    virtual size_t total() const = 0;     // bins that count (excludes ignored)
    virtual size_t covered() const = 0;
    virtual double weight() const { return 1.0; }
    double percent() const { return total() ? 100.0 * static_cast<double>(covered()) / total() : 100.0; }
    virtual void to_json(JsonWriter& w) const = 0;
};

class Coverpoint : public CoverItem {
public:
    explicit Coverpoint(std::string name, int at_least = 1) : name_(std::move(name)), at_least_(at_least) {}

    // Builders
    Coverpoint& bin(const std::string& n, int64_t lo, int64_t hi) {
        bins_.push_back({n, lo, hi});
        return *this;
    }
    Coverpoint& bin(const std::string& n, int64_t v) { return bin(n, v, v); }
    Coverpoint& ignore_bin(const std::string& n, int64_t lo, int64_t hi) {
        bins_.push_back({n, lo, hi, 0, true});
        return *this;
    }
    // One bin per value lo..hi, named by the number (SV `bins b[] = {[lo:hi]}`)
    Coverpoint& values(int64_t lo, int64_t hi) {
        for (int64_t v = lo; v <= hi; ++v) bin(std::to_string(v), v);
        return *this;
    }
    // weight 0: used only to define a cross's axes, not reported as an item (SV option.weight)
    Coverpoint& set_weight(double w) {
        weight_ = w;
        return *this;
    }

    // Index of the first bin containing v, or -1 (no bin / ignored)
    int index_of(int64_t v) const {
        for (size_t i = 0; i < bins_.size(); ++i)
            if (v >= bins_[i].lo && v <= bins_[i].hi) return bins_[i].ignore ? -1 : static_cast<int>(i);
        return -1;
    }
    void sample(int64_t v) {
        int i = index_of(v);
        if (i >= 0) ++bins_[i].hits;
    }

    const std::string& name() const override { return name_; }
    double weight() const override { return weight_; }
    size_t total() const override {
        size_t n = 0;
        for (const auto& b : bins_) n += !b.ignore;
        return n;
    }
    size_t covered() const override {
        size_t n = 0;
        for (const auto& b : bins_) n += (!b.ignore && b.hits >= static_cast<uint64_t>(at_least_));
        return n;
    }
    const std::vector<Bin>& bins() const { return bins_; }
    size_t size() const { return bins_.size(); }
    int at_least() const { return at_least_; }

    void to_json(JsonWriter& w) const override {
        w.begin_obj();
        w.key("type").val("coverpoint");
        w.key("at_least").val(at_least_);
        w.key("bins").begin_obj();
        for (const auto& b : bins_)
            if (!b.ignore) w.key(b.name).val(b.hits);
        w.end_obj();
        w.key("ignore").begin_arr();
        for (const auto& b : bins_)
            if (b.ignore) w.val(b.name);
        w.end_arr();
        w.end_obj();
    }

private:
    std::string name_;
    int at_least_;
    double weight_ = 1.0;
    std::vector<Bin> bins_;
};
