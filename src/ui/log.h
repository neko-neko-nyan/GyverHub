#pragma once
#include <Print.h>

#include "utils/misc.h"

class GHlog : public Print {
   public:
    // начать и указать размер буфера
    void begin(int n = 64) {
        end();
        size = n;
        buffer = new char[size];
        clear();
    }

    ~GHlog() {
        end();
    }

    void end() {
        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    virtual size_t write(uint8_t n) {
        if (buffer) {
            _write(n);
        }
        return 1;
    }

    // virtual size_t write(const uint8_t *buffer, size_t size) {

    // }

    void clear() {
        len = head = 0;
        _write('\n');
    }

    void read(String* s, bool esc = false) {
        if (!buffer) return;
        bool start = 0;
        for (uint16_t i = 0; i < len; i++) {
            char c = _read(i);
            if (start) {
                if (esc) {
                    if (c == '\"') *s += '\\';
                }
                *s += c;
            } else if (c == '\n') start = 1;
        }
    }

   private:
    void _write(uint8_t n) {
        if (n == '\r') return;
        if (len < size) len++;
        buffer[head] = n;
        if (++head >= size) head = 0;
    }
    char _read(uint16_t num) {
        return buffer[(len < size) ? num : ((head + num) % size)];
    }

    char* buffer = nullptr;
    uint16_t size = 0;
    uint16_t len = 0;
    uint16_t head = 0;
};