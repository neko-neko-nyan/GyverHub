#pragma once
#include <Arduino.h>
#include "macro.hpp"

#if GHI_ESP_BUILD
#include <bitset>
#else
namespace std {
    template<size_t N>
    class bitset {
    public:
        bitset(unsigned long value) {}

        unsigned long long to_ullong() const noexcept {
            return 0;
        }
    };
}
#endif

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