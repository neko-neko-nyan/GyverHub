#pragma once
#include "macro.hpp"
#include "ui/canvas.h"
#include "ui/button.h"
#include "ui/color.h"
#include "ui/log.h"
#include "ui/point.h"
#include "utils/json.h"
#include "hub/client.h"
#include "ui/flags.h"

namespace gyverhub {
    class Builder;
    typedef void (*BuildCallback)(Builder *builder);
    typedef void (*SendCallback)(const String &);

    // тип билда
    enum class BuildType {
        NONE,
        ACTION,  // set command or set topic
        COUNT,  // before ui
        READ,  // read topic
        UI,  // ui
    };
    
    enum DataType {
        GH_NULL,

        GH_STR,
        GH_CSTR,

        GH_BOOL,
        GH_INT8,
        GH_UINT8,
        GH_INT16,
        GH_UINT16,
        GH_INT32,
        GH_UINT32,

        GH_FLOAT,
        GH_DOUBLE,

        GH_COLOR,
        GH_FLAGS,
        GH_POS,
    };

    class Builder {
    private:
        SendCallback sendCallback = nullptr;
        Json* sptr = nullptr;
        GHclient client {};
        BuildType buildType = BuildType::NONE;
        bool mustRefresh = false;
        uint8_t menu = 0;
        uint16_t tab_width = 0;
        uint16_t count = 0;

        // имя компонента
        const char* name = nullptr;

        // значение компонента
        const char* value = nullptr;

        size_t maxChunkSize = 0;
        size_t totalSize = 0;

        Builder(BuildType buildType, const char* name = nullptr, const char* value = nullptr) : buildType(buildType), name(name), value(value) {};

        void _afterComponent() {
            if (buildType == BuildType::COUNT) {
                totalSize += sptr->length();
                sptr->clear();
            } else if (sendCallback && sptr->length() >= maxChunkSize) {
                sendCallback(*sptr);
                sptr->clear();
            }
        }

    public:

        uint8_t currentMenu() {
            return menu;
        }

        void refresh() {
            mustRefresh = true;
        }

        static bool buildSet(BuildCallback cb, const char* name, const char* value, GHclient client){
            Builder b{BuildType::ACTION, name, value};
            b.client = client;
            cb(&b);
            return b.mustRefresh;
        }

        static bool buildRead(BuildCallback cb, gyverhub::Json *answ, const char* name) {
            Builder b{BuildType::READ, name};
            b.sptr = answ;
            cb(&b);
            return b.buildType == BuildType::NONE;
        }

        static size_t buildCount(BuildCallback cb, GHclient client) {
            Builder b{BuildType::COUNT};
            b.client = client;
            gyverhub::Json count;
            b.sptr = &count;
            cb(&b);
            return b.totalSize;
        }

        static void buildUi(BuildCallback cb, gyverhub::Json *answ, GHclient client, size_t maxChunkSize = 0, SendCallback sendCallback = nullptr) {
            Builder b{BuildType::UI};
            b.client = client;
            b.maxChunkSize = maxChunkSize;
            b.sendCallback = sendCallback;
            b.sptr = answ;
            cb(&b);
        }

        // ========================= PRIVATE =========================
    private:
        bool autoNameEq() {
            return name[0] == '_' && name[1] == 'n' && (uint16_t)atoi(name + 2) == count;
        }
        void _nameAuto() {
            count++;
        }
        bool _checkName() {
            if (sptr && buildType == BuildType::READ && autoNameEq()) {
                buildType = BuildType::NONE;
                return true;
            }
            return false;
        }

        void parse(void *var, DataType dtype);

        bool _parse(void *var, DataType dtype) {
            if (sptr && buildType == BuildType::ACTION && autoNameEq()) {
                buildType = BuildType::NONE;
                parse(var, dtype);
                return true;
            }
            return false;
        }
        bool _isUI() {
            return sptr && (buildType == BuildType::UI || buildType == BuildType::COUNT);
        }

        // ================
        void appendObject(void* val, DataType type);

        void _add(VSPTR str, bool fstr = true) {
            if (str) {
                if (fstr) *sptr += (FSTR)str;
                else *sptr += (PGM_P)str;
            }
        }
        void _begin(FSTR type) {
            _add(F("{\"type\":\""));
            *sptr += type;
            _quot();
        }
        void _end() {
            *sptr += '}';
            _afterComponent();
            *sptr += ',';
        }
        void _quot() {
            *sptr += '\"';
        }
        void _tabw() {
            if (tab_width) {
                _add(F(",\"tab_w\":"));
                *sptr += tab_width;
            }
        }

        // ================
        void _value() {
            *sptr += F(",\"value\":");
        }
        void _value(VSPTR value, bool fstr) {
            _value();
            _quot();
            sptr->appendEscaped(value, fstr);  //_add(value, fstr);
            _quot();
        }
        void _name() {
            _add(F(",\"name\":\"_n"));
            *sptr += count;
            _quot();
        }
        void _label(VSPTR label, bool fstr) {
            _add(F(",\"label\":\""));
            sptr->appendEscaped(label, fstr);  //_add(label, fstr);
            _quot();
        }
        void _text() {
            _add(F(",\"text\":"));
        }
        void _text(VSPTR text, bool fstr) {
            _text();
            _quot();
            sptr->appendEscaped(text, fstr);  //_add(text, fstr);
            _quot();
        }

        // ================
        void _color(gyverhub::Color color) {
            if (color == Colors::UNSET) return;
            _add(F(",\"color\":"));
            *sptr += color.toHex();
        }
        void _size(int val) {
            _add(F(",\"size\":"));
            *sptr += val;
        }

        // ================
        void _minv(float val) {
            _add(F(",\"min\":"));
            if (isnan(val)) *sptr += 0;
            else *sptr += val;
        }

        void _maxv(float val) {
            _add(F(",\"max\":"));
            if (isnan(val)) *sptr += 0;
            else *sptr += val;
        }

        void _step(float val) {
            _add(F(",\"step\":"));
            if (isnan(val)) *sptr += 0;
            else {
                if (val < 0.01) *sptr += String(val, 4);
                else *sptr += val;
            }
        }

    public:
        // ========================== WIDGET ==========================
        void BeginRow(int height = 0) {
            if (_isUI()) {
                tab_width = 100;
                _add(F("{\"type\":\"row_b\",\"height\":"));
                *sptr += height;
                _end();
            }
        }
        void BeginWidgets(int height = 0) {
            BeginRow(height);
        }

        void EndRow() {
            tab_width = 0;
            if (_isUI()) {
                _add(F("{\"type\":\"row_e\""));
                _end();
            }
        }
        void EndWidgets() {
            EndRow();
        }

        void WidgetSize(int width) {
            tab_width = width;
        }

        // ========================== DUMMY ===========================
        bool Dummy(void* var = nullptr, DataType type = GH_NULL) {
            _nameAuto();
            if (_checkName()) {
                appendObject(var, type);
            }
            return _parse(var, type);
        }

        // ========================== BUTTON ==========================
        bool Button(gyverhub::Button* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET, int size = 20) {
            return _button(true, F("button"), var, label, color, size);
        }
        bool Button(gyverhub::Button* var, CSREF label, gyverhub::Color color = Colors::UNSET, int size = 20) {
            return _button(false, F("button"), var, label.c_str(), color, size);
        }

        bool ButtonIcon(gyverhub::Button* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET, int size = 50) {
            return _button(true, F("button_i"), var, label, color, size);
        }
        bool ButtonIcon(gyverhub::Button* var, CSREF label, gyverhub::Color color = Colors::UNSET, int size = 50) {
            return _button(false, F("button_i"), var, label.c_str(), color, size);
        }

        bool _button(bool fstr, FSTR tag, gyverhub::Button* var, VSPTR label, gyverhub::Color color, int size) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                _label(label, fstr);
                _color(color);
                _size(size);
                _tabw();
                _end();
            }
            return var && _parse(&var->value, GH_UINT8) && var->clicked();
        }

        // ========================== LABEL ==========================
        void Label(CSREF value = "", FSTR label = nullptr, gyverhub::Color color = Colors::UNSET, int size = 40) {
            _label(true, value, label, color, size);
        }
        void Label(CSREF value, CSREF label, gyverhub::Color color = Colors::UNSET, int size = 40) {
            _label(false, value, label.c_str(), color, size);
        }

        void _label(bool fstr, CSREF value, VSPTR label, gyverhub::Color color, int size) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("label"));
                _name();
                _value(value.c_str(), 0);
                _label(label, fstr);
                _color(color);
                _size(size);
                _tabw();
                _end();
            } else if (_checkName()) {
                sptr->appendEscaped(value.c_str(), 0);
            }
        }

        // ========================== TITLE ==========================
        void Title(FSTR label) {
            _title(true, label);
        }
        void Title(CSREF label) {
            _title(false, label.c_str());
        }

        void _title(bool fstr, VSPTR label) {
            if (_isUI()) {
                _begin(F("title"));
                _label(label, fstr);
                _end();
            }
        }

        // ========================== LOG ==========================
        void Log(GHlog* log, FSTR label = nullptr) {
            _log(true, log, label);
        }
        void Log(GHlog* log, CSREF label) {
            _log(false, log, label.c_str());
        }

        void _log(bool fstr, GHlog* log, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("log"));
                _name();
                _value();
                _quot();
                log->read(sptr);
                _quot();
                _label(label, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                log->read(sptr);
            }
        }

        // ========================== DISPLAY ==========================
        void Display(FSTR value = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET, int rows = 2, int size = 40) {
            _display(true, value, label, color, rows, size);
        }
        void Display(CSREF value, CSREF label = "", gyverhub::Color color = Colors::UNSET, int rows = 2, int size = 40) {
            _display(false, value.c_str(), label.c_str(), color, rows, size);
        }

        void _display(bool fstr, VSPTR value, VSPTR label, gyverhub::Color color, int rows, int size) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("display"));
                _name();
                _value(value, fstr);
                _label(label, fstr);
                _color(color);
                _add(F(",\"rows\":"));
                *sptr += rows;
                _size(size);
                _tabw();
                _end();
            } else if (_checkName()) {
                sptr->appendEscaped(value, fstr);
            }
        }

        // ========================= TABLE ==========================
        void Table(FSTR text = nullptr, FSTR align = nullptr, FSTR width = nullptr, FSTR label = nullptr) {
            _table(true, text, align, width, label);
        }
        void Table(CSREF text, CSREF align = "", CSREF width = "", CSREF label = "") {
            _table(false, text.c_str(), align.c_str(), width.c_str(), label.c_str());
        }

        void _table(bool fstr, VSPTR value, VSPTR align, VSPTR width, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("table"));
                _name();
                _value(value, fstr);
                _add(F(",\"align\":\""));
                _add(align, fstr);
                _quot();
                _add(F(",\"width\":\""));
                _add(width, fstr);
                _quot();
                _label(label, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                sptr->appendEscaped(value, fstr);
            }
        }

        // ========================== HTML ==========================
        void HTML(FSTR value = nullptr, FSTR label = nullptr) {
            _html(true, value, label);
        }
        void HTML(CSREF value, CSREF label = "") {
            _html(false, value.c_str(), label.c_str());
        }

        void _html(bool fstr, VSPTR value, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("html"));
                _name();
                _value(value, fstr);
                _label(label, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                sptr->appendEscaped(value, fstr);
            }
        }

        // =========================== JS ===========================
        void JS(FSTR value) {
            _js(true, value);
        }
        void JS(CSREF value) {
            _js(false, value.c_str());
        }

        void _js(bool fstr, VSPTR value) {
            if (_isUI()) {
                _begin(F("js"));
                _value(value, fstr);
                _end();
            }
        }

        // ========================== INPUT ==========================
        bool Input(void* var = nullptr, DataType type = GH_NULL, FSTR label = nullptr, int maxv = 0, FSTR regex = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _input(true, F("input"), var, type, label, maxv, regex, color);
        }
        bool Input(void* var, DataType type, CSREF label, int maxv = 0, CSREF regex = "", gyverhub::Color color = Colors::UNSET) {
            return _input(false, F("input"), var, type, label.c_str(), maxv, regex.c_str(), color);
        }

        // ========================== PASS ==========================
        bool Pass(void* var = nullptr, DataType type = GH_NULL, FSTR label = nullptr, int maxv = 0, gyverhub::Color color = Colors::UNSET) {
            return _input(true, F("pass"), var, type, label, maxv, nullptr, color);
        }
        bool Pass(void* var, DataType type, CSREF label, int maxv = 0, gyverhub::Color color = Colors::UNSET) {
            return _input(false, F("pass"), var, type, label.c_str(), maxv, "", color);
        }

        bool _input(bool fstr, FSTR tag, void* var, DataType type, VSPTR label, int maxv, VSPTR regex, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                _value();
                _quot();
                appendObject(var, type);
                _quot();
                _label(label, fstr);
                if (maxv) _maxv((long)maxv);
                _add(F(",\"regex\":\""));
                sptr->appendEscaped(regex, fstr);
                _quot();
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, type);
            }
            return _parse(var, type);
        }

        // ========================== SLIDER ==========================
        bool Slider(void* var = nullptr, DataType type = GH_NULL, FSTR label = nullptr, float minv = 0, float maxv = 100, float step = 1, gyverhub::Color color = Colors::UNSET) {
            return _spinner(true, F("slider"), var, type, label, minv, maxv, step, color);
        }
        bool Slider(void* var, DataType type, CSREF label, float minv = 0, float maxv = 100, float step = 1, gyverhub::Color color = Colors::UNSET) {
            return _spinner(false, F("slider"), var, type, label.c_str(), minv, maxv, step, color);
        }

        // ========================== SPINNER ==========================
        bool Spinner(void* var = nullptr, DataType type = GH_NULL, FSTR label = nullptr, float minv = 0, float maxv = 100, float step = 1, gyverhub::Color color = Colors::UNSET) {
            return _spinner(true, F("spinner"), var, type, label, minv, maxv, step, color);
        }
        bool Spinner(void* var, DataType type, CSREF label, float minv = 0, float maxv = 100, float step = 1, gyverhub::Color color = Colors::UNSET) {
            return _spinner(false, F("spinner"), var, type, label.c_str(), minv, maxv, step, color);
        }

        bool _spinner(bool fstr, FSTR tag, void* var, DataType type, VSPTR label, float minv, float maxv, float step, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                _value();
                appendObject(var, type);
                _label(label, fstr);
                _minv(minv);
                _maxv(maxv);
                _step(step);
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, type);
            }
            return _parse(var, type);
        }

        // ========================== GAUGE ===========================
        void Gauge(float value = 0.f, FSTR text = nullptr, FSTR label = nullptr, float minv = 0.f, float maxv = 100.f, float step = 1.f, gyverhub::Color color = Colors::UNSET) {
            _gauge(true, value, text, label, minv, maxv, step, color);
        }
        void Gauge(float value, CSREF text, CSREF label = "", float minv = 0.f, float maxv = 100.f, float step = 1.f, gyverhub::Color color = Colors::UNSET) {
            _gauge(false, value, text.c_str(), label.c_str(), minv, maxv, step, color);
        }

        void _gauge(bool fstr, float value, VSPTR text, VSPTR label, float minv, float maxv, float step, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("gauge"));
                _name();
                _value();
                if (isnan(value)) *sptr += 0;
                else *sptr += value;
                _text(text, fstr);
                _label(label, fstr);
                _minv(minv);
                _maxv(maxv);
                _step(step);
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                *sptr += value;
            }
        }

        // ========================== SWITCH ==========================

        bool Switch(bool* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _switch(true, F("switch"), var, label, color, nullptr);
        }
        bool Switch(bool* var, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _switch(false, F("switch"), var, label.c_str(), color, nullptr);
        }

        bool SwitchIcon(bool* var = nullptr, FSTR label = nullptr, FSTR text = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _switch(true, F("switch_i"), var, label, color, text);
        }
        bool SwitchIcon(bool* var, CSREF label, CSREF text = "", gyverhub::Color color = Colors::UNSET) {
            return _switch(false, F("switch_i"), var, label.c_str(), color, text.c_str());
        }

        bool SwitchText(bool* var = nullptr, FSTR label = nullptr, FSTR text = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _switch(true, F("switch_t"), var, label, color, text);
        }
        bool SwitchText(bool* var, CSREF label, CSREF text = "", gyverhub::Color color = Colors::UNSET) {
            return _switch(false, F("switch_t"), var, label.c_str(), color, text.c_str());
        }

        bool _switch(bool fstr, FSTR tag, bool* var, VSPTR label, gyverhub::Color color, VSPTR text) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                _value();
                appendObject(var, GH_BOOL);
                _label(label, fstr);
                _color(color);
                _text(text, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, GH_BOOL);
            }
            return _parse(var, GH_BOOL);
        }

        // ========================== DATETIME ==========================
        bool Date(void* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _date(true, F("date"), var, label, color);
        }
        bool Date(void* var, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _date(false, F("date"), var, label.c_str(), color);
        }

        bool Time(void* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _date(true, F("time"), var, label, color);
        }
        bool Time(void* var, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _date(false, F("time"), var, label.c_str(), color);
        }

        bool DateTime(void* var = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _date(true, F("datetime"), var, label, color);
        }
        bool DateTime(void* var, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _date(false, F("datetime"), var, label.c_str(), color);
        }

        bool _date(bool fstr, FSTR tag, void* var, VSPTR label, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                _label(label, fstr);
                _value();
                appendObject(var, GH_UINT32);
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, GH_UINT32);
            }
            return _parse(var, GH_UINT32);
        }

        // ========================== SELECT ==========================
        bool Select(uint8_t* var, FSTR text, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _select(true, var, text, label, color);
        }
        bool Select(uint8_t* var, CSREF text, CSREF label = "", gyverhub::Color color = Colors::UNSET) {
            return _select(false, var, text.c_str(), label.c_str(), color);
        }

        bool _select(bool fstr, uint8_t* var, VSPTR text, VSPTR label, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("select"));
                _name();
                _value();
                appendObject(var, GH_UINT8);
                _text(text, fstr);
                _label(label, fstr);
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, GH_UINT8);
            }
            return _parse(var, GH_UINT8);
        }

        // ========================== FLAGS ==========================
        bool Flags(gyverhub::Flags* var = nullptr, FSTR text = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _flags(true, var, text, label, color);
        }
        bool Flags(gyverhub::Flags* var, CSREF text, CSREF label = "", gyverhub::Color color = Colors::UNSET) {
            return _flags(false, var, text.c_str(), label.c_str(), color);
        }

        bool _flags(bool fstr, void* var, VSPTR text, VSPTR label, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("flags"));
                _name();
                _value();
                appendObject(var, GH_FLAGS);
                _text(text, fstr);
                _label(label, fstr);
                _color(color);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, GH_FLAGS);
            }
            return _parse(var, GH_FLAGS);
        }

        // ========================== COLOR ==========================
        bool Color(gyverhub::Color* var = nullptr, FSTR label = nullptr) {
            return _color(true, var, label);
        }
        bool Color(gyverhub::Color* var, CSREF label) {
            return _color(false, var, label.c_str());
        }

        bool _color(bool fstr, gyverhub::Color* var, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("color"));
                _name();
                _value();
                appendObject(var, GH_COLOR);
                _label(label, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                appendObject(var, GH_COLOR);
            }
            return _parse(var, GH_COLOR);
        }

        // ========================== LED ==========================
        void LED(bool value = false, FSTR label = nullptr, FSTR icon = nullptr) {
            _led(true, value, label, icon);
        }
        void LED(bool value, CSREF label, CSREF icon = "") {
        _led(false, value, label.c_str(), icon.c_str());
        }

        void _led(bool fstr, bool value, VSPTR label, VSPTR text) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("led"));
                _name();
                _value();
                *sptr += value;
                _label(label, fstr);
                _text(text, fstr);
                _tabw();
                _end();
            } else if (_checkName()) {
                *sptr += value;
            }
        }

        // ========================== SPACE ==========================
        void Space(int height = 0) {
            if (_isUI()) {
                _begin(F("spacer"));
                _add(F(",\"height\":"));
                *sptr += height;
                _tabw();
                _end();
            }
        }

        // ========================== MENU ==========================
        bool Menu(FSTR text) {
            return _tabs(true, true, &menu, text, nullptr);
        }
        bool Menu(CSREF text) {
            return _tabs(false, true, &menu, text.c_str(), nullptr);
        }

        // ========================== TABS ==========================
        bool Tabs(uint8_t* var, FSTR text, FSTR label = nullptr) {
            return _tabs(true, false, var, text, label);
        }
        bool Tabs(uint8_t* var, CSREF text, CSREF label = "") {
            return _tabs(false, false, var, text.c_str(), label.c_str());
        }

        bool _tabs(bool fstr, bool menu, uint8_t* var, VSPTR text, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(menu ? F("menu") : F("tabs"));
                if (menu) {
                    _begin(F("menu"));
                    _add(F(",\"name\":\"_menu\""));
                } else {
                    _begin(F("tabs"));
                    _name();
                }
                _value();
                *sptr += *var;
                _text(text, fstr);
                _label(label, fstr);
                _tabw();
                _end();
            }
            bool act = _parse(var, GH_UINT8);
            if (act) refresh();
            return act;
        }

        // ========================= CANVAS =========================
        bool Canvas(int width = 400, int height = 300, gyverhub::Canvas* cv = nullptr, gyverhub::Point* pos = nullptr, FSTR label = nullptr) {
            return _canvas(true, width, height, cv, pos, label, false);
        }
        bool Canvas(int width, int height, gyverhub::Canvas* cv, gyverhub::Point* pos, CSREF label) {
            return _canvas(false, width, height, cv, pos, label.c_str(), false);
        }

        bool BeginCanvas(int width = 400, int height = 300, gyverhub::Canvas* cv = nullptr, gyverhub::Point* pos = nullptr, FSTR label = nullptr) {
            return _canvas(true, width, height, cv, pos, label, true);
        }
        bool BeginCanvas(int width, int height, gyverhub::Canvas* cv, gyverhub::Point* pos, CSREF label) {
            return _canvas(false, width, height, cv, pos, label.c_str(), true);
        }

        bool _canvas(bool fstr, int width, int height, gyverhub::Canvas* cv, gyverhub::Point* pos, VSPTR label, bool begin) {
            _nameAuto();
            if (!_isUI() && cv) cv->extBuffer(nullptr);

            if (_isUI()) {
                _begin(F("canvas"));
                _name();
                _add(F(",\"width\":"));
                *sptr += width;
                _add(F(",\"height\":"));
                *sptr += height;
                _label(label, fstr);
                if (pos) _add(F(",\"active\":1"));
                _value();
                *sptr += '[';
                if (begin && cv) cv->extBuffer(sptr);
                else EndCanvas();
            }
            return _parse(pos, GH_POS);
        }

        void EndCanvas() {
            if (_isUI()) {
                *sptr += ']';
                _tabw();
                _end();
            }
        }

        // ========================= IMAGE =========================
        void Image(FSTR path, FSTR label = nullptr) {
            _image(true, path, label);
        }
        void Image(CSREF path, CSREF label = "") {
            _image(false, path.c_str(), label.c_str());
        }

        void _image(bool fstr, VSPTR path, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("image"));
                _name();
                _value(path, fstr);
                _label(label, fstr);
                _tabw();
                _end();
            }
        }

        // ========================= STREAM =========================
        void Stream(uint16_t port = 82) {
            if (_isUI()) {
                _begin(F("stream"));
                _add(F(",\"port\":"));
                *sptr += port;
                _tabw();
                _end();
            }
        }

        // =========================== JOY ===========================
        bool Joystick(gyverhub::Point* pos = nullptr, bool autoc = 1, bool exp = 0, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _joy(F("joy"), true, pos, autoc, exp, label, color);
        }
        bool Joystick(gyverhub::Point* pos, bool autoc, bool exp, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _joy(F("joy"), false, pos, autoc, exp, label.c_str(), color);
        }

        // =========================== DPAD ===========================
        bool Dpad(gyverhub::Point* pos = nullptr, FSTR label = nullptr, gyverhub::Color color = Colors::UNSET) {
            return _joy(F("dpad"), true, pos, 0, 0, label, color);
        }
        bool Dpad(gyverhub::Point* pos, CSREF label, gyverhub::Color color = Colors::UNSET) {
            return _joy(F("dpad"), false, pos, 0, 0, label.c_str(), color);
        }

        bool _joy(FSTR tag, bool fstr, gyverhub::Point* pos, bool autoc, bool exp, VSPTR label, gyverhub::Color color) {
            _nameAuto();
            if (_isUI()) {
                _begin(tag);
                _name();
                if (autoc) {
                    _add(F(",\"auto\":"));
                    *sptr += autoc;
                }
                if (exp) {
                    _add(F(",\"exp\":"));
                    *sptr += exp;
                }
                _label(label, fstr);
                _color(color);
                _tabw();
                _end();
            }
            bool act = _parse(pos, GH_POS);
            if (act && pos != nullptr) {
                pos->x -= 255;
                pos->y -= 255;
            }
            return act;
        }

        // ======================= CONFIRM ========================
        bool Confirm(bool* var = nullptr, FSTR label = nullptr) {
            return _confirm(true, var, label);
        }
        bool Confirm(bool* var, CSREF label) {
            return _confirm(false, var, label.c_str());
        }

        bool _confirm(bool fstr, bool* var, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("confirm"));
                _name();
                _label(label, fstr);
                _end();
            }
            return _parse(var, GH_BOOL);
        }

        // ========================= PROMPT ========================
        bool Prompt(void* value = nullptr, DataType type = GH_NULL, FSTR label = nullptr) {
            return _prompt(true, value, type, label);
        }
        bool Prompt(void* value, DataType type, CSREF label) {
            return _prompt(false, value, type, label.c_str());
        }

        bool _prompt(bool fstr, void* value, DataType type, VSPTR label) {
            _nameAuto();
            if (_isUI()) {
                _begin(F("prompt"));
                _name();
                _value();
                _quot();
                appendObject(value, type);
                _quot();
                _label(label, fstr);
                _end();
            }
            return _parse(value, type);
        }
    };
}
