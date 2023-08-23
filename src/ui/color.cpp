#include "color.h"

gyverhub::Color gyverhub::Color::fromHsv(uint8_t h, uint8_t s, uint8_t v) {
    float R = 0.f, G = 0.f, B = 0.f;

    float H = h / 255.f;
    float S = s / 255.f;
    float V = v / 255.f;

    uint8_t i = H * 6;
    float f = H * 6 - i;
    float p = V * (1 - S);
    float q = V * (1 - f * S);
    float t = V * (1 - (1 - f) * S);

    switch (i) {
        case 0:
            R = V, G = t, B = p;
            break;
        case 1:
            R = q, G = V, B = p;
            break;
        case 2:
            R = p, G = V, B = t;
            break;
        case 3:
            R = p, G = q, B = V;
            break;
        case 4:
            R = t, G = p, B = V;
            break;
        case 5:
            R = V, G = p, B = q;
            break;
    }

    return Color(R * 255, G * 255, B * 255);
}

void gyverhub::Color::setHue(uint8_t color) {
    uint8_t shift;
    if (color > 170) {
        shift = (color - 170) * 3;
        r = shift;
        g = 0;
        b = 255 - shift;
    } else if (color > 85) {
        shift = (color - 85) * 3;
        r = 0;
        g = 255 - shift;
        b = shift;
    } else {
        shift = color * 3;
        r = 255 - shift;
        g = shift;
        b = 0;
    }
}
