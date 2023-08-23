#pragma once
#include <Arduino.h>

#include "../config.hpp"
#include "../macro.hpp"

#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
#include <FS.h>
#if (GH_FS == LittleFS)
#include <LittleFS.h>
#elif (GH_FS == SPIFFS)
#include <SPIFFS.h>
#endif
#endif

#ifdef ESP8266
#else
#ifndef GH_NO_OTA
#include <Update.h>
#endif
#endif
#endif

extern String _GH_empty_str;

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
void GH_showFiles(String& answ, const String& path, GH_UNUSED uint8_t levels = 0, uint16_t* count = nullptr);
