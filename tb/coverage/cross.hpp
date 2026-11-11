// cross.hpp -- Cross (SV `cross`): every pair of bins from two coverpoints, minus
// ignored pairs. Bins are named "<a bin>,<b bin>".
#pragma once
#include <set>
#include <utility>
#include <vector>

#include "coverage/coverpoint.hpp"

class Cross : public CoverItem {
public:
    Cross(std::string name, const Coverpoint* a, const Coverpoint* b, int at_least = 1)
        : name_(std::move(name)), a_(a), b_(b), at_least_(at_least), hits_(a->size() * b->size(), 0) {}

    Cross& ignore_pair(int ia, int ib) {
        ignored_.insert({ia, ib});
        return *this;
    }

    // Sample with raw values; each is mapped through its coverpoint's bins.
    void sample(int64_t va, int64_t vb) {
        int ia = a_->index_of(va), ib = b_->index_of(vb);
        if (ia < 0 || ib < 0 || ignored_.count({ia, ib})) return;
        ++hits_[static_cast<size_t>(ia) * b_->size() + ib];
    }

    const std::string& name() const override { return name_; }
    size_t total() const override { return valid_pairs() - ignored_.size(); }
    size_t covered() const override {
        size_t n = 0;
        for_each([&](int ia, int ib, uint64_t h) {
            (void)ia; (void)ib;
            n += h >= static_cast<uint64_t>(at_least_);
        });
        return n;
    }

    void to_json(JsonWriter& w) const override {
        w.begin_obj();
        w.key("type").val("cross");
        w.key("at_least").val(at_least_);
        w.key("axes").begin_arr().val(a_->name()).val(b_->name()).end_arr();
        w.key("bins").begin_obj();
        for_each([&](int ia, int ib, uint64_t h) { w.key(pair_name(ia, ib)).val(h); });
        w.end_obj();
        w.key("ignore").begin_arr();
        for (const auto& p : ignored_) w.val(pair_name(p.first, p.second));
        w.end_arr();
        w.end_obj();
    }

private:
    size_t valid_pairs() const {
        size_t na = 0, nb = 0;
        for (const auto& b : a_->bins()) na += !b.ignore;
        for (const auto& b : b_->bins()) nb += !b.ignore;
        return na * nb;
    }
    template <class F>
    void for_each(F f) const {
        for (size_t ia = 0; ia < a_->size(); ++ia) {
            if (a_->bins()[ia].ignore) continue;
            for (size_t ib = 0; ib < b_->size(); ++ib) {
                if (b_->bins()[ib].ignore || ignored_.count({static_cast<int>(ia), static_cast<int>(ib)})) continue;
                f(static_cast<int>(ia), static_cast<int>(ib), hits_[ia * b_->size() + ib]);
            }
        }
    }
    std::string pair_name(int ia, int ib) const { return a_->bins()[ia].name + "," + b_->bins()[ib].name; }

    std::string name_;
    const Coverpoint* a_;
    const Coverpoint* b_;
    int at_least_;
    std::vector<uint64_t> hits_;
    std::set<std::pair<int, int>> ignored_;
};
