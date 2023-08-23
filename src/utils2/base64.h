#pragma once

#include <cstdint>
#include <cstddef>

namespace gyverhub {
    char toBase64(uint8_t n);
    uint8_t fromBase64(char b);

    size_t base64EncodedLength(size_t len);
    size_t base64DecodedLength(const char *data, size_t len);

    uint8_t *base64Decode(const char *data, size_t len, size_t &out_len);
    char *base64Encode(const uint8_t *data, size_t len, bool pgm, size_t &out_len);
}
