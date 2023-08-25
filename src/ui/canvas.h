#pragma once
#include "macro.hpp"
#include "utils2/json.h"

// HTML Canvas API
// https://www.w3schools.com/tags/ref_canvas.asp
// https://processing.org/reference/

namespace gyverhub {
    enum class EllipseMode {
        CENTER,
        CORNER
    };

    enum class RectMode {
        CORNER,
        CORNERS,
        CENTER,
        RADIUS
    };

    enum class LineCap {
        BUTT = 0,
        ROUND = 1,
        SQUARE = 2
    };
    
    enum class LineJoin {
        BEVEL = 4,
        MITER = 5,
        ROUND = 1
    };

    enum class TextAlign {
        START = 6,
        END = 7,
        CENTER = 8,
        LEFT = 9,
        RIGHT = 10
    };

    enum class TextBaseline {
        ALPHABETIC = 11,
        TOP = 12,
        HANGING = 13,
        MIDDLE = 14,
        IDEOGRAPHIC = 15,
        BOTTOM = 16,
    };

    enum class GlobalCompositeOperation  {
        SRC_OVER = 17,
        SRC_ATOP = 18,
        SRC_IN = 19,
        SRC_OUT = 20,
        DST_OVER = 21,
        DST_ATOP = 22,
        DST_IN = 23,
        DST_OUT = 24,
        LIGHTER = 25,
        COPY = 26,
        XOR = 27
    };

    class Canvas {
    private:
        Json* ps = nullptr;
        bool first = true;
        char* fontName;
        int fontSize = 20;
        EllipseMode _ellipseMode = EllipseMode::CENTER;
        RectMode _rectMode = RectMode::CORNER;
        bool enableStroke = true;
        bool enableFill = true;

        void command(int cmd, int num, const char *text, ...) {
            if (!ps) return;

            if (first) first = false;
            else *ps += ',';
            *ps += '"';
            *ps += cmd;

            if (num > 0) {
                *ps += ':';

                if (text) {
                    num--;
                    *ps += '\'';
                    if (ps) ps->appendEscaped(text);
                    *ps += '\'';
                    if (num > 0) *ps += ',';
                }

                va_list valist;
                va_start(valist, num);
                for (int i = 0; i < num; i++) {
                    *ps += va_arg(valist, int);
                    if (i < num - 1) *ps += ',';
                }
                va_end(valist);
            }
            
            *ps += '"';
        }

        static inline uint32_t color(uint32_t hex, uint8_t a = 255) {
            return ((uint32_t)hex << 8) | a;
        }

    public:
        Canvas() {
            ps = &buf;
            fontName = strdup("Arial");
        }

        ~Canvas() {
            if (fontName != nullptr) free(fontName);
        }

        // буфер
        Json buf;

        // подключить внешний буфер
        void extBuffer(Json* sptr) {
            ps = sptr;
        }

        // очистить буфер
        void clearBuffer() {
            first = true;
            if (ps) ps->clear();
        }

        // добавить строку кода на js
        void custom(const String& s) {
            if (!ps) return;
            if (first) first = false;
            else *ps += ',';
            *ps += '"';
            ps->appendEscaped(s.c_str());
            *ps += '"';
        }
        void custom(FSTR s) {
            if (!ps) return;
            if (first) first = false;
            else *ps += ',';
            *ps += '"';
            ps->appendEscaped(s);
            *ps += '"';
        }

        // =====================================================
        // =============== PROCESSING-LIKE API =================
        // =====================================================

        // =================== BACKGROUND ======================
        // очистить полотно
        void clear() {
            clearRect(0, 0, -1, -1);
            beginPath();
        }

        // залить полотно установленным в fill() цветом
        void background() {
            fillRect(0, 0, -1, -1);
        }

        // залить полотно указанным цветом (цвет, прозрачность)
        void background(uint32_t hex, uint8_t a = 255) {
            fillStyle(hex, a);
            background();
        }

        // ======================== FILL =======================
        // выбрать цвет заливки (цвет, прозрачность)
        void fill(uint32_t hex, uint8_t a = 255) {
            fillStyle(hex, a);
            enableFill = true;
        }

        // отключить заливку
        void noFill() {
            enableFill = false;
        }

        // ===================== STROKE ====================
        // выбрать цвет обводки (цвет, прозрачность)
        void stroke(uint32_t hex, uint8_t a = 255) {
            strokeStyle(hex, a);
            enableStroke = true;
        }

        // отключить обводку
        void noStroke() {
            enableStroke = false;
        }

        // толщина обводки, px
        void strokeWeight(int v) {
            lineWidth(v);
        }

        // соединение линий: CV_MITER (умолч), CV_BEVEL, CV_ROUND
        void strokeJoin(LineJoin v) {
            lineJoin(v);
        }

        // края линий: CV_PROJECT (умолч), CV_ROUND, CV_SQUARE
        void strokeCap(LineCap v) {
            lineCap(v);
        }

        // ===================== PRIMITIVES ====================
        // окружность (x, y, радиус), px
        void circle(int x, int y, int r) {
            beginPath();
            switch (_ellipseMode) {
                case EllipseMode::CORNER:
                    arc(x + r, y + r, r);
                    break;
                case EllipseMode::CENTER:
                    arc(x, y, r);
                    break;
            }
            if (enableStroke) stroke();
            if (enableFill) fill();
        }

        // линия (координаты начала и конца)
        void line(int x1, int y1, int x2, int y2) {
            beginPath();
            moveTo(x1, y1);
            lineTo(x2, y2);
            stroke();
        }

        // точка
        void point(int x, int y) {
            beginPath();
            fillRect(x, y, 1, 1);
        }

        // четырёхугольник (координаты углов)
        void quadrangle(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
            beginPath();
            moveTo(x1, y1);
            lineTo(x2, y2);
            lineTo(x3, y3);
            lineTo(x4, y4);
            closePath();
            if (enableStroke) stroke();
            if (enableFill) fill();
        }

        // треугольник (координаты углов)
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
            beginPath();
            moveTo(x1, y1);
            lineTo(x2, y2);
            lineTo(x3, y3);
            closePath();
            if (enableStroke) stroke();
            if (enableFill) fill();
        }

        // прямоугольник
        void rect(int x, int y, int w, int h, int tl = -1, int tr = -1, int br = 0, int bl = 0) {
            beginPath();
            int X = x, Y = y, W = w, H = h;
            switch (_rectMode) {
                case RectMode::CORNER:
                    break;
                case RectMode::CORNERS:
                    W = w - x;
                    H = h - y;
                    break;
                case RectMode::CENTER:
                    X = x - w / 2;
                    Y = y - h / 2;
                    break;
                case RectMode::RADIUS:
                    X = x - w;
                    Y = y - h;
                    W = w * 2;
                    H = h * 2;
                    break;
                default:
                    break;
            }
            if (tl < 0) drawRect(X, Y, W, H);
            else if (tr < 0) roundRect(X, Y, W, H, tl);
            else roundRect(X, Y, W, H, tl, tr, br, bl);
            if (enableStroke) stroke();
            if (enableFill) fill();
        }

        // квадрат
        void square(int x, int y, int w) {
            rect(x, y, w, w);
        }

        // режим окружности: CV_CENTER (умолч), CV_CORNER
        void ellipseMode(EllipseMode mode) {
            _ellipseMode = mode;
        }

        // режим прямоугольника: CV_CORNER (умолч), CV_CORNERS, CV_CENTER, CV_RADIUS
        void rectMode(RectMode mode) {
            _rectMode = mode;
        }

        // ======================= TEXT ========================
        // размер шрифта, px
        void textSize(int size) {
            textFont(nullptr, size);
        }

        // шрифт
        void textFont(const char* name, int size = -1) {
            if (name != nullptr) {
                if (fontName != nullptr)
                    free(fontName);
                fontName = strdup(name);
            }
            if (size >= 0) fontSize = size;

            size_t len = strlen(fontName) + 9;
            char *buf = (char*) malloc(len);
            buf[0] = '\0';
            itoa(fontSize, buf, 10);
            strcat(buf, "px ");
            strcat(buf, fontName);

            font(buf);
            free(buf);
        }

        // вывести текст, опционально макс длина
        void text(const String& text, int x, int y, int w = 0) {
            if (enableStroke) strokeText(text, x, y, w);
            if (enableFill) fillText(text, x, y, w);
        }

        // выравнивание текста
        // CV_LEFT, CV_CENTER, CV_RIGHT
        // TXT_TOP, TXT_BOTTOM, TXT_CENTER, TXT_BASELINE
        void textAlign(TextAlign h, TextBaseline v) {
            textAlign(h);
            textBaseline(v);
        }

        // сохранить конфигурацию полотна
        void push() {
            save();
        }

        // восстановить конфигурацию полотна
        void pop() {
            restore();
        }

        // ======================================================
        // ================== HTML CANVAS API ===================
        // ======================================================

        // цвет заполнения
        void fillStyle(uint32_t hex, uint8_t a = 255) {
            command(0, 1, nullptr, color(hex, a));
        }

        // цвет обводки
        void strokeStyle(uint32_t hex, uint8_t a = 255) {
            command(1, 1, nullptr, color(hex, a));
        }

        // цвет тени
        void shadowColor(uint32_t hex, uint8_t a = 255) {
            command(2, 1, nullptr, color(hex, a));
        }

        // размытость тени, px
        void shadowBlur(int v) {
            command(3, 1, nullptr, v);
        }

        // отступ тени, px
        void shadowOffsetX(int v) {
            command(4, 1, nullptr, v);
        }

        // отступ тени, px
        void shadowOffsetY(int v) {
            command(5, 1, nullptr, v);
        }

        // ширина линий, px
        void lineWidth(int v) {
            command(6, 1, nullptr, v);
        }

        // длина соединения CV_MITER, px
        // https://www.w3schools.com/tags/canvas_miterlimit.asp
        void miterLimit(int v) {
            command(7, 1, nullptr, v);
        }

        // шрифт: "30px Arial"
        // https://www.w3schools.com/tags/canvas_font.asp
        void font(const String& v) {
            command(8, 1, v.c_str());
        }
        void font(const char *v) {
            command(8, 1, v);
        }

        // выравнивание текста: CV_START (умолч), CV_END, CV_CENTER, CV_LEFT, CV_RIGHT
        // https://www.w3schools.com/tags/canvas_textalign.asp
        void textAlign(TextAlign v) {
            command(9, 1, nullptr, v);
        }

        // позиция текста: CV_ALPHABETIC (умолч), CV_TOP, CV_HANGING, CV_MIDDLE, CV_IDEOGRAPHIC, CV_BOTTOM
        // https://www.w3schools.com/tags/canvas_textbaseline.asp
        void textBaseline(TextBaseline v) {
            command(10, 1, nullptr, v);
        }

        // края линий: CV_BUTT (умолч), CV_ROUND, CV_SQUARE
        // https://www.w3schools.com/tags/canvas_linecap.asp
        void lineCap(LineCap v) {
            command(11, 1, nullptr, v);
        }

        // соединение линий: CV_MITER (умолч), CV_BEVEL, CV_ROUND
        // https://www.w3schools.com/tags/canvas_linejoin.asp
        void lineJoin(LineJoin v) {
            command(12, 1, nullptr, v);
        }

        // тип наложения графики: CV_SRC_OVER (умолч), CV_SRC_ATOP, CV_SRC_IN, CV_SRC_OUT, CV_DST_OVER, CV_DST_ATOP, CV_DST_IN, CV_DST_OUT, CV_LIGHTER, CV_COPY, CV_XOR
        // https://www.w3schools.com/tags/canvas_globalcompositeoperation.asp
        void globalCompositeOperation(GlobalCompositeOperation v) {
            command(13, 1, nullptr, v);
        }

        // прозрачность рисовки, 0.0-1.0
        void globalAlpha(float v) {
            command(14, 1, nullptr, v);
        }

        // масштабировать область рисования
        // https://www.w3schools.com/tags/canvas_scale.asp
        void scale(int sw, int sh) {
            command(14, 2, nullptr, sw, sh);
        }

        // вращать область рисования (в радианах)
        // https://www.w3schools.com/tags/canvas_rotate.asp
        void rotate(float v) {
            command(16, 1, nullptr, v);
        }

        // прямоугольник
        void drawRect(int x, int y, int w, int h) {
            command(17, 4, nullptr, x, y, w, h);
        }

        // закрашенный прямоугольник
        void fillRect(int x, int y, int w, int h) {
            command(18, 4, nullptr, x, y, w, h);
        }

        // обведённый прямоугольник
        void strokeRect(int x, int y, int w, int h) {
            command(19, 4, nullptr, x, y, w, h);
        }

        // очистить область
        void clearRect(int x, int y, int w, int h) {
            command(20, 4, nullptr, x, y, w, h);
        }

        // переместить курсор
        void moveTo(int x, int y) {
            command(21, 2, nullptr, x, y);
        }

        // нарисовать линию от курсора
        void lineTo(int x, int y) {
            command(22, 2, nullptr, x, y);
        }

        // провести кривую
        // https://www.w3schools.com/tags/canvas_quadraticcurveto.asp
        void quadraticCurveTo(int cpx, int cpy, int x, int y) {
            command(23, 4, nullptr, cpx, cpy, x, y);
        }

        // провести кривую Безье
        // https://www.w3schools.com/tags/canvas_beziercurveto.asp
        void bezierCurveTo(int cp1x, int cp1y, int cp2x, int cp2y, int x, int y) {
            command(24, 6, nullptr, cp1x, cp1y, cp2x, cp2y, x, y);
        }

        // перемещать область рисования
        // https://www.w3schools.com/tags/canvas_translate.asp
        void translate(int x, int y) {
            command(25, 2, nullptr, x, y);
        }

        // скруглить
        // https://www.w3schools.com/tags/canvas_arcto.asp
        void arcTo(int x1, int y1, int x2, int y2, int r) {
            command(26, 5, nullptr, x1, y1, x2, y2, r);
        }

        // провести дугу (радианы)
        // https://www.w3schools.com/tags/canvas_arc.asp
        void arc(int x, int y, int r, float sa = 0, float ea = TWO_PI, bool ccw = 0) {
            command(27, 6, nullptr, x, y, r, sa, ea, ccw);  // TODO check
        }

        // вывести закрашенный текст, опционально макс. длина
        void fillText(const String& text, int x, int y, int w = 0) {
            command(28, 4, text.c_str(), x, y, w);
        }

        // вывести обведённый текст, опционально макс. длина
        void strokeText(const String& text, int x, int y, int w = 0) {
            command(29, 4, text.c_str(), x, y, w);
        }

        // вывести картинку
        // https://www.w3schools.com/tags/canvas_drawimage.asp
        void drawImage(const String& img, int x, int y) {
            command(30, 3, img.c_str(), x, y);
        }
        void drawImage(const String& img, int x, int y, int w, int h) {
            command(30, 5, img.c_str(), x, y, w, h);
        }
        void drawImage(const String& img, int sx, int sy, int sw, int sh, int x, int y, int w, int h) {
            command(30, 9, img.c_str(), sx, sy, sw, sh, x, y, w, h);
        }

        // скруглённый прямоугольник
        void roundRect(int x, int y, int w, int h, int tl = 0, int tr = -1, int br = -1, int bl = -1) {
            if (tr < 0) command(31, 5, nullptr, x, y, w, h, tl);
            else if (br < 0) command(31, 6, nullptr, x, y, w, h, tl, tr);
            else command(31, 8, nullptr, x, y, w, h, tl, tr, br, bl);
        }

        // залить
        void fill() {
            command(32, 0, nullptr);
        }

        // обвести
        void stroke() {
            command(33, 0, nullptr);
        }

        // начать путь
        void beginPath() {
            command(34, 0, nullptr);
        }

        // завершить путь (провести линию на начало)
        void closePath() {
            command(35, 0, nullptr);
        }
        // ограничить область рисования
        // https://www.w3schools.com/tags/canvas_clip.asp
        void clip() {
            command(36, 0, nullptr);
        }

        // сохранить конфигурацию полотна
        void save() {
            command(37, 0, nullptr);
        }

        // восстановить конфигурацию полотна
        void restore() {
            command(38, 0, nullptr);
        }
    };
}
