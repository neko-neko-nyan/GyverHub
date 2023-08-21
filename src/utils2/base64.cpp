#include "base64.h"
#include <Arduino.h>


static const char _b64chars[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const uint8_t _b64index[] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};


char gyverhub::toBase64(uint8_t n) {
    return pgm_read_byte(_b64chars + n);
}

uint8_t gyverhub::fromBase64(char b) {
    return pgm_read_byte(_b64index + b);
}


size_t gyverhub::base64EncodedLength(size_t len) {
    return ((len + 2) / 3) * 4;
}

size_t gyverhub::base64DecodedLength(const char *data, size_t len) {
    if (len < 4) return 0;

    int pad = 0;
    if (data[len - 1] == '=') {
        if (data[len - 2] == '=') pad = 2;
        else pad = 1;
    }
    
    return ((len + 3) / 4) * 3 - pad;
}

uint8_t *gyverhub::base64Decode(const char *data, size_t len, size_t &out_len) {
    out_len = base64DecodedLength(data, len);
    uint8_t *res = (uint8_t*) malloc(out_len);
    if (res == nullptr) return nullptr;
    

    int val = 0, valb = -8, idx = 0;
    for (uint16_t i = 0; i < len && data[i] != '='; i++) {
        uint8_t b = fromBase64(data[i]);
        val = (val << 6) + b;
        valb += 6;
        if (valb >= 0) {
            res[idx++] = (uint8_t)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }

    return res;
}

char *gyverhub::base64Encode(const uint8_t *data, size_t len, bool pgm, size_t &out_len) {
    out_len = base64EncodedLength(len);
    char *res = (char*) malloc(out_len);
    if (res == nullptr) return nullptr;

    size_t out_i = 0;
    uint16_t val = 0;
    int valb = -6;
    for (size_t in_i = 0; in_i < len; in_i++) {
        uint8_t b = pgm ? pgm_read_byte(&data[in_i]) : data[in_i];
        val = (val << 8) + b;
        valb += 8;
        while (valb >= 0) {
            char c = toBase64((val >> valb) & 0x3F);
            res[out_i++] = c;
        }
    }

    if (valb > -6) {
        char c = toBase64(((val << 8) >> (valb + 8)) & 0x3F);
        res[out_i++] = c;
    }

    while (out_i % 4 != 0)
        res[out_i++] = '=';

    return res;
}