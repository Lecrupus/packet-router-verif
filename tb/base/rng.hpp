// rng.hpp -- the single source of randomness (R-TB-02) and SV-style weighted dist.
#pragma once
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// Wraps one std::mt19937_64. Only raw engine output is used (no std::*_distribution,
// whose results differ between standard libraries), so a seed reproduces bit-for-bit.
class Rng {
public:
    explicit Rng(uint64_t seed = 1) : gen_(seed) {}
    void seed(uint64_t s) { gen_.seed(s); }

    uint64_t u64() { return gen_(); }
    uint32_t u32() { return static_cast<uint32_t>(gen_() >> 32); }
    // Inclusive range [lo, hi]
    int range(int lo, int hi) {
        if (hi <= lo) return lo;
        const uint64_t span = static_cast<uint64_t>(hi - lo) + 1;
        return lo + static_cast<int>(gen_() % span);
    }
    double uniform() { return static_cast<double>(gen_() >> 11) * (1.0 / 9007199254740992.0); }
    bool chance(double p) { return p > 0.0 && uniform() < p; }

private:
    std::mt19937_64 gen_;
};

// C++ equivalent of an SV `dist` block. Each bucket [lo:hi] gets `weight` in total,
// spread uniformly over its values (SV `:/`); a single value behaves like `:=`.
class WeightedDist {
public:
    struct Bucket {
        int lo, hi;
        double weight;
    };

    WeightedDist() = default;
    WeightedDist(std::initializer_list<Bucket> b) : buckets_(b) {}

    WeightedDist& add(int lo, int hi, double weight) {
        buckets_.push_back({lo, hi, weight});
        return *this;
    }
    static WeightedDist constant(int v) { return WeightedDist{{v, v, 1.0}}; }

    int pick(Rng& rng) const {
        double total = 0;
        for (const auto& b : buckets_) total += b.weight;
        if (buckets_.empty() || total <= 0) return 0;
        double r = rng.uniform() * total;
        for (const auto& b : buckets_) {
            if (r < b.weight) return rng.range(b.lo, b.hi);
            r -= b.weight;
        }
        const auto& last = buckets_.back();
        return rng.range(last.lo, last.hi);
    }

    std::string to_string() const {
        std::ostringstream os;
        os << "{";
        for (size_t i = 0; i < buckets_.size(); ++i) {
            const auto& b = buckets_[i];
            if (i) os << ", ";
            if (b.lo == b.hi) os << b.lo << ":=" << b.weight;
            else os << "[" << b.lo << ":" << b.hi << "]:/" << b.weight;
        }
        os << "}";
        return os.str();
    }

private:
    std::vector<Bucket> buckets_;
};
