#pragma once
// Deterministic PCG32 RNG. Gameplay never uses std::rand; all randomness
// flows through this so simulations can be replayed (tests, ghost data).
#include <cstdint>
#include <vector>

namespace ink {

class Rng {
public:
    Rng() { reseed(0x9E3779B97F4A7C15ULL); }
    explicit Rng(uint64_t seed) { reseed(seed); }

    void reseed(uint64_t seed) {
        state_ = 0U;
        inc_ = (seed << 1u) | 1u;
        next();
        state_ += 0x853c49e6748fea9bULL;
        next();
    }

    uint32_t next() {
        uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
    }

    double unit() { return (next() >> 8) * (1.0 / 16777216.0); } // [0,1)
    int range(int lo, int hi) { // inclusive
        if (hi <= lo) return lo;
        return lo + next() % static_cast<uint32_t>(hi - lo + 1);
    }
    bool chance(double p) { return unit() < p; }

    template <typename T>
    const T& pick(const std::vector<T>& v) {
        return v[next() % v.size()];
    }

private:
    uint64_t state_ = 0;
    uint64_t inc_ = 1;
};

} // namespace ink
