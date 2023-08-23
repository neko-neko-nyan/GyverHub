#pragma once
#include <Arduino.h>

namespace gyverhub {
    class Point {
    public:
        int16_t x = 0;
        int16_t y = 0;

    private:
        bool changed = false;

    public:
        constexpr Point() = default;
        constexpr Point(const Point &) = default;
        constexpr Point(int16_t x, int16_t y, bool changed = false) : x(x), y(y), changed(changed) {}

        inline bool isChanged() const noexcept {
            return changed;
        }

        // расстояние между двумя точками
        static int16_t pointsDistance(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
        
        static inline int16_t pointsDistance(Point p0, int16_t x1, int16_t y1) {
            return pointsDistance(p0.x, p0.y, x1, y1);
        }
        
        static inline int16_t pointsDistance(int16_t x0, int16_t y0, Point p1) {
            return pointsDistance(x0, y0, p1.x, p1.y);
        }
        
        static inline int16_t pointsDistance(Point p0, Point p1) {
            return pointsDistance(p0.x, p0.y, p1.x, p1.y);
        }

        // точка лежит внутри прямоугольника (координаты угла и размеры)

        static bool isPointInRect(int16_t x, int16_t y, int16_t rx, int16_t ry, int16_t w, int16_t h);

        static inline bool isPointInRect(Point p, int16_t rx, int16_t ry, int16_t w, int16_t h){
            return isPointInRect(p.x, p.y, rx, ry, w, h);
        }

        static inline bool isPointInRect(int16_t x, int16_t y, Point r, int16_t w, int16_t h){
            return isPointInRect(x, y, r.x, r.y, w, h);
        }

        static inline bool isPointInRect(Point p, Point r, int16_t w, int16_t h){
            return isPointInRect(p.x, p.y, r.x, r.y, w, h);
        }

        // точка лежит внутри окружности (координаты центра и радиус)

        static bool isPointInCircle(int16_t x, int16_t y, int16_t cx, int16_t cy, int16_t r);

        static inline bool isPointInCircle(Point p, int16_t cx, int16_t cy, int16_t r) {
            return isPointInCircle(p.x, p.y, cx, cy, r);
        }

        static inline bool isPointInCircle(int16_t x, int16_t y, Point c, int16_t r) {
            return isPointInCircle(x, y, c.x, c.y, r);
        }

        static inline bool isPointInCircle(Point p, Point c, int16_t r) {
            return isPointInCircle(p.x, p.y, c.x, c.y, r);
        }


        // расстояние до точки

        inline int16_t distanceTo(int16_t x1, int16_t y1) {
            return pointsDistance(x, y, x1, y1);
        }

        inline int16_t distanceTo(Point p) {
            return pointsDistance(x, y, p.x, p.y);
        }

        // точка лежит внутри прямоугольника
        
        inline bool isInRect(int16_t rx, int16_t ry, int16_t w, int16_t h) {
            return isPointInRect(x, y, rx, ry, w, h);
        }
        
        inline bool isInRect(Point r, int16_t w, int16_t h) {
            return isPointInRect(x, y, r.x, r.y, w, h);
        }

        // точка лежит внутри окружности

        inline bool isInCircle(int16_t cx, int16_t cy, int16_t r) {
            return isPointInCircle(x, y, cx, cy, r);
        }

        inline bool isInCircle(Point c, int16_t r) {
            return isPointInCircle(x, y, c.x, c.y, r);
        }
    };
}


class GHpos {
   public:
    GHpos() {}
    GHpos(int16_t nx, int16_t ny, bool nc = 0) : x(nx), y(ny), _changed(nc) {}

    bool changed() {
        return _changed ? (_changed = 0, 1) : 0;
    }

    // координаты
    int16_t x = 0;
    int16_t y = 0;

    bool _changed = 0;
};
