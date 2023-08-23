#pragma once
#include <Arduino.h>
#include <bitset>

template<size_t N>
class GHflags : public std::bitset<N> {
public:
    GHflags() : std::bitset<N>() {}
    GHflags(const GHflags& f) = default;
    GHflags(uint16_t nflags) : std::bitset<N>((unsigned long) nflags) {}

    constexpr bool get(size_t idx) const noexcept {
        return this->test(idx);
    }

    String toString() {
        String s;
        s.reserve(N);
        for (size_t i = 0; i < N; i++) s += this->test(i);
        return s;
    }
};