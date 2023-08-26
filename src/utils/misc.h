#pragma once
#include <Arduino.h>
#include "macro.hpp"

template <uint8_t SIZE>
struct GHparser {
    GHparser(char* url) {
        int16_t len = strlen(url);
        for (uint8_t i = 0; i < SIZE; i++) {
            char* div = (char*)memchr(url, '/', len);
            str[i] = url;
            size++;
            if (div && i < SIZE - 1) {
                uint8_t divlen = div - url;
                len -= divlen + 1;
                url += divlen + 1;
                *div = 0;
            } else break;
        }
    }
    ~GHparser() {
        for (int i = 1; i < size; i++) {
            *(str[i] - 1) = '/';
        }
    }

    char* str[SIZE] = {};
    uint8_t size = 0;
};

char* GH_splitter(char* list, char div = ',');

void GH_listDir(String& str, const String& path = "/", char div = ',');
void GH_showFiles(String& answ, const String& path, GHI_UNUSED uint8_t levels = 0, uint16_t* count = nullptr);
