#pragma once
#include <stdint.h>

namespace gyverhub {
    enum class Colors : uint32_t {
        RED = 0xcb2839,
        ORANGE = 0xd55f30,
        YELLOW = 0xd69d27,
        GREEN = 0x37A93C,
        MINT = 0x25b18f,
        AQUA = 0x2ba1cd,
        BLUE = 0x297bcd,
        VIOLET = 0x825ae7,
        PINK = 0xc8589a,
        UNSET = 0xffffffff,
    };

    class Color {
        union rgb2hex {
            struct {
                uint8_t b;
                uint8_t g;
                uint8_t r;
            } c;
            uint32_t h;
        };

        uint8_t r = 0, g = 0, b = 0;
        
        constexpr Color(rgb2hex x) : r(x.c.r), g(x.c.g), b(x.c.b) {}

    public:
        constexpr Color() {}
        constexpr Color(const Color& col) = default;
        constexpr Color(uint8_t gray) : r(gray), g(gray), b(gray) {}
        constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
        constexpr Color(Colors col) : Color(rgb2hex {.h = static_cast<uint32_t>(col)}) {}

        static Color fromHsv(uint8_t h, uint8_t s, uint8_t v);

        static constexpr Color fromHex(uint32_t hex) {
            return Color(rgb2hex {.h = hex});
        }

        uint32_t toHex() const {
            return (rgb2hex{.c = {r, g, b}}).h;
        }
        
        bool operator==(const Color &b) const {
            return this->r == b.r && this->g == b.g && this->b == b.b;
        }
        
        bool operator==(const Colors &b) const {
            return *this == Color(b);
        }

        uint8_t getR() const {
            return r;
        }

        uint8_t getG() const {
            return g;
        }

        uint8_t getB() const {
            return b;
        }

        void setR(uint8_t r) {
            this->r = r;
        }

        void setG(uint8_t g) {
            this->g = g;
        }

        void setB(uint8_t b) {
            this->b = b;
        }

        void setHue(uint8_t color);

        Color withHue(uint8_t color) const {
            Color c = *this;
            c.setHue(color);
            return c;
        }
    };
}
