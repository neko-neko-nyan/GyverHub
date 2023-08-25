#ifndef _GyverHUB_h
#define _GyverHUB_h

#include <Arduino.h>

#include "ui/builder.h"
#include "ui/canvas.h"
#include "config.hpp"
#include "macro.hpp"
#include "stream.h"
#include "ui/color.h"
#include "utils/datatypes.h"
#include "utils/flags.h"
#include "ui/log.h"
#include "utils/misc.h"
#include "utils/modules.h"
#include "utils/stats.h"
#include "utils2/base64.h"
#include "utils2/timer.h"
#include "utils2/json.h"
#include "utils2/files.h"

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
class GyverHub : public HubStream, public HubHTTP, public HubMQTT, public HubWS {
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
        GH_DEBUG_LOG("called");
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
    }

    // остановить
    void end() {
        GH_DEBUG_LOG("called");
#if GH_HTTP_IMPL != GH_IMPL_NONE
        endWS();
        endHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        endMQTT();
#endif
        running_f = false;
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
    void onBuild(gyverhub::BuildCallback handler) {
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
            sptr->appendEscaped(label.c_str());
            *sptr += F("\":\"");
            sptr->appendEscaped(text.c_str());
            *sptr += F("\",");
        }
    }

    // ========================= CLI =========================

    // подключить обработчик входящих сообщений с веб-консоли
    void onCLI(void (*handler)(String& s)) {
        cli_cb = *handler;
    }

    // отправить текст в веб-консоль. Опционально цвет
    void print(const String& str, gyverhub::Color color = gyverhub::Colors::GH_DEFAULT) {
        if (!focused()) return;
        gyverhub::Json answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("print"));
        answ.itemString(F("text"), str);
        answ.itemInteger(F("color"), color.toHex());
        answ.end();
        _send(answ);
    }

    // ========================== STATUS ==========================

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

    // подключить обработчик запроса клиента
    void onRequest(bool (*handler)(const char* name, const char* value, GHclient nclient, GHcommand cmd)) {
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
        gyverhub::Json answ;
        _datasend(answ, data);
        _answer(answ);
    }

    // отправить сырые данные вручную (см. GyverHub.js API)
    void send(const String& data) {
        gyverhub::Json answ;
        _datasend(answ, data);
        _send(answ);
    }

    // ========================= NOTIF ==========================
    // отправить пуш уведомление
    void sendPush(const String& text) {
        if (!running_f) return;
        gyverhub::Json answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("push"));
        answ.itemString(F("text"), text);
        answ.end();
        _send(answ, true);
    }

    // отправить всплывающее уведомление
    void sendNotice(const String& text, gyverhub::Color color = gyverhub::Colors::GH_GREEN) {
        if (!running_f || !focused()) return;
        gyverhub::Json answ;
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("notice"));
        answ.itemString(F("text"), text);
        answ.itemInteger(F("color"), color.toHex());
        answ.end();
        _send(answ);
    }

    // показать окно с ошибкой
    void sendAlert(const String& text) {
        if (!running_f || !focused()) return;
        gyverhub::Json answ;
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
        if (!running_f || !build_cb || !focused()) return;

        gyverhub::Json answ;
        _updateBegin(answ);

        char* str = (char*)name.c_str();
        char* p = str;
        GH_splitter(NULL);
        while ((p = GH_splitter(str)) != NULL) {
            answ.key(p);
            answ += '\"';
            answ.reserve(answ.length() + 64);
            gyverhub::Builder::buildRead(build_cb, &answ, p);
            answ += F("\",");
        }
        answ[answ.length() - 1] = '}';
        answ.end();
        _send(answ);
    }

    // автоматически рассылать обновления клиентам при действиях на странице (умолч. true)
    void sendUpdateAuto(bool f) {
        autoUpd_f = f;
    }

    void _updateBegin(gyverhub::Json& answ) {
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("update"));
        answ.key(F("updates"));
        answ += '{';
    }
    void _sendUpdate(const char* name, const char* value) {
        if (!running_f || !focused()) return;
        gyverhub::Json answ;
        _updateBegin(answ);
        answ.itemString(name, value);
        answ += '}';
        answ.end();
        _send(answ);
    }

    // ======================= SEND CANVAS ========================
    // отправить холст
    void sendCanvas(const String& name, gyverhub::Canvas& cv) {
        if (!running_f) return;
        gyverhub::Json answ;
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
    void sendCanvasBegin(const String& name, gyverhub::Canvas& cv) {
        if (!running_f) return;
        cv.buf.clear();
        _updateBegin(cv.buf);
        cv.buf.key(name.c_str());
        cv.buf += '[';
    }

    // закончить отправку холста
    void sendCanvasEnd(gyverhub::Canvas& cv) {
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
        if (!running_f || !build_cb) return;

        char* str = (char*)name.c_str();
        char* p = str;
        GH_splitter(NULL);
        while ((p = GH_splitter(str)) != NULL) {
            gyverhub::Json value;
            if (gyverhub::Builder::buildRead(build_cb, &value, p)) sendGet(p, value);
        }
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
            GH_DEBUG_LOG("Event: GH_DISCOVER_ALL from %d", from);
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return;
        }

        GHparser<5> p(url);
        if (strcmp(p.str[0], prefix)) return;  // wrong prefix
        if (strcmp(p.str[1], id)) return;      // wrong id

        if (p.size == 2) {
            GH_DEBUG_LOG("Event: GH_DISCOVER from %d", from);
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return;
        }

        if (p.size == 3) {
            GH_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 3)", from);
            return;
        }
        // p.size >= 4

        const char* cmd = p.str[3];
        GHcommand cmdn = GH_getCmd(cmd);
        if (cmdn == GHcommand::UNKNOWN) {
            GH_DEBUG_LOG("Event: GH_UNKNOWN from %d (cmdn == -1)", from);
            return;
        }

        const char* client_id = p.str[2];
        const char* name = p.str[4];
        GHclient client(from, client_id);
        client_ptr = &client;

        if (!_reqHook(name, value, client, cmdn)) {
            answerErr(F("Forbidden"));
            return;
        }

#if GH_MQTT_IMPL != GH_IMPL_NONE
        // MQTT HOOK
        if (p.size == 4 && from == GH_MQTT && build_cb) {
             if (cmdn == GHcommand::READ) {
                GH_DEBUG_LOG("Event: GH_READ_HOOK from %d", from);
                if (modules.read(GH_MOD_READ)) sendGet(name);
                return;
            }
            
            if (cmdn == GHcommand::SET) {
                GH_DEBUG_LOG("Event: GH_SET_HOOK from %d", from);
                if (modules.read(GH_MOD_SET)) {
                    gyverhub::Builder::buildSet(build_cb, name, value, client);
                    if (autoGet_f) sendGet(name, value);
                    if (autoUpd_f) _sendUpdate(name, value);
                }
                return;
            }
        }
#endif
        setFocus(from);

        if (p.size == 4) {
            switch (cmdn) {
                case GHcommand::FOCUS:
                    GH_DEBUG_LOG("Event: GH_FOCUS from %d", from);
                    answerUI();
                    return;

                case GHcommand::PING:
                    GH_DEBUG_LOG("Event: GH_PING from %d", from);
                    answerType();
                    return;

                case GHcommand::UNFOCUS:
                    GH_DEBUG_LOG("Event: GH_UNFOCUS from %d", from);
                    answerType();
                    clearFocus(from);
                    return;

                case GHcommand::INFO:
                    GH_DEBUG_LOG("Event: GH_INFO from %d", from);
                    if (modules.read(GH_MOD_INFO)) answerInfo();
                    return;

#ifdef GH_ESP_BUILD
                case GHcommand::FSBR:
                    GH_DEBUG_LOG("Event: GH_FSBR from %d", from);
#ifndef GH_NO_FS
                    if (modules.read(GH_MOD_FSBR)) {
                        if (fs_mounted) answerFsbr();
                        else answerType(F("fs_error"));
                    } else
#endif
                    answerDsbl();
                    return;

                case GHcommand::FORMAT:
                    GH_DEBUG_LOG("Event: GH_FORMAT from %d", from);
#ifndef GH_NO_FS
                    if (modules.read(GH_MOD_FORMAT)) {
                        GH_FS.format();
                        GH_FS.end();
                        fs_mounted = GH_FS.begin();
                        answerFsbr();
                    } else
#endif
                    answerDsbl();
                    return;

                case GHcommand::REBOOT:
                    GH_DEBUG_LOG("Event: GH_REBOOT from %d", from);
                    if (modules.read(GH_MOD_REBOOT)) {
                        reboot_f = GH_REB_BUTTON;
                        answerType();
                    }
                    return;
#endif
                default:
                    GH_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 4, cmdn == %d)", from, (int) cmdn);
                    return;
            }
        }

        // p.size == 5

        switch (cmdn) {
            case GHcommand::DATA:
                GH_DEBUG_LOG("Event: GH_DATA from %d", from);
                answ_f = 0;
                if (data_cb) data_cb(name, value);
                if (!answ_f) answerType();
                return;

            case GHcommand::SET:
                GH_DEBUG_LOG("Event: GH_SET from %d", from);
                if (!build_cb || !modules.read(GH_MOD_SET)) {
                    answerType();
                } else {
                    bool mustRefresh = gyverhub::Builder::buildSet(build_cb, name, value, client);
#ifdef GH_ESP_BUILD
                    if (autoGet_f) sendGet(name, value);
#endif
                    if (autoUpd_f) _sendUpdate(name, value);
                    if (mustRefresh) answerUI();
                    else if (!autoUpd_f) answerType();
                }
                return;

            case GHcommand::CLI:
                GH_DEBUG_LOG("Event: GH_CLI from %d", from);
                answerType();
                if (cli_cb) {
                    String str(value);
                    cli_cb(str);
                }
                return;

#ifdef GH_ESP_BUILD
            case GHcommand::DELETE:
                GH_DEBUG_LOG("Event: GH_DELETE from %d", from);
#ifndef GH_NO_FS
                if (modules.read(GH_MOD_DELETE)) {
                    GH_FS.remove(name);
                    gyverhub::rmdirRecursive(name);
                    answerFsbr();
                } else
#endif
                answerDsbl();
                return;

            case GHcommand::RENAME:
                GH_DEBUG_LOG("Event: GH_RENAME from %d", from);
#ifndef GH_NO_FS
                if (modules.read(GH_MOD_RENAME)) {
                    if (GH_FS.rename(name, value))
                        answerFsbr();
                    else answerErr(F("Rename failed"));
                } else
#endif
                answerDsbl();
                return;

            case GHcommand::FETCH: {
#ifdef GH_NO_FS
                GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!modules.read(GH_MOD_FETCH)) {
                    GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (disabled on RT)", from);
                    answerDsbl();
                    return;
                }

                if (file_d || file_b) {
                    GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (busy)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                fetch_path = name;
                if (fetch_cb) fetch_cb(fetch_path, true);
                if (!file_d && !file_b) file_d = GH_FS.open(name, "r");
                if (!file_d && !file_b) {
                    GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (not found)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_FETCH from %d", from);
                fs_client = client;
                fs_tmr.reset();
                uint32_t size = file_b ? file_b_size : file_d.size();
                file_b_idx = 0;
                dwn_chunk_count = 0;
                dwn_chunk_amount = (size + GH_DOWN_CHUNK_SIZE - 1) / GH_DOWN_CHUNK_SIZE;  // round up
                answerType(F("fetch_start"));
                return;
#endif
            }

            case GHcommand::FETCH_CHUNK:
#ifdef GH_NO_FS
                GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if ((!file_d && !file_b) || fs_client != client) {
                    GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_FETCH_CHUNK from %d", from);
                fs_tmr.reset();
                answerChunk();
                dwn_chunk_count++;
                if (dwn_chunk_count < dwn_chunk_amount)
                    return;
                
                GH_DEBUG_LOG("Event: GH_FETCH_FINISH from %d", from);
                if (fetch_cb) fetch_cb(fetch_path, false);
                if (file_d) file_d.close();
                file_b = nullptr;
                fetch_path.clear();
                return;
#endif

            case GHcommand::FETCH_STOP:
#ifdef GH_NO_FS
                GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if ((!file_d && !file_b) || fs_client != client) {
                    GH_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_FETCH_ABORTED from %d", from);
                if (fetch_cb) fetch_cb(fetch_path, false);
                if (file_d) file_d.close();
                file_b = nullptr;
                fetch_path.clear();
                return;
#endif

            case GHcommand::UPLOAD:
#ifdef GH_NO_FS
                GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!modules.read(GH_MOD_UPLOAD)) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (disabled on RT)", from);
                    answerDsbl();
                    return;
                }

                if (fs_upload_file) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (busy)", from);
                    answerType(F("upload_err"));
                    return;
                }

                gyverhub::mkdirRecursive(name);
                fs_upload_file = GH_FS.open(name, "w");
                if (!fs_upload_file) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (not found)", from);
                    answerType(F("upload_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_UPLOAD from %d", from);
                fs_upload_client = client;
                fs_upload_tmr.reset();

                answerType(F("upload_start"));
                return;
#endif

            case GHcommand::UPLOAD_CHUNK: {
#ifdef GH_NO_FS
                GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!fs_upload_file || fs_upload_client != client) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("upload_err"));
                    return;
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (invalid name)", from);
                    answerType(F("upload_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_UPLOAD_CHUNK from %d", from);
                size_t len;
                uint8_t *data = gyverhub::base64Decode(value, strlen(value), len);

                if (fs_upload_file.write(data, len) != len) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (write failed)", from);
                    fs_upload_file.close();
                    answerType(F("upload_err"));
                    return;
                }

                if (isLast) {
                    GH_DEBUG_LOG("Event: GH_UPLOAD_FINISH from %d", from);
                    fs_upload_file.close();
                    answerType(F("upload_end"));
                } else {
                    fs_upload_tmr.reset();
                    answerType(F("upload_next_chunk"));
                }
                return;
#endif
            }

            case GHcommand::OTA: {
#ifdef GH_NO_OTA
                GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!modules.read(GH_MOD_OTA)) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (disabled on RT)", from);
                    answerDsbl();
                    return;
                }

                if (ota_f) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (busy)", from);
                    answerErr(F("Update already running"));
                    return;
                }

                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (invalid type)", from);
                    answerErr(F("Invalid type"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_OTA from %d", from);
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
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.begin failed)", from);
                    answerType(F("ota_err"));
                    return;
                }

                ota_client = client;
                ota_f = true;
                ota_tmr.reset();
                answerType(F("ota_start"));
                return;
#endif
            }

            case GHcommand::OTA_CHUNK: {
#ifdef GH_NO_OTA
                GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!ota_f || ota_client != client) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("ota_err"));
                    return;
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (invalid name)", from);
                    answerType(F("ota_err"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_OTA_CHUNK from %d", from);
                size_t len;
                uint8_t *data = gyverhub::base64Decode(value, strlen(value), len);
                if (Update.write(data, len) != len) {
                    GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.write failed)", from);
                    ota_f = false;
                    Update.abort();
                    answerType(F("ota_err"));
                }

                if (isLast) {
                    GH_DEBUG_LOG("Event: GH_OTA_FINISH from %d", from);
                    ota_f = false;
                    if (Update.end(true)) {
                        reboot_f = GH_REB_OTA;
                        answerType(F("ota_end"));
                    } else {
                        GH_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.end failed)", from);
                        answerType(F("ota_err"));
                    }
                } else {
                    answerType(F("ota_next_chunk"));
                    ota_tmr.reset();
                }
                return;
#endif
            }

            case GHcommand::OTA_URL: {
#ifdef GH_NO_OTA_URL
                GH_DEBUG_LOG("Event: GH_OTA_URL from %d (disabled on CT)", from);
                answerDsbl();
                return;
#else
                if (!modules.read(GH_MOD_OTA_URL)) {
                    GH_DEBUG_LOG("Event: GH_OTA_URL from %d (disabled on CT)", from);
                    answerDsbl();
                    return;
                }

                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    GH_DEBUG_LOG("Event: GH_OTA_URL from %d (invalid type)", from);
                    answerErr(F("Invalid type"));
                    return;
                }

                GH_DEBUG_LOG("Event: GH_OTA_URL from %d (start)", from);
                answerType();

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
                ota_url_f = 0;
                if (ok) {
                    GH_DEBUG_LOG("Event: GH_OTA_URL from %d (finished)", from);
                    reboot_f = GH_REB_OTA_URL;
                    answerType(F("ota_url_ok"));
                } else {
                    GH_DEBUG_LOG("Event: GH_OTA_URL from %d (failed)", from);
                    answerType(F("ota_url_err"));
                }

                return;
#endif
            }
#endif
            default:
                GH_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 5, cmdn == %d)", from, (int) cmdn);
                return;
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
        if (ota_f && ota_tmr.isTimedOut(GH_CONN_TOUT * 1000ul)) {
            GH_DEBUG_LOG("Event: GH_OTA_ABORTED from %d", ota_client.from);
            Update.abort();
            ota_f = false;
        }
#endif
#ifndef GH_NO_FS
        if (fs_upload_file && fs_upload_tmr.isTimedOut(GH_CONN_TOUT * 1000ul)) {
            GH_DEBUG_LOG("Event: GH_UPLOAD_ABORTED from %d", fs_upload_client.from);
            fs_upload_file.close();
        }

        if ((file_d || file_b) && fs_tmr.isTimedOut(GH_CONN_TOUT * 1000ul)) {
            GH_DEBUG_LOG("Event: GH_FETCH_ABORTED from %d", fs_client.from);
            if (fetch_cb) fetch_cb(fetch_path, false);
            if (file_d) file_d.close();
            file_b = nullptr;
            fetch_path = "";
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

    bool _reqHook(const char* name, const char* value, GHclient client, GHcommand event) {
        if (req_cb && !req_cb(name, value, client, event)) return 0;  // forbidden
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
        gyverhub::Json answ;
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
    void checkEndInfo(gyverhub::Json& answ, GHinfo_t info) {
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
        bool chunked = buf_size;

#ifdef GH_ESP_BUILD
        if (client_ptr->from == GH_WS || client_ptr->from == GH_MQTT) chunked = false;
#endif

        gyverhub::Json answ;
        answ.reserve((chunked ? buf_size : gyverhub::Builder::buildCount(build_cb, *client_ptr)) + 100);
        answ.begin();
        answ.key(F("controls"));
        answ += '[';
        gyverhub::Builder::buildUi(build_cb, &answ, *client_ptr, chunked ? buf_size : 0);
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
        gyverhub::Json answ;
        answ.reserve(50);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), type);
        answ.end();
        _answer(answ);
    }
    void answerErr(FSTR err) {
        gyverhub::Json answ;
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
        gyverhub::Json answ;
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

        gyverhub::Json answ;
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
        _answer(answ);
    }

    // ======================= CHUNK ========================
    void answerChunk() {
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
        gyverhub::Json answ;
        answ.reserve(GH_DOWN_CHUNK_SIZE + 100);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("fetch_next_chunk"));
        answ.itemInteger(F("chunk"), dwn_chunk_count);
        answ.itemInteger(F("amount"), dwn_chunk_amount);
        answ += F("\"data\":\"");
        if (file_b) {
            size_t len = min((size_t) file_b_size, (size_t) GH_DOWN_CHUNK_SIZE);
            size_t out_len;
            char *b64 = gyverhub::base64Encode(file_b + file_b_idx, len, file_b_pgm, out_len);
            answ.concat(b64, out_len);
            free(b64);
        } else {
            size_t len = min((size_t) file_d.available(), (size_t) GH_DOWN_CHUNK_SIZE);
            uint8_t *data = (uint8_t *)malloc(len);
            len = file_d.read(data, len);

            if (len) {
                size_t out_len;
                char *b64 = gyverhub::base64Encode(data, len, false, out_len);
                free(data);
                answ.concat(b64, out_len);
                free(b64);
            } else {
                free(data);
            }
        }
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
            case GH_MANUAL:
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
    void _datasend(gyverhub::Json& answ, const String& data) {
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
    gyverhub::BuildCallback build_cb = nullptr;
    bool (*req_cb)(const char* name, const char* value, GHclient nclient, GHcommand cmd) = nullptr;
    void (*info_cb)(GHinfo_t info) = nullptr;
    void (*cli_cb)(String& str) = nullptr;
    void (*manual_cb)(String& s, bool broadcast) = nullptr;
    GHclient* client_ptr = nullptr;
    gyverhub::Json *sptr = nullptr;

    bool running_f = 0;
    bool answ_f = 0;

    uint16_t buf_size = 0;

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
    gyverhub::Timer ota_tmr {};
    GHclient ota_client;
#endif
#ifndef GH_NO_FS
    bool fs_mounted = 0;
    // upload
    GHclient fs_upload_client;
    gyverhub::Timer fs_upload_tmr {};
    File fs_upload_file;
    // fetch
    GHclient fs_client;
    gyverhub::Timer fs_tmr {};
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