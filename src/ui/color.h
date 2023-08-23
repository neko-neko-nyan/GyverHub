#pragma once
#include <cstdint>

namespace gyverhub {
    enum class Colors : uint32_t {
        GH_RED = 0xcb2839,
        GH_ORANGE = 0xd55f30,
        GH_YELLOW = 0xd69d27,
        GH_GREEN = 0x37A93C,
        GH_MINT = 0x25b18f,
        GH_AQUA = 0x2ba1cd,
        GH_BLUE = 0x297bcd,
        GH_VIOLET = 0x825ae7,
        GH_PINK = 0xc8589a,
        GH_DEFAULT = 0xffffffff,
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

        void setG(uint8_t r) {
            this->g = g;
        }

        void setB(uint8_t r) {
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
