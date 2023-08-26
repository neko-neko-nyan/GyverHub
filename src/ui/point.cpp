#include "point.h"
#include <math.h>


int16_t gyverhub::Point::pointsDistance(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    x1 -= x0;
    y1 -= y0;
    return sqrt(x1 * x1 + y1 * y1);
}

bool gyverhub::Point::isPointInRect(int16_t x, int16_t y, int16_t rx, int16_t ry, int16_t w, int16_t h) {
    return (x > rx) && (x < rx + w) && (y > ry) && (y < ry + h);
}

bool gyverhub::Point::isPointInCircle(int16_t x, int16_t y, int16_t cx, int16_t cy, int16_t r) {
    return pointsDistance(x, y, cx, cy) < r;
}
