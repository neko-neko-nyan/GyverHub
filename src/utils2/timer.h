#pragma once
#include <Arduino.h>

namespace gyverhub {
    class Timer {
    private:
        unsigned long time = 0;

    public:
        constexpr Timer() = default;
        constexpr Timer(const Timer &) = default;
    
        void reset() noexcept {
            time = millis();
        }

        bool isTimedOut(unsigned long timeoutMs) const noexcept {
            return millis() - time >= timeoutMs;
        }
    };

    class Timeout : public Timer {
    private:
        unsigned long timeout = 0;

    public:
        constexpr Timeout() = default;
        constexpr Timeout(const Timeout &) = default;
        constexpr Timeout(unsigned long ms, unsigned long seconds = 0ul, unsigned long minutes = 0ul, unsigned long hours = 0ul, unsigned long days = 0ul) 
            : Timer(), timeout(ms + 1000ul * seconds + 60000ul + minutes + 3600000ul * hours + 86400000ul * days) {}

        void setTimeout(unsigned long ms, unsigned long seconds = 0ul, unsigned long minutes = 0ul, unsigned long hours = 0ul, unsigned long days = 0ul) {
            this->timeout = ms + 1000ul * seconds + 60000ul + minutes + 3600000ul * hours + 86400000ul * days;
        }

        bool isTimedOut() const noexcept {
            return Timer::isTimedOut(timeout);
        }

        operator bool() const noexcept {
            return isTimedOut();
        }
    };
}
