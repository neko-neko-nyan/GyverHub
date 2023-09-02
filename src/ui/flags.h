#pragma once
#include <Arduino.h>
#include "macro.hpp"
#include <limits.h>

namespace gyverhub {
    class Flags {
        typedef unsigned long word_t;

        size_t size = 0;
        word_t *value = nullptr;

        static constexpr const size_t bitsPerWord = (CHAR_BIT * sizeof(unsigned long));

        static constexpr size_t wordsCount(size_t bitsCount) noexcept {
            return bitsCount / bitsPerWord + (bitsCount % bitsPerWord == 0 ? 0 : 1);
        }

        static constexpr size_t whichBit(size_t pos) noexcept {
            return pos % bitsPerWord;
        }

        static constexpr word_t maskBit(size_t pos) noexcept {
            return static_cast<word_t>(1) << whichBit(pos);
        }

        static constexpr size_t whichWord(size_t pos) noexcept {
            return pos / bitsPerWord;
        }

        word_t& getWord(size_t pos) noexcept {
            return value[whichWord(pos)];
        }

        constexpr word_t getWord(size_t pos) const noexcept {
            return value[whichWord(pos)];
        }

        void resize(size_t newSize) noexcept {
            if (size == newSize) return;
            if (newSize == 0) {
                free(value);
                size = 0;
                value = nullptr;
            } else if (value == nullptr) {
                void *mem = calloc(1, sizeof(word_t) * wordsCount(newSize));
                if (mem == nullptr) return;
                value = reinterpret_cast<word_t*>(mem);
                size = newSize;

            } else {
                size_t newWords = wordsCount(newSize);
                void *mem = realloc(value, sizeof(word_t) * newWords);
                if (mem == nullptr) return;
                value = reinterpret_cast<word_t*>(mem);

                size_t words = wordsCount(size);
                if (newWords > words)
                    memset(value + words, 0, newWords - words);
                size = newSize;
            }
        }

    public:
        Flags() = default;
        Flags(const Flags& f) = default;
        Flags(const char *flags) {
            set(flags);
        }

        constexpr bool get(size_t pos) const noexcept {
            return pos < size ? getUnchecked(pos) : false;
        }

        void set(size_t pos, bool val = true) noexcept {
            if (pos < size) setUnchecked(pos, val);
        }

        void reset(size_t pos) noexcept {
            if (pos >= size) return;
            getWord(pos) &= ~maskBit(pos);
        }

        bool flip(size_t pos) noexcept {
            if (pos >= size) return 0;
            word_t &w = getWord(pos);
            word_t m = maskBit(pos);
            w ^= m;
            return (w & m) == static_cast<word_t>(0);
        }

        constexpr size_t length() const noexcept {
            return size;
        }

        constexpr bool getUnchecked(size_t pos) const noexcept {
            return (getWord(pos) & maskBit(pos)) != static_cast<word_t>(0);
        }

        void setUnchecked(size_t pos, bool val = true) noexcept {
            if (val) getWord(pos) |= maskBit(pos);
            else getWord(pos) &= ~maskBit(pos);
        }

        void set(const char *flags) {
            size_t length = 0;
            for (const char *p = flags; *p; p++) {
                if (*p == '0' || *p == '1')
                    length++;
            }
            resize(length);

            length = 0;
            for (const char *p = flags; *p; p++) {
                if (*p == '0')
                    setUnchecked(length++, false);
                if (*p == '1')
                    setUnchecked(length++, true);
            }
        }

        char *toString() {
            char *s = (char*) malloc(size + 1);
            if (s == nullptr) return nullptr;
            for (size_t i = 0; i < size; i++) s[i] = getUnchecked(i) ? '1' : '0';
            s[size] = '\0';
            return s;
        }
    };
}
