#ifndef _GyverHUB_h
#define _GyverHUB_h

#include <Arduino.h>

#include "builder.h"
#include "canvas.h"
#include "config.hpp"
#include "macro.hpp"
#include "stream.h"
#include "utils/build.h"
#include "utils/cmd_p.h"
#include "utils/color.h"
#include "utils/datatypes.h"
#include "utils/flags.h"
#include "utils/log.h"
#include "utils/misc.h"
#include "utils/modules.h"
#include "utils/stats.h"
#include "utils/stats_p.h"
#include "utils/timer.h"
#include "utils/json.h"

#ifdef GH_ESP_BUILD
#include <FS.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#ifndef GH_NO_OTA
#ifndef GH_NO_OTA_URL
#include <ESP8266httpUpdate.h>
#endif
#endif
#endif

#ifdef ESP32
#include <WiFi.h>
#ifndef GH_NO_OTA
#include <Update.h>
#ifndef GH_NO_OTA_URL
#include <HTTPUpdate.h>
#endif
#endif
#endif

#ifndef GH_NO_FS
#if (GH_FS == LittleFS)
#include <LittleFS.h>
#elif (GH_FS == SPIFFS)
#include <SPIFFS.h>
#endif
#endif

#endif

#if GH_MQTT_IMPL == GH_IMPL_ASYNC
# include "async/mqtt.h"
#elif GH_MQTT_IMPL == GH_IMPL_SYNC
# include "sync/mqtt.h"
#elif GH_MQTT_IMPL == GH_IMPL_NATIVE
# include "esp_idf/mqtt.h"
#elif GH_MQTT_IMPL == GH_IMPL_NONE

class HubMQTT {
   public:
    void setupMQTT(const char* host, uint16_t port, const char* login = nullptr, const char* pass = nullptr, uint8_t nqos = 0, bool nret = 0) {}
};

#endif


#if GH_MQTT_IMPL == GH_IMPL_ASYNC
# include "async/http.h"
# include "async/ws.h"
#elif GH_MQTT_IMPL == GH_IMPL_SYNC
# include "sync/http.h"
# include "sync/ws.h"
#elif GH_MQTT_IMPL == GH_IMPL_NATIVE
# include "esp_idf/http_ws.h"
#elif GH_MQTT_IMPL == GH_IMPL_NONE

class HubHTTP {};
class HubWS {};

#endif

// ========================== CLASS ==========================
class GyverHub : public HubBuilder, public HubStream, public HubHTTP, public HubMQTT, public HubWS {
   public:
    // ========================== CONSTRUCT ==========================

    // настроить префикс, название и иконку. Опционально задать свой ID устройства (для esp он генерируется автоматически)
    GyverHub(const char* prefix = "", const char* name = "", const char* icon = "", uint32_t id = 0) {
        config(prefix, name, icon, id);
    }

    // настроить префикс, название и иконку. Опционально задать свой ID устройства (для esp он генерируется автоматически)
    void config(const char* nprefix, const char* nname, const char* nicon, uint32_t nid = 0) {
        prefix = nprefix;
        name = nname;
        icon = nicon;
#ifdef GH_ESP_BUILD
        if (nid) ultoa((nid <= 0xfffff) ? (nid + 0xfffff) : nid, id, HEX);
        else {
            uint8_t mac[6];
            WiFi.macAddress(mac);
            ultoa(*((uint32_t*)(mac + 2)), id, HEX);
        }
#else
        ultoa((nid <= 0x100000) ? (nid + 0x100000) : nid, id, HEX);
#endif

#ifdef GH_NO_FS
        modules.unset(GH_MOD_FSBR | GH_MOD_FORMAT | GH_MOD_FETCH | GH_MOD_UPLOAD | GH_MOD_OTA | GH_MOD_OTA_URL | GH_MOD_DELETE | GH_MOD_RENAME);
#endif
#ifdef GH_NO_OTA
        modules.unset(GH_MOD_OTA);
#endif
#ifdef GH_NO_OTA_URL
        modules.unset(GH_MOD_OTA_URL);
#endif
#ifndef GH_ESP_BUILD
        modules.unset(GH_MOD_REBOOT | GH_MOD_FSBR | GH_MOD_FORMAT | GH_MOD_FETCH | GH_MOD_UPLOAD | GH_MOD_OTA | GH_MOD_OTA_URL | GH_MOD_DELETE | GH_MOD_RENAME);
#endif
    }

    // ========================== SETUP ==========================

    // запустить
    void begin() {
#if GH_HTTP_IMPL != GH_IMPL_NONE
        beginWS();
        beginHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        beginMQTT();
#endif

#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
#ifdef ESP8266
        fs_mounted = GH_FS.begin();
#else
        fs_mounted = GH_FS.begin(true);
#endif
#endif
#endif
        running_f = true;
        sendEvent(GH_START, GH_SYSTEM);
    }

    // остановить
    void end() {
#if GH_HTTP_IMPL != GH_IMPL_NONE
        endWS();
        endHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        endMQTT();
#endif
        running_f = false;
        sendEvent(GH_STOP, GH_SYSTEM);
    }

    // установить версию прошивки для отображения в Info и OTA
    void setVersion(const char* v) {
        version = v;
    }

    // установить размер буфера строки для сборки интерфейса при ручной отправке
    // 0 - интерфейс будет собран и отправлен цельной строкой
    // >0 - пакет будет отправляться частями
    void setBufferSize(uint16_t size) {
        buf_size = size;
    }

    // включение/отключение системных модулей
    GHmodule modules;

    // ========================== PIN ==========================

    // установить пин-код для открытия устройства (значение больше 1000, не может начинаться с 000..)
    void setPIN(uint32_t npin) {
        PIN = npin;
    }

    // прочитать пин-код
    uint32_t getPIN() {
        return PIN;
    }

    // ========================= ATTACH =========================

    // подключить функцию-сборщик интерфейса
    void onBuild(void (*handler)()) {
        build_cb = *handler;
    }

    // подключить функцию-обработчик запроса при ручном соединении
    void onManual(void (*handler)(String& s, bool broadcast)) {
        manual_cb = *handler;
    }

    // ========================= INFO =========================

    // подключить функцию-сборщик инфо
    void onInfo(void (*handler)(GHinfo_t info)) {
        info_cb = *handler;
    }

    // добавить поле в info
    void addInfo(const String& label, const String& text) {
        if (sptr) {
            *sptr += '\"';
            GH_addEsc(sptr, label.c_str());  //*sptr += label;
            *sptr += F("\":\"");
            GH_addEsc(sptr, text.c_str());  //*sptr += text;
            *sptr += F("\",");
        }
    }

    // ========================= CLI =========================

    // подключить обработчик входящих сообщений с веб-консоли
    void onCLI(void (*handler)(String& s)) {
        cli_cb = *handler;
    }

    // отправить текст в веб-консоль. Опционально цвет
    void print(const String& str, uint32_t color = GH_DEFAULT) {
        if (!focused()) return;
        GHJson answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("print"));
        answ.itemString(F("text"), str);
        answ.itemInteger(F("color"), color);
        answ.end();
        _send(answ);
    }

    // ========================== STATUS ==========================

    // подключить обработчик изменения статуса
    void onEvent(void (*handler)(GHevent_t state, GHconn_t from)) {
        event_cb = *handler;
    }

    // отправить event для отладки
    void sendEvent(GHevent_t state, GHconn_t from) {
        if (event_cb) event_cb(state, from);
    }

    // вернёт true, если система запущена
    bool running() {
        return running_f;
    }

    // подключить функцию-обработчик перезагрузки. Будет вызвана перед перезагрузкой
    void onReboot(void (*handler)(GHreason_t r)) {
#ifdef GH_ESP_BUILD
        reboot_cb = *handler;
#endif
    }

    // получить свойства текущего билда. Вызывать внутри обработчика
    GHbuild getBuild() {
        return bptr ? *bptr : GHbuild();
    }

    // true - интерфейс устройства сейчас открыт на сайте или в приложении
    bool focused() {
        if (!running_f) return 0;
        for (uint8_t i = 0; i < GH_CONN_AMOUNT; i++) {
            if (focus_arr[i]) return 1;
        }
        return 0;
    }

    // проверить фокус по указанному типу связи
    bool focused(GHconn_t from) {
        return focus_arr[from];
    }

    // обновить веб-интерфейс. Вызывать внутри обработчика build
    void refresh() {
        refresh_f = true;
    }

    // true - если билдер вызван для set или read операций
    bool buildRead() {
        return (bptr && (bptr->type == GH_BUILD_ACTION || bptr->type == GH_BUILD_READ));
    }

    // true - если билдер вызван для запроса компонентов (при загрузке панели управления)
    bool buildUI() {
        return (bptr && (bptr->type == GH_BUILD_UI || bptr->type == GH_BUILD_COUNT));
    }

    // получить текущее действие для ручной обработки значений
    const GHaction& action() const {
        return bptr->action;
    }

    // подключить обработчик запроса клиента
    void onRequest(bool (*handler)(GHbuild build)) {
        req_cb = *handler;
    }

    // ========================= FETCH ==========================

    // подключить обработчик скачивания
    void onFetch(void (*handler)(String& path, bool start)) {
        fetch_cb = *handler;
    }

    // отправить файл (вызывать в обработчике onFetch)
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
    void fetchFile(const char* path) {
        file_d = GH_FS.open(path, "r");
    }

    template<typename T>
    void fetchFile(T f) {
        file_d = f;
    }
#endif
#endif

    // отправить сырые данные (вызывать в обработчике onFetch)
    void fetchBytes(uint8_t* bytes, uint32_t size) {
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
        file_b = bytes;
        file_b_size = size;
        file_b_pgm = 0;
#endif
#endif
    }
    // отправить сырые данные из PGM (вызывать в обработчике onFetch)
    void fetchBytes_P(const uint8_t* bytes, uint32_t size) {
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
        file_b = bytes;
        file_b_size = size;
        file_b_pgm = 1;
#endif
#endif
    }

    // ========================= DATA ==========================
    // подключить обработчик данных (см. GyverHub.js API)
    void onData(void (*handler)(const char* name, const char* value)) {
        data_cb = *handler;
    }

    // ответить клиенту. Вызывать в обработчике onData (см. GyverHub.js API)
    void answer(const String& data) {
        GHJson answ;
        _datasend(answ, data);
        _answer(answ);
    }

    // отправить сырые данные вручную (см. GyverHub.js API)
    void send(const String& data) {
        GHJson answ;
        _datasend(answ, data);
        _send(answ);
    }

    // ========================= NOTIF ==========================
    // отправить пуш уведомление
    void sendPush(const String& text) {
        if (!running_f) return;
        upd_f = 1;
        GHJson answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("push"));
        answ.itemString(F("text"), text);
        answ.end();
        _send(answ, true);
    }

    // отправить всплывающее уведомление
    void sendNotice(const String& text, uint32_t color = GH_GREEN) {
        if (!running_f || !focused()) return;
        upd_f = 1;
        GHJson answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("notice"));
        answ.itemString(F("text"), text);
        answ.itemInteger(F("color"), color);
        answ.end();
        _send(answ);
    }

    // показать окно с ошибкой
    void sendAlert(const String& text) {
        if (!running_f || !focused()) return;
        upd_f = 1;
        GHJson answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("alert"));
        answ.itemString(F("text"), text);
        answ.end();
        _send(answ);
    }

    // ========================= UPDATE ==========================

    // отправить update вручную с указанием значения
    void sendUpdate(const String& name, const String& value) {
        _sendUpdate(name.c_str(), value.c_str());
    }

    // отправить update по имени компонента (значение будет прочитано в build). Нельзя вызывать из build. Имена можно передать списком через запятую
    void sendUpdate(const String& name) {
        if (!running_f || !build_cb || bptr || !focused()) return;
        GHbuild build(GH_BUILD_READ);
        bptr = &build;

        GHJson answ;
        sptr = &answ;
        _updateBegin(answ);

        char* str = (char*)name.c_str();
        char* p = str;
        GH_splitter(NULL);
        while ((p = GH_splitter(str)) != NULL) {
            build.type = GH_BUILD_READ;
            build.action.name = p;
            build.action.count = 0;
            answ.key(p);
            answ += '\"';
            answ.reserve(answ.length() + 64);
            build_cb();
            answ += F("\",");
        }
        bptr = nullptr;
        sptr = nullptr;
        answ[answ.length() - 1] = '}';
        answ.end();
        _send(answ);
    }

    // автоматически рассылать обновления клиентам при действиях на странице (умолч. true)
    void sendUpdateAuto(bool f) {
        autoUpd_f = f;
    }

    void _updateBegin(GHJson& answ) {
        upd_f = 1;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("update"));
        answ.key(F("updates"));
        answ += '{';
    }
    void _sendUpdate(const char* name, const char* value) {
        if (!running_f || !focused()) return;
        GHJson answ;
        _updateBegin(answ);
        answ.itemString(name, value);
        answ += '}';
        answ.end();
        _send(answ);
    }

    // ======================= SEND CANVAS ========================
    // отправить холст
    void sendCanvas(const String& name, GHcanvas& cv) {
        if (!running_f) return;
        GHJson answ;
        _updateBegin(answ);
        answ.key(name.c_str());
        answ += '[';
        answ += cv.buf;
        answ += F("]}");
        answ.end();
        _send(answ);
        cv.clearBuffer();
    }

    // начать отправку холста
    void sendCanvasBegin(const String& name, GHcanvas& cv) {
        if (!running_f) return;
        cv.buf.clear();
        _updateBegin(cv.buf);
        cv.buf.key(name.c_str());
        cv.buf += '[';
    }

    // закончить отправку холста
    void sendCanvasEnd(GHcanvas& cv) {
        cv.buf += F("]}");
        cv.buf.end();
        _send(cv.buf);
        cv.clearBuffer();
    }

    // ======================== SEND GET =========================

    // автоматически отправлять новое состояние на get-топик при изменении через set (умолч. false)
    void sendGetAuto(bool v) {
#ifdef GH_ESP_BUILD
        autoGet_f = v;
#endif
    }

    // отправить имя-значение на get-топик (MQTT)
    void sendGet(GH_UNUSED const String& name, GH_UNUSED const String& value) {
        if (!running_f) return;
#if GH_MQTT_IMPL != GH_IMPL_NONE
        String topic(prefix);
        topic += F("/hub/");
        topic += id;
        topic += F("/get/");
        topic += name;
        sendMQTT(topic, value);
#endif
    }

    // отправить значение по имени компонента на get-топик (MQTT) (значение будет прочитано в build). Имена можно передать списком через запятую
    void sendGet(GH_UNUSED const String& name) {
#if GH_MQTT_IMPL != GH_IMPL_NONE
        if (!running_f || !build_cb || bptr) return;
        GHbuild build(GH_BUILD_READ);
        bptr = &build;

        String value;
        sptr = &value;

        char* str = (char*)name.c_str();
        char* p = str;
        GH_splitter(NULL);
        while ((p = GH_splitter(str)) != NULL) {
            build.type = GH_BUILD_READ;
            build.action.name = p;
            build.action.count = 0;
            build_cb();
            if (build.type == GH_BUILD_NONE) sendGet(p, value);
        }
        bptr = nullptr;
        sptr = nullptr;
#endif
    }

    // ========================== ON/OFF ==========================

    // отправить MQTT LWT команду на включение
    void turnOn() {
        _power(F("online"));
    }

    // отправить MQTT LWT команду на выключение
    void turnOff() {
        _power(F("offline"));
    }

    // ========================== PARSER ==========================

    // парсить строку вида PREFIX/ID/HUB_ID/CMD/NAME=VALUE
    void parse(char* url, GHconn_t from) {
        if (!running_f) return;
        char* eq = strchr(url, '=');
        char val[1] = "";
        if (eq) *eq = 0;
        parse(url, eq ? (eq + 1) : val, from);
        if (eq) *eq = '=';
    }

    // парсить строку вида PREFIX/ID/HUB_ID/CMD/NAME с отдельным value
    void parse(char* url, const char* value, GHconn_t from) {
        if (!running_f) return;

        if (!modules.read(GH_MOD_SERIAL) && from == GH_SERIAL) return;
        else if (!modules.read(GH_MOD_BT) && from == GH_BT) return;
        else if (!modules.read(GH_MOD_WS) && from == GH_WS) return;
        else if (!modules.read(GH_MOD_HTTP) && from == GH_HTTP) return;
        else if (!modules.read(GH_MOD_MQTT) && from == GH_MQTT) return;

#if defined(GH_ESP_BUILD) && !defined(GH_NO_FS) && !defined(GH_NO_OTA) && !defined(GH_NO_OTA_URL)
        if (ota_url_f) return;
#endif

        if (!strcmp(url, prefix)) {  // == prefix
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return sendEvent(GH_DISCOVER_ALL, from);
        }

        GHparser<5> p(url);
        if (strcmp(p.str[0], prefix)) return;  // wrong prefix
        if (strcmp(p.str[1], id)) return;      // wrong id

        if (p.size == 2) {
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return sendEvent(GH_DISCOVER, from);
        }

        if (p.size == 3) return sendEvent(GH_UNKNOWN, from);
        // p.size >= 4

        const char* cmd = p.str[3];
        int cmdn = GH_getCmd(cmd);
        if (cmdn < 0) return sendEvent(GH_UNKNOWN, from);

        const char* client_id = p.str[2];
        const char* name = p.str[4];
        GHclient client(from, client_id);
        client_ptr = &client;

        if (!_reqHook(name, value, client, (GHevent_t)cmdn)) {
            answerErr(F("Forbidden"));
            return;
        }

        if (p.size == 4) {
#if GH_MQTT_IMPL != GH_IMPL_NONE
            // MQTT HOOK
            if (from == GH_MQTT && build_cb) {
                if (!strcmp_P(cmd, PSTR("read"))) {
                    if (modules.read(GH_MOD_READ)) sendGet(name);
                    client_ptr = nullptr;
                    return sendEvent(GH_READ_HOOK, from);
                } else if (!strcmp_P(cmd, PSTR("set"))) {
                    if (modules.read(GH_MOD_SET)) {
                        GHbuild build(GH_BUILD_ACTION, name, value, client);
                        bptr = &build;
                        build_cb();
                        bptr = nullptr;
                        client_ptr = nullptr;
                        if (autoGet_f) sendGet(name, value);
                        if (autoUpd_f) _sendUpdate(name, value);
                    }
                    return sendEvent(GH_SET_HOOK, from);
                }
            }
#endif
            setFocus(from);

            switch (cmdn) {
                case 0:  // focus
                    answerUI();
                    return sendEvent(GH_FOCUS, from);

                case 1:  // ping
                    answerType();
                    return sendEvent(GH_PING, from);

                case 2:  // unfocus
                    answerType();
                    clearFocus(from);
                    return sendEvent(GH_UNFOCUS, from);

                case 3:  // info
                    if (modules.read(GH_MOD_INFO)) answerInfo();
                    return sendEvent(GH_INFO, from);

#ifdef GH_ESP_BUILD
                case 4:  // fsbr
#ifndef GH_NO_FS
                    if (modules.read(GH_MOD_FSBR)) {
                        if (fs_mounted) answerFsbr();
                        else answerType(F("fs_error"));
                    }
#else
                    answerDsbl();
#endif
                    return sendEvent(GH_FSBR, from);

                case 5:  // format
#ifndef GH_NO_FS
                    if (modules.read(GH_MOD_FORMAT)) {
                        GH_FS.format();
                        GH_FS.end();
                        fs_mounted = GH_FS.begin();
                        answerFsbr();
                    }
#else
                    answerDsbl();
#endif
                    return sendEvent(GH_FORMAT, from);

                case 6:  // reboot
                    if (modules.read(GH_MOD_REBOOT)) {
                        reboot_f = GH_REB_BUTTON;
                        answerType();
                    }
                    return sendEvent(GH_REBOOT, from);
#endif
            }
            return;
        }

        // p.size == 5
        setFocus(from);

        switch (cmdn) {
            case 7:  // data
                answ_f = 0;
                if (data_cb) data_cb(name, value);
                if (!answ_f) answerType();
                return sendEvent(GH_DATA, from);

            case 8:  // set
                if (!build_cb || !modules.read(GH_MOD_SET)) {
                    answerType();
                } else {
                    GHbuild build(GH_BUILD_ACTION, name, value, client);
                    bptr = &build;
                    upd_f = refresh_f = 0;
                    build_cb();
                    bptr = nullptr;
#ifdef GH_ESP_BUILD
                    if (autoGet_f) sendGet(name, value);
#endif
                    if (autoUpd_f) _sendUpdate(name, value);
                    if (refresh_f) answerUI();
                    else if (!upd_f) answerType();
                }
                return sendEvent(GH_SET, from);

            case 9:  // cli
                answerType();
                if (cli_cb) {
                    String str(value);
                    cli_cb(str);
                }
                return sendEvent(GH_CLI, from);

#ifdef GH_ESP_BUILD
            case 10:  // delete
#ifndef GH_NO_FS
                if (modules.read(GH_MOD_DELETE)) {
                    GH_FS.remove(name);
                    GH_rmdir(name);
                    answerFsbr();
                }
#else
                answerDsbl();
#endif
                return sendEvent(GH_DELETE, from);

            case 11:  // rename
#ifndef GH_NO_FS
                if (modules.read(GH_MOD_RENAME) && GH_FS.rename(name, value)) answerFsbr();
#else
                answerDsbl();
#endif
                return sendEvent(GH_RENAME, from);

            case 12:  // fetch
#ifndef GH_NO_FS
                if (!file_d && !file_b && modules.read(GH_MOD_FETCH)) {
                    fetch_path = name;
                    if (fetch_cb) fetch_cb(fetch_path, true);
                    if (!file_d && !file_b) file_d = GH_FS.open(name, "r");
                    if (file_d || file_b) {
                        fs_client = client;
                        fs_tmr = millis();
                        uint32_t size = file_b ? file_b_size : file_d.size();
                        file_b_idx = 0;
                        dwn_chunk_count = 0;
                        dwn_chunk_amount = (size + GH_DOWN_CHUNK_SIZE - 1) / GH_DOWN_CHUNK_SIZE;  // round up
                        answerType(F("fetch_start"));
                        return sendEvent(GH_FETCH, from);
                    }
                }
#endif
                answerType(F("fetch_err"));
                return sendEvent(GH_FETCH_ERROR, from);

            case 13:  // fetch_chunk
#ifndef GH_NO_FS
                fs_tmr = millis();
                if ((!file_d && !file_b) || fs_client != client || !modules.read(GH_MOD_FETCH)) {
                    answerType(F("fetch_err"));
                    return sendEvent(GH_FETCH_ERROR, from);
                } else {
                    answerChunk();
                    dwn_chunk_count++;
                    if (dwn_chunk_count >= dwn_chunk_amount) {
                        if (fetch_cb) fetch_cb(fetch_path, false);
                        if (file_d) file_d.close();
                        file_b = nullptr;
                        fetch_path = "";
                        return sendEvent(GH_FETCH_FINISH, from);
                    }
                    return sendEvent(GH_FETCH_CHUNK, from);
                }
#endif
                break;

            case 14:  // fetch_stop
#ifndef GH_NO_FS
                if (fetch_cb) fetch_cb(fetch_path, false);
                if (file_d) file_d.close();
                file_b = nullptr;
                fetch_path = "";
                sendEvent(GH_FETCH_ABORTED, from);
#endif
                break;

            case 15:  // upload
#ifdef GH_NO_FS
                answerDsbl();
                return sendEvent(GH_UPLOAD_ERROR, from);
#else
                if (!modules.read(GH_MOD_UPLOAD)) {
                    answerDsbl();
                    return sendEvent(GH_UPLOAD_ERROR, from);
                }

                if (fs_upload_file) {
                    answerType(F("upload_err"));
                    return sendEvent(GH_UPLOAD_ERROR, from);
                }

                GH_mkdir_pc(name);
                fs_upload_file = GH_FS.open(name, "w");
                if (!fs_upload_file) {
                    answerType(F("upload_err"));
                    return sendEvent(GH_UPLOAD_ERROR, from);
                }

                fs_upload_client = client;
                fs_upload_tmr = millis();

                answerType(F("upload_start"));
                return sendEvent(GH_UPLOAD, from);
#endif

            case 16:  // upload_chunk
#ifdef GH_NO_FS
                answerDsbl();
                return sendEvent(GH_UPLOAD_ERROR, from);
#else
                if (!fs_upload_file || fs_upload_client != client) {
                    answerType(F("upload_err"));
                    return sendEvent(GH_UPLOAD_ERROR, from);
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    answerType(F("upload_err"));
                    return sendEvent(GH_UPLOAD_ERROR, from);
                }

                GH_B64toFile(fs_upload_file, value);

                if (isLast) {
                    fs_upload_file.close();
                    answerType(F("upload_end"));
                    sendEvent(GH_UPLOAD_FINISH, from);
                } else {
                    fs_upload_tmr = millis();
                    answerType(F("upload_next_chunk"));
                    sendEvent(GH_UPLOAD_CHUNK, from);
                }
                return;
                
#endif

            case 17: { // ota
#ifdef GH_NO_OTA
                answerDsbl();
                return sendEvent(GH_OTA_ERROR, from);
#else
                if (!modules.read(GH_MOD_OTA)) {
                    answerDsbl();
                    return sendEvent(GH_OTA_ERROR, from);
                }

                if (ota_f) {
                    answerErr(F("Update already running"));
                    return sendEvent(GH_OTA_ERROR, from);
                }

                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    answerErr(F("Invalid type"));
                    return sendEvent(GH_OTA_ERROR, from);
                }

                size_t ota_size;
                int ota_type;
                if (isFlash) {
                    ota_type = U_FLASH;
#ifdef ESP8266
                    ota_size = (size_t)((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#else
                    ota_size = UPDATE_SIZE_UNKNOWN;
#endif
                } else {
#ifdef ESP8266
                    ota_type = U_FS;
                    ota_size = (size_t)&_FS_end - (size_t)&_FS_start;
                    close_all_fs();
#else
                    ota_type = U_SPIFFS;
                    ota_size = UPDATE_SIZE_UNKNOWN;
#endif
                }

                if (!Update.begin(ota_size, ota_type)) {
                    answerType(F("ota_err"));
                    return sendEvent(GH_OTA_ERROR, from);
                }

                ota_client = client;
                ota_f = true;
                ota_tmr = millis();
                answerType(F("ota_start"));
                return sendEvent(GH_OTA, from);
#endif
            }

            case 18: { // ota_chunk
#ifdef GH_NO_OTA
                answerDsbl();
                return sendEvent(GH_OTA_ERROR, from);
#else
                if (!ota_f || ota_client != client) {
                    answerType(F("ota_err"));
                    return sendEvent(GH_OTA_ERROR, from);
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    answerType(F("ota_err"));
                    return sendEvent(GH_OTA_ERROR, from);
                }

                GH_B64toUpdate(value);

                if (isLast) {
                    ota_f = false;
                    reboot_f = GH_REB_OTA;
                    if (Update.end(true)) answerType(F("ota_end"));
                    else answerType(F("ota_err"));
                    sendEvent(GH_OTA_FINISH, from);
                } else {
                    answerType(F("ota_next_chunk"));
                    ota_tmr = millis();
                    sendEvent(GH_OTA_CHUNK, from);
                }
                return;
#endif
            }

            case 19: { // ota_url
#ifdef GH_NO_OTA_URL
                answerDsbl();
                return sendEvent(GH_OTA_URL, from);
#else
                if (!modules.read(GH_MOD_OTA_URL)) {
                    answerDsbl();
                    return sendEvent(GH_OTA_URL, from);
                }

                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    answerErr(F("Invalid type"));
                    return sendEvent(GH_OTA_URL, from);
                }

                answerType();
                sendEvent(GH_OTA_URL, from);

                bool ok = 0;
                ota_url_f = 1;
#ifdef ESP8266
                ESPhttpUpdate.rebootOnUpdate(false);
                BearSSL::WiFiClientSecure client;
                if (strncmp_P(value, PSTR("https"), 5) == 0) client.setInsecure();
                if (isFlash) ok = ESPhttpUpdate.update(client, value);
                else ESPhttpUpdate.updateFS(client, value);
#else
                httpUpdate.rebootOnUpdate(false);
                WiFiClientSecure client;
                if (strncmp_P(value, PSTR("https"), 5) == 0) client.setInsecure();
                if (isFlash) ok = httpUpdate.update(client, value);
                else ok = httpUpdate.updateSpiffs(client, value);
#endif
                if (ok) {
                    reboot_f = GH_REB_OTA_URL;
                    answerType(F("ota_url_ok"));
                } else {
                    ota_url_f = 0;
                    answerType(F("ota_url_err"));
                }

                return;
#endif
            }
#endif
        }
    }

    // ========================== TICK ==========================

    // тикер, вызывать в loop
    bool tick() {
        if (!running_f) return 0;

        if ((uint16_t)((uint16_t)millis() - focus_tmr) >= 1000) {
            focus_tmr = millis();
            for (uint8_t i = 0; i < GH_CONN_AMOUNT; i++) {
                if (focus_arr[i]) focus_arr[i]--;
            }
        }

#ifndef GH_NO_STREAM
        tickStream();
#endif
#if GH_HTTP_IMPL != GH_IMPL_NONE && GH_HTTP_IMPL != GH_IMPL_NATIVE
        tickWS();
        tickHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE && GH_MQTT_IMPL != GH_IMPL_NATIVE
        tickMQTT();
#endif

#ifdef GH_ESP_BUILD
#ifndef GH_NO_OTA
        if (ota_f && (uint16_t)millis() - ota_tmr >= (GH_CONN_TOUT * 1000)) {
            Update.end();
            ota_f = false;
            sendEvent(GH_OTA_ABORTED, ota_client.from);
        }
#endif
#ifndef GH_NO_FS
        if (fs_upload_file && (uint16_t)millis() - fs_upload_tmr >= (GH_CONN_TOUT * 1000)) {
            fs_upload_file.close();
            sendEvent(GH_UPLOAD_ABORTED, fs_upload_client.from);
        }

        if ((file_d || file_b) && (uint16_t)millis() - fs_tmr >= (GH_CONN_TOUT * 1000)) {
            if (fetch_cb) fetch_cb(fetch_path, false);
            if (file_d) file_d.close();
            file_b = nullptr;
            fetch_path = "";
            sendEvent(GH_FETCH_ABORTED, fs_client.from);
        }
#endif
        if (reboot_f) {
            if (reboot_cb) reboot_cb(reboot_f);
            delay(2000);
            ESP.restart();
        }
#endif
        return 1;
    }

    // =========================================================================================
    // ======================================= PRIVATE =========================================
    // =========================================================================================
   private:
    void _rebootOTA() {
#ifdef GH_ESP_BUILD
        reboot_f = GH_REB_OTA;
#endif
    }
    const char* getPrefix() {
        return prefix;
    }
    const char* getID() {
        return id;
    }
    void _afterComponent() {
        switch (buf_mode) {
            case GH_NORMAL:
                break;

            case GH_COUNT:
                buf_count += sptr->length();
                *sptr = "";
                break;

            case GH_CHUNKED:
                if (sptr->length() >= buf_size) {
                    _answer(*sptr, false);
                    *sptr = "";
                }
                break;
        }
    }
    void _power(FSTR mode) {
        if (!running_f) return;

#if GH_MQTT_IMPL != GH_IMPL_NONE
        String topic(prefix);
        topic += F("/hub/");
        topic += id;
        topic += F("/status");
        sendMQTT(topic, mode);
#endif
    }

    bool _reqHook(const char* name, const char* value, GHclient client, GHevent_t event) {
        if (req_cb && !req_cb(GHbuild(GH_BUILD_NONE, name, value, client, event))) return 0;  // forbidden
        return 1;
    }
    bool _checkModule(GHmodule_t mod) {
        return modules.read(mod);
    }

#if defined(GH_ESP_BUILD) && !defined(GH_NO_FS)
    void _fetchStartHook(String& path, File** file, const uint8_t** bytes, uint32_t* size, bool* pgm) {
        if (!fetch_cb || file_d || file_b) return;  // busy

        fetch_cb(path, true);
        *file = &file_d;
        *bytes = file_b;
        *size = file_b_size;
        *pgm = file_b_pgm;
    }
    void _fetchEndHook(String& path) {
        if (!fetch_cb) return;

        fetch_cb(path, false);
        file_b = nullptr;
        file_b_size = 0;
        if (file_d) file_d.close();
    }
#endif

    // ======================= INFO ========================
    void answerInfo() {
        GHJson answ;
        answ.reserve(250);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("info"));

        answ += F("\"info\":{\"version\":{");
        answ.itemString(F("Library"), GH_LIB_VERSION);
        if (version) answ.itemString(F("Firmware"), version);

        checkEndInfo(answ, GH_INFO_VERSION);
        answ += (F(",\"net\":{"));
#ifdef GH_ESP_BUILD
        answ.itemString(F("Mode"), WiFi.getMode() == WIFI_AP ? F("AP") : (WiFi.getMode() == WIFI_STA ? F("STA") : F("AP_STA")));
        answ.itemString(F("MAC"), WiFi.macAddress());
        answ.itemString(F("SSID"), WiFi.SSID());
        answ.itemString(F("RSSI"), String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + '%');
        answ.itemString(F("IP"), WiFi.localIP().toString());
        answ.itemString(F("AP_IP"), WiFi.softAPIP().toString());
#endif
        checkEndInfo(answ, GH_INFO_NETWORK);
        answ += (F(",\"memory\":{"));

#ifdef GH_ESP_BUILD
        answ.item(F("RAM"), String("[") + ESP.getFreeHeap() + ",0]");

#ifndef GH_NO_FS
#ifdef ESP8266
        FSInfo fs_info;
        GH_FS.info(fs_info);
        answ.item(F("Flash"), String("[") + fs_info.usedBytes + ',' + fs_info.totalBytes + "]");
#else
        answ.item(F("Flash"), String("[") + GH_FS.usedBytes() + ',' + GH_FS.totalBytes() + "]");
#endif

        answ.item(F("Sketch"), String("[") + ESP.getSketchSize() + ',' + ESP.getFreeSketchSpace() + "]");
#endif
#endif

        checkEndInfo(answ, GH_INFO_MEMORY);
        answ += (F(",\"system\":{"));
        answ.itemInteger(F("Uptime"), millis() / 1000ul);
#ifdef GH_ESP_BUILD
#ifdef ESP8266
        answ.itemString(F("Platform"), F("ESP8266"));
#else
        answ.itemString(F("Platform"), F("ESP32"));
#endif
        answ.itemInteger(F("CPU_MHz"), ESP.getCpuFreqMHz());
        answ.itemString(F("Flash_chip"), String(ESP.getFlashChipSize() / 1000.0, 1) + " kB");
#endif

#ifdef __AVR_ATmega328P__
        answ.itemString(F("Platform"), F("ATmega328"));
#endif

        checkEndInfo(answ, GH_INFO_SYSTEM);
        answ += '}';

        answ.end();
        _answer(answ);
    }
    void checkEndInfo(String& answ, GHinfo_t info) {
        if (info_cb) {
            sptr = &answ;
            info_cb(info);
            sptr = nullptr;
        }
        if (answ[answ.length() - 1] == ',') answ[answ.length() - 1] = '}';
        else answ += '}';
    }

    // ======================= UI ========================
    void answerUI() {
        if (!build_cb) return answerType();
        GHbuild build;
        build.client = *client_ptr;
        bptr = &build;
        bool chunked = buf_size;

#ifdef GH_ESP_BUILD
        if (build.client.from == GH_WS || build.client.from == GH_MQTT) chunked = false;
#endif

        if (!chunked) {
            build.type = GH_BUILD_COUNT;
            build.action.count = 0;
            buf_mode = GH_COUNT;
            buf_count = 0;
            String count;
            sptr = &count;
            tab_width = 0;
            build_cb();
        }

        GHJson answ;
        answ.reserve((chunked ? buf_size : buf_count) + 100);
        answ.begin();
        answ.key(F("controls"));
        answ += '[';
        buf_mode = chunked ? GH_CHUNKED : GH_NORMAL;
        build.type = GH_BUILD_UI;
        build.action.count = 0;
        sptr = &answ;
        tab_width = 0;
        build_cb();
        sptr = nullptr;
        bptr = nullptr;

        if (answ[answ.length() - 1] == ',') answ[answ.length() - 1] = ']';  // ',' = ']'
        else answ += ']';
        answ += ',';
        answ.appendId(id);
        answ.itemString(F("type"), F("ui"));
        answ.end();
        _answer(answ);
    }

    // ======================= TYPE ========================
    void answerType(FSTR type = nullptr) {
        if (!type) type = F("OK");
        GHJson answ;
        answ.reserve(50);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), type);
        answ.end();
        _answer(answ);
    }
    void answerErr(FSTR err) {
        GHJson answ;
        answ.reserve(50);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("ERR"));
        answ.itemString(F("text"), err);
        answ.end();
        _answer(answ);
    }
    void answerDsbl() {
        answerErr(F("Module disabled"));
    }

    // ======================= FSBR ========================
    void answerFsbr() {
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
        GHJson answ;
        answ.reserve(100);
        
        uint16_t count = 0;
        GH_showFiles(answ, "/", GH_FS_DEPTH, &count);

        answ.clear();
        answ.reserve(count + 50);
        answ.begin();
        answ.key(F("fs"));
        answ += '{';
        answ.itemInteger(F("/"), 0);
        GH_showFiles(answ, "/", GH_FS_DEPTH);
        answ[answ.length() - 1] = '}';  // ',' = '}'
        answ += ',';

        answ.appendId(id);
        answ.itemString(F("type"), F("fsbr"));

#ifdef ESP8266
        FSInfo fs_info;
        GH_FS.info(fs_info);
        answ.itemInteger(F("total"), fs_info.totalBytes);
        answ.itemInteger(F("used"), fs_info.usedBytes);
#else
        answ.itemInteger(F("total"), GH_FS.totalBytes());
        answ.itemInteger(F("used"), GH_FS.usedBytes());
#endif
        answ.end();
        _answer(answ);
#endif
#else
        answerDsbl();
#endif
    }

    // ======================= DISCOVER ========================
    void answerDiscover() {
        uint32_t hash = 0;
        if (PIN > 999) {
            char pin_s[11];
            ultoa(PIN, pin_s, 10);
            uint16_t len = strlen(pin_s);
            for (uint16_t i = 0; i < len; i++) {
                hash = (((uint32_t)hash << 5) - hash) + pin_s[i];
            }
        }

        GHJson answ;
        answ.reserve(120);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("discover"));
        answ.itemString(F("name"), name);
        answ.itemString(F("icon"), icon);
        answ.itemInteger(F("PIN"), hash);
        answ.itemString(F("version"), version);
        answ.itemInteger(F("max_upl"), GH_UPL_CHUNK_SIZE);
#ifdef ATOMIC_FS_UPDATE
        answ.itemString(F("ota_t"), F("gz"));
#else
        answ.itemString(F("ota_t"), F("bin"));
#endif
        answ.itemInteger(F("modules"), modules.mods);
        answ.end();
        _answer(answ, true);
    }

    // ======================= CHUNK ========================
    void answerChunk() {
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
        GHJson answ;
        answ.reserve(GH_DOWN_CHUNK_SIZE + 100);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("fetch_next_chunk"));
        answ.itemInteger(F("chunk"), dwn_chunk_count);
        answ.itemInteger(F("amount"), dwn_chunk_amount);
        answ += F("\"data\":\"");
        if (file_b) GH_bytesToB64(file_b, file_b_idx, file_b_size, file_b_pgm, answ);
        else GH_fileToB64(file_d, answ);
        answ += '\"';
        answ.end();
        _answer(answ);
#endif
#endif
    }

    // ======================= ANSWER ========================
    void _answer(String& answ, bool close = true) {
        if (!client_ptr) return;
#ifdef GH_ESP_BUILD
        switch (client_ptr->from) {
#if GH_HTTP_IMPL != GH_IMPL_NONE
            case GH_WS:
                answerWS(answ);
                break;
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
            case GH_MQTT:
                answerMQTT(answ, client_ptr->id);
                break;
#endif
#if GH_HTTP_IMPL != GH_IMPL_NONE
            case GH_HTTP:
                answerHTTP(answ);
                break;
#endif
#ifndef GH_NO_STREAM
            case GH_SERIAL:
                sendStream(answ);
                break;
#endif
            case GH_MANUAL2:
                if (manual_cb) manual_cb(answ, false);
                break;

            default:
                break;
        }
#endif
        if (close) client_ptr = nullptr;
    }

    // ======================= SEND ========================
    void _send(String& answ, bool broadcast = false) {
        if (manual_cb) manual_cb(answ, broadcast);

#ifndef GH_NO_STREAM
        if (focus_arr[GH_SERIAL]) sendStream(answ);
#endif

#if GH_HTTP_IMPL != GH_IMPL_NONE
        if (focus_arr[GH_WS]) sendWS(answ);
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        if (focus_arr[GH_MQTT] || broadcast) sendMQTT(answ);
#endif
    }
    void _datasend(GHJson& answ, const String& data) {
        answ_f = 1;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("data"));
        answ.itemString(F("data"), data);
        answ.end();
    }

    // ========================== MISC ==========================
    void setFocus(GHconn_t from) {
        focus_arr[from] = GH_CONN_TOUT;
    }
    void clearFocus(GHconn_t from) {
        focus_arr[from] = 0;
        client_ptr = nullptr;
    }

    // ========================== VARS ==========================
    const char* prefix = nullptr;
    const char* name = nullptr;
    const char* icon = nullptr;
    const char* version = nullptr;
    uint32_t PIN = 0;
    char id[9];

    void (*fetch_cb)(String& path, bool start) = nullptr;
    void (*data_cb)(const char* name, const char* value) = nullptr;
    void (*build_cb)() = nullptr;
    bool (*req_cb)(GHbuild build) = nullptr;
    void (*info_cb)(GHinfo_t info) = nullptr;
    void (*cli_cb)(String& str) = nullptr;
    void (*manual_cb)(String& s, bool broadcast) = nullptr;
    void (*event_cb)(GHevent_t state, GHconn_t from) = nullptr;
    GHclient* client_ptr = nullptr;

    bool running_f = 0;
    bool refresh_f = 0;
    bool upd_f = 0;
    bool answ_f = 0;

    enum GHbuildmode_t {
        GH_NORMAL,
        GH_COUNT,
        GH_CHUNKED,
    };
    GHbuildmode_t buf_mode = GH_NORMAL;
    uint16_t buf_size = 0;
    uint16_t buf_count = 0;

    uint16_t focus_tmr = 0;
    int8_t focus_arr[GH_CONN_AMOUNT] = {};
    bool autoUpd_f = true;

#ifdef GH_ESP_BUILD
    void (*reboot_cb)(GHreason_t r) = nullptr;
    bool autoGet_f = 0;
    GHreason_t reboot_f = GH_REB_NONE;

#ifndef GH_NO_OTA_URL
    bool ota_url_f = 0;
#endif
#ifndef GH_NO_OTA
    bool ota_f = false;
    uint16_t ota_tmr = 0;
    GHclient ota_client;
#endif`
#ifndef GH_NO_FS
    bool fs_mounted = 0;
    // upload
    GHclient fs_upload_client;
    uint16_t fs_upload_tmr = 0;
    File fs_upload_file;
    // fetch
    GHclient fs_client;
    uint16_t fs_tmr = 0;
    String fetch_path;
    const uint8_t* file_b = nullptr;
    uint32_t file_b_size, file_b_idx;
    bool file_b_pgm = 0;
    File file_d;
    uint16_t dwn_chunk_count = 0;
    uint16_t dwn_chunk_amount = 0;
#endif
#endif
};
#endif