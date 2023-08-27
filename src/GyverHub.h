#pragma once

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
#include "utils/stats.h"
#include "utils2/base64.h"
#include "utils2/timer.h"
#include "utils2/json.h"
#include "utils2/files.h"
#include "hub/info.h"

#if GHI_ESP_BUILD
#include <FS.h>
#include "hub/fetch.h"
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#if GHI_MOD_ENABLED(GH_MOD_OTA_URL)
#include <ESP8266httpUpdate.h>
#endif
#endif

#ifdef ESP32
#include <WiFi.h>
#if GHI_MOD_ENABLED(GH_MOD_OTA)
#include <Update.h>
#endif
#if GHI_MOD_ENABLED(GH_MOD_OTA_URL)
#include <HTTPUpdate.h>
#endif
#endif

#if !GHI_ESP_BUILD
#define SIGRD 5
#include <avr/boot.h>
#endif

#if GHC_FS == GHC_FS_LITTLEFS
#include <LittleFS.h>
#elif GHC_FS == GHC_FS_SPIFFS
#include <SPIFFS.h>
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
    void setupMQTT(GHI_UNUSED const char* host, GHI_UNUSED uint16_t port, GHI_UNUSED const char* login = nullptr, GHI_UNUSED const char* pass = nullptr,
     GHI_UNUSED uint8_t nqos = 0, GHI_UNUSED bool nret = 0) {}
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
    /**
     * @param prefix Префикс
     * @param name Название устройства
     * @param icon Иконка устройства
     * @param id ID устройства. По умолчаню на ESP32/ESP8266 генерируется из MAC адреса WiFi интерфейса, на AVR (arduino) - из сигнатуры МК.
     */
    GyverHub(const char* prefix = "", const char* name = "", const char* icon = "", uint32_t id = 0) {
        config(prefix, name, icon, id);
    }

    /**
     * Перенастроить устройство
     * @param prefix Префикс
     * @param name Название устройства
     * @param icon Иконка устройства
     * @param id ID устройства. По умолчаню на ESP32/ESP8266 генерируется из MAC адреса WiFi интерфейса, на AVR (arduino) - из сигнатуры МК.
     */
    void config(const char* prefix, const char* name, const char* icon = "", uint32_t id = 0) {
        this->prefix = prefix;
        this->name = name;
        this->icon = icon;
        if (!id) {
#if GHI_ESP_BUILD
            uint8_t mac[6];
            WiFi.macAddress(mac);
            id = *((uint32_t*)(mac + 2));
#else
            id |= boot_signature_byte_get(0);
            id <<= 8;
            id |= boot_signature_byte_get(1);
            id <<= 8;
            id |= boot_signature_byte_get(2);
            id <<= 8;
            id |= boot_signature_byte_get(3);
#endif
        }
        ultoa(id, this->id, HEX);
    }

    /// установить версию прошивки для отображения в Info и OTA
    void setVersion(const char* version) {
        this->version = version;
    }

    /// установить пин-код для открытия устройства (значение больше 1000, не может начинаться с 000..)
    void setPIN(uint16_t pin) {
        if (pin > 9999) return;
        uint32_t hash = 0;

        char pin_s[11];
        ultoa(pin, pin_s, 10);

        uint16_t len = strlen(pin_s);
        for (uint16_t i = 0; i < len; i++) {
            hash = ((hash << 5u) - hash) + pin_s[i];
        }

        this->pinHash = hash;
        
        // uint16_t pin2 = 0;
        // for (int i = 0; i < 4; i++) {
        //     pin2 *= 10;
        //     pin2 += pin % 10;
        //     pin /= 10;
        // }

        // do {
        //     hash = (hash << 5u) - hash + '0' + (pin2 % 10);
        //     pin2 /= 10;
        // } while (pin2 != 0);
    }

    /// установить пин-код для открытия устройства (значение больше 1000, не может начинаться с 000..)
    void removePin(uint32_t pin) {
        this->pinHash = 0;
    }

    /**
     * Установить размер буфера строки для сборки интерфейса при ручной отправке.
     * 0 - интерфейс будет собран и отправлен цельной строкой
     * >0 - пакет будет отправляться частями
     */ 
    void setBufferSize(uint16_t size) {
        buf_size = size;
    }

    /// автоматически рассылать обновления клиентам при действиях на странице (умолч. true)
    void sendUpdateAuto(bool f) {
        autoUpd_f = f;
    }

#if GH_MQTT_IMPL != GH_IMPL_NONE

    /// автоматически отправлять новое состояние на get-топик при изменении через set (умолч. false)
    void sendGetAuto(bool v) {
        autoGet_f = v;
    }

#endif


    // ========================= Checks =========================

    /// вернёт true, если система запущена
    bool running() {
        return running_f;
    }

    /// true - интерфейс устройства сейчас открыт на сайте или в приложении
    bool focused() {
        if (!running_f) return 0;
        for (uint8_t i = 0; i < GH_CONN_AMOUNT; i++) {
            if (focus_arr[i]) return 1;
        }
        return 0;
    }

    /// проверить фокус по указанному типу связи
    bool focused(GHconn_t from) {
        return focus_arr[from];
    }


    // ========================= Handlers =========================

    /// подключить функцию-сборщик интерфейса
    void onBuild(gyverhub::BuildCallback handler) {
        build_cb = handler;
    }

    /// подключить функцию-обработчик запроса при ручном соединении
    void onManual(void (*handler)(const String& s, bool broadcast)) {
        manual_cb = handler;
    }

    /// подключить функцию-сборщик инфо
    void onInfo(gyverhub::InfoCallback handler) {
        info_cb = handler;
    }

    /// подключить обработчик входящих сообщений с веб-консоли
    void onCLI(void (*handler)(String& s)) {
        cli_cb = *handler;
    }

#if GHI_ESP_BUILD

    /// подключить функцию-обработчик перезагрузки. Будет вызвана перед перезагрузкой
    void onReboot(void (*handler)(GHreason_t)) {
        reboot_cb = handler;
    }

#endif

    /// подключить обработчик запроса клиента
    void onRequest(bool (*handler)(const char* name, const char* value, GHclient nclient, GHcommand cmd)) {
        req_cb = handler;
    }

    /// подключить обработчик данных (см. GyverHub.js API)
    void onData(void (*handler)(const char* name, const char* value)) {
        data_cb = handler;
    }

#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_FETCH)

    /// подключить обработчик скачивания
    void onFetch(gyverhub::FetchCallback handler) {
        fetch.setCallback(handler);
    }

#endif


#if GH_MQTT_IMPL != GH_IMPL_NONE

    // ========================== ON/OFF ==========================

    // отправить MQTT LWT команду на включение
    void turnOn() {
        _power(F("online"));
    }

    // отправить MQTT LWT команду на выключение
    void turnOff() {
        _power(F("offline"));
    }

private:
    void _power(FSTR mode) {
        if (!running_f) return;

        String topic(prefix);
        topic += F("/hub/");
        topic += id;
        topic += F("/status");
        sendMQTT(topic, mode);
    }
public:

#endif


    // ========================= DATA ==========================

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

private:
    void _datasend(gyverhub::Json& answ, const String& data) {
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("data"));
        answ.itemString(F("data"), data);
        answ.end();
    }
public:

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

    // отправить update вручную с указанием значения
    void sendUpdate(const String& name, const String& value) {
        sendUpdate(name.c_str(), value.c_str());
    }

    // отправить update вручную с указанием значения
    void sendUpdate(const char* name, const char* value) {
        if (!running_f || !focused()) return;
        gyverhub::Json answ;
        _updateBegin(answ);
        answ.itemString(name, value);
        answ += '}';
        answ.end();
        _send(answ);
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

private:
    void _updateBegin(gyverhub::Json& answ) {
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("update"));
        answ.key(F("updates"));
        answ += '{';
    }
public:

    // ======================== SEND GET =========================

#if GH_MQTT_IMPL != GH_IMPL_NONE

    // отправить имя-значение на get-топик (MQTT)
    void sendGet(GHI_UNUSED const String& name, GHI_UNUSED const String& value) {
        if (!running_f) return;
        String topic(prefix);
        topic += F("/hub/");
        topic += id;
        topic += F("/get/");
        topic += name;
        sendMQTT(topic, value);
    }

    // отправить значение по имени компонента на get-топик (MQTT) (значение будет прочитано в build). Имена можно передать списком через запятую
    void sendGet(GHI_UNUSED const String& name) {
        if (!running_f || !build_cb) return;

        char* str = (char*)name.c_str();
        char* p = str;
        GH_splitter(NULL);
        while ((p = GH_splitter(str)) != NULL) {
            gyverhub::Json value;
            if (gyverhub::Builder::buildRead(build_cb, &value, p)) sendGet(p, value);
        }
    }

#endif

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

#if !GHI_MOD_ENABLED(GH_MOD_SERIAL)
        if (from == GH_SERIAL) return;
#endif
#if !GHI_MOD_ENABLED(GH_MOD_BT)
        if (from == GH_BT) return;
#endif
#if !GHI_MOD_ENABLED(GH_MOD_WS)
        if (from == GH_WS) return;
#endif
#if !GHI_MOD_ENABLED(GH_MOD_HTTP)
        if (from == GH_HTTP) return;
#endif
#if !GHI_MOD_ENABLED(GH_MOD_MQTT)
        if (from == GH_MQTT) return;
#endif

#if GHI_ESP_BUILD && !defined(GH_NO_FS) && !defined(GH_NO_OTA) && !defined(GH_NO_OTA_URL)
        if (ota_url_f) return;
#endif

        if (!strcmp(url, prefix)) {  // == prefix
            GHI_DEBUG_LOG("Event: GH_DISCOVER_ALL from %d", from);
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return;
        }

        GHparser<5> p(url);
        if (strcmp(p.str[0], prefix)) return;  // wrong prefix
        if (strcmp(p.str[1], id)) return;      // wrong id

        if (p.size == 2) {
            GHI_DEBUG_LOG("Event: GH_DISCOVER from %d", from);
            GHclient client(from, value);
            client_ptr = &client;
            answerDiscover();
            return;
        }

        if (p.size == 3) {
            GHI_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 3)", from);
            return;
        }
        // p.size >= 4

        const char* cmd = p.str[3];
        GHcommand cmdn = GH_getCmd(cmd);
        if (cmdn == GHcommand::UNKNOWN) {
            GHI_DEBUG_LOG("Event: GH_UNKNOWN from %d (cmdn == -1)", from);
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

#if GH_MQTT_IMPL != GH_IMPL_NONE && (GHI_MOD_ENABLED(GH_MOD_READ) || GHI_MOD_ENABLED(GH_MOD_SET))
        // MQTT HOOK
        if (p.size == 4 && from == GH_MQTT && build_cb) {
#if GHI_MOD_ENABLED(GH_MOD_READ)
             if (cmdn == GHcommand::READ) {
                GHI_DEBUG_LOG("Event: GH_READ_HOOK from %d", from);
                sendGet(name);
                return;
            }
#endif

#if GHI_MOD_ENABLED(GH_MOD_SET)
            if (cmdn == GHcommand::SET) {
                GHI_DEBUG_LOG("Event: GH_SET_HOOK from %d", from);
                gyverhub::Builder::buildSet(build_cb, name, value, client);
                if (autoGet_f) sendGet(name, value);
                if (autoUpd_f) sendUpdate(name, value);
                return;
            }
#endif
        }
#endif
        setFocus(from);

        if (p.size == 4) {
            switch (cmdn) {
                case GHcommand::FOCUS:
                    GHI_DEBUG_LOG("Event: GH_FOCUS from %d", from);
                    answerUI();
                    return;

                case GHcommand::PING:
                    GHI_DEBUG_LOG("Event: GH_PING from %d", from);
                    answerType();
                    return;

                case GHcommand::UNFOCUS:
                    GHI_DEBUG_LOG("Event: GH_UNFOCUS from %d", from);
                    answerType();
                    clearFocus(from);
                    return;
#if GHI_MOD_ENABLED(GH_MOD_INFO)
                case GHcommand::INFO:
                    GHI_DEBUG_LOG("Event: GH_INFO from %d", from);
                    answerInfo();
                    return;
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_FSBR)
                case GHcommand::FSBR:
                    GHI_DEBUG_LOG("Event: GH_FSBR from %d", from);
                    if (fs_mounted) answerFsbr();
                    else answerType(F("fs_error"));
                    return;
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_FORMAT)
                case GHcommand::FORMAT:
                    GHI_DEBUG_LOG("Event: GH_FORMAT from %d", from);
                    GHI_FS.format();
                    GHI_FS.end();
                    fs_mounted = GHI_FS.begin();
                    answerFsbr();
                    return;
#endif
#if GHI_ESP_BUILD && GHI_MOD_ENABLED(GH_MOD_REBOOT)
                case GHcommand::REBOOT:
                    GHI_DEBUG_LOG("Event: GH_REBOOT from %d", from);
                    reboot_f = GH_REB_BUTTON;
                    answerType();
                    return;
#endif
                default:
                    GHI_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 4, cmdn == %d)", from, (int) cmdn);
                    answerDsbl();
                    return;
            }
        }

        // p.size == 5

        switch (cmdn) {
            case GHcommand::DATA:
                GHI_DEBUG_LOG("Event: GH_DATA from %d", from);
                if (data_cb) data_cb(name, value);
                answerType();
                return;
#if GHI_MOD_ENABLED(GH_MOD_SET)
            case GHcommand::SET: {
                GHI_DEBUG_LOG("Event: GH_SET from %d", from);
                if (!build_cb) {
                    answerType();
                    return;
                }
                
                bool mustRefresh = gyverhub::Builder::buildSet(build_cb, name, value, client);
#if GH_MQTT_IMPL != GH_IMPL_NONE
                if (autoGet_f) sendGet(name, value);
#endif
                if (autoUpd_f) sendUpdate(name, value);
                if (mustRefresh) answerUI();
                else if (!autoUpd_f) answerType();
                return;
            }
#endif
            case GHcommand::CLI:
                GHI_DEBUG_LOG("Event: GH_CLI from %d", from);
                answerType();
                if (cli_cb) {
                    String str(value);
                    cli_cb(str);
                }
                return;
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_DELETE)
            case GHcommand::DELETE:
                GHI_DEBUG_LOG("Event: GH_DELETE from %d", from);
                GHI_FS.remove(name);
                gyverhub::rmdirRecursive(name);
                answerFsbr();
                return;
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_RENAME)
            case GHcommand::RENAME:
                GHI_DEBUG_LOG("Event: GH_RENAME from %d", from);
                if (GHI_FS.rename(name, value))
                    answerFsbr();
                else answerErr(F("Rename failed"));
                return;
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_FETCH)
            case GHcommand::FETCH: {
                if (fetch.isActive()) {
                    GHI_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (busy)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                if (!fetch.open(name)) {
                    GHI_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (not found)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_FETCH from %d", from);
                fs_client = client;
                fs_tmr.reset();
                answerType(F("fetch_start"));
                return;
            }

            case GHcommand::FETCH_CHUNK:
                if (!fetch.isActive() || fs_client != client) {
                    GHI_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_FETCH_CHUNK from %d", from);
                fs_tmr.reset();
                answerChunk();
                GHI_DEBUG_LOG("Event: GH_FETCH_FINISH from %d", from);
                fetch.close();
                return;

            case GHcommand::FETCH_STOP:
                if (!fetch.isActive() || fs_client != client) {
                    GHI_DEBUG_LOG("Event: GH_FETCH_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("fetch_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_FETCH_ABORTED from %d", from);
                fetch.close();
                return;
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_UPLOAD)
            case GHcommand::UPLOAD:
                if (fs_upload_file) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (busy)", from);
                    answerType(F("upload_err"));
                    return;
                }

                gyverhub::mkdirRecursive(name);
                fs_upload_file = GHI_FS.open(name, "w");
                if (!fs_upload_file) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (not found)", from);
                    answerType(F("upload_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_UPLOAD from %d", from);
                fs_upload_client = client;
                fs_upload_tmr.reset();

                answerType(F("upload_start"));
                return;

            case GHcommand::UPLOAD_CHUNK: {
                if (!fs_upload_file || fs_upload_client != client) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("upload_err"));
                    return;
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (invalid name)", from);
                    answerType(F("upload_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_UPLOAD_CHUNK from %d", from);
                size_t len;
                uint8_t *data = gyverhub::base64Decode(value, strlen(value), len);

                if (fs_upload_file.write(data, len) != len) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_ERROR from %d (write failed)", from);
                    fs_upload_file.close();
                    answerType(F("upload_err"));
                    return;
                }

                if (isLast) {
                    GHI_DEBUG_LOG("Event: GH_UPLOAD_FINISH from %d", from);
                    fs_upload_file.close();
                    answerType(F("upload_end"));
                } else {
                    fs_upload_tmr.reset();
                    answerType(F("upload_next_chunk"));
                }
                return;
            }
#endif
#if GHI_ESP_BUILD && GHI_MOD_ENABLED(GH_MOD_OTA)
            case GHcommand::OTA: {
                if (ota_f) {
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (busy)", from);
                    answerErr(F("Update already running"));
                    return;
                }

                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (invalid type)", from);
                    answerErr(F("Invalid type"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_OTA from %d", from);
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
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.begin failed)", from);
                    answerType(F("ota_err"));
                    return;
                }

                ota_client = client;
                ota_f = true;
                ota_tmr.reset();
                answerType(F("ota_start"));
                return;
            }

            case GHcommand::OTA_CHUNK: {
                if (!ota_f || ota_client != client) {
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (closed or wrong clid)", from);
                    answerType(F("ota_err"));
                    return;
                }

                bool isLast = strcmp_P(name, PSTR("last")) == 0;
                if (!isLast && strcmp_P(name, PSTR("next")) != 0) {
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (invalid name)", from);
                    answerType(F("ota_err"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_OTA_CHUNK from %d", from);
                size_t len;
                uint8_t *data = gyverhub::base64Decode(value, strlen(value), len);
                if (Update.write(data, len) != len) {
                    GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.write failed)", from);
                    ota_f = false;
#ifdef ESP32
                    Update.abort();
#else
                    Update.begin(0);
#endif
                    answerType(F("ota_err"));
                }

                if (isLast) {
                    GHI_DEBUG_LOG("Event: GH_OTA_FINISH from %d", from);
                    ota_f = false;
                    if (Update.end(true)) {
                        reboot_f = GH_REB_OTA;
                        answerType(F("ota_end"));
                    } else {
                        GHI_DEBUG_LOG("Event: GH_OTA_ERROR from %d (Update.end failed)", from);
                        answerType(F("ota_err"));
                    }
                } else {
                    answerType(F("ota_next_chunk"));
                    ota_tmr.reset();
                }
                return;
            }
#endif
#if GHI_ESP_BUILD && GHI_MOD_ENABLED(GH_MOD_OTA_URL)
            case GHcommand::OTA_URL: {
                bool isFlash = strcmp_P(name, PSTR("flash")) == 0;
                if (!isFlash && strcmp_P(name, PSTR("fs")) != 0) {
                    GHI_DEBUG_LOG("Event: GH_OTA_URL from %d (invalid type)", from);
                    answerErr(F("Invalid type"));
                    return;
                }

                GHI_DEBUG_LOG("Event: GH_OTA_URL from %d (start)", from);
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
                    GHI_DEBUG_LOG("Event: GH_OTA_URL from %d (finished)", from);
                    reboot_f = GH_REB_OTA_URL;
                    answerType(F("ota_url_ok"));
                } else {
                    GHI_DEBUG_LOG("Event: GH_OTA_URL from %d (failed)", from);
                    answerType(F("ota_url_err"));
                }

                return;
            }
#endif
            default:
                GHI_DEBUG_LOG("Event: GH_UNKNOWN from %d (size == 5, cmdn == %d)", from, (int) cmdn);
                answerDsbl();
                return;
        }
    }


    // ========================== SETUP ==========================

    // запустить
    void begin() {
        GHI_DEBUG_LOG("called");
#if GH_HTTP_IMPL != GH_IMPL_NONE
        beginWS();
        beginHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        beginMQTT();
#endif

#if GHC_FS != GHC_FS_NONE
#ifdef ESP8266
        fs_mounted = GHI_FS.begin();
#else
        fs_mounted = GHI_FS.begin(true);
#endif
#endif
        running_f = true;
    }

    // остановить
    void end() {
        GHI_DEBUG_LOG("called");
#if GH_HTTP_IMPL != GH_IMPL_NONE
        endWS();
        endHTTP();
#endif
#if GH_MQTT_IMPL != GH_IMPL_NONE
        endMQTT();
#endif
        running_f = false;
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

#if GHI_ESP_BUILD && GHI_MOD_ENABLED(GH_MOD_OTA)
        if (ota_f && ota_tmr.isTimedOut(GHC_CONN_TOUT * 1000ul)) {
            GHI_DEBUG_LOG("Event: GH_OTA_ABORTED from %d", ota_client.from);
#ifdef ESP32
            Update.abort();
#else
            Update.begin(0);
#endif
            ota_f = false;
        }
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_UPLOAD)
        if (fs_upload_file && fs_upload_tmr.isTimedOut(GHC_CONN_TOUT * 1000ul)) {
            GHI_DEBUG_LOG("Event: GH_UPLOAD_ABORTED from %d", fs_upload_client.from);
            fs_upload_file.close();
        }
#endif
#if GHC_FS != GHC_FS_NONE && GHI_MOD_ENABLED(GH_MOD_FETCH)
        if (fetch.isActive() && fs_tmr.isTimedOut(GHC_CONN_TOUT * 1000ul)) {
            GHI_DEBUG_LOG("Event: GH_FETCH_ABORTED from %d", fs_client.from);
            fetch.close();
        }
#endif
#if GHI_ESP_BUILD
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
#if GHI_ESP_BUILD
        reboot_f = GH_REB_OTA;
#endif
    }
    const char* getPrefix() {
        return prefix;
    }
    const char* getID() {
        return id;
    }

    bool _reqHook(const char* name, const char* value, GHclient client, GHcommand event) {
        if (req_cb && !req_cb(name, value, client, event)) return 0;  // forbidden
        return 1;
    }
    bool _checkModule(uint32_t mod) {
        return GHI_MOD_ENABLED(mod);
    }

#if GHC_FS != GHC_FS_NONE
    void _fetchStartHook(String& path, File** file, const uint8_t** bytes, uint32_t* size, bool* pgm) {
        if (!fetch.isActive() && fetch.open(path.c_str()))
            fetch.getData(file, bytes, size, pgm);
    }
    void _fetchEndHook(String& path) {
        fetch.close();
    }
#endif

    // ======================= INFO ========================
    void answerInfo() {
        gyverhub::Json answ;
        answ.reserve(250);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("info"));

        gyverhub::InfoBuilder::build(info_cb, answ, version);

        answ.end();
        _answer(answ);
    }

    // ======================= UI ========================
    void answerUI() {
        // TODO переделать
        // Хак для локальной функции
        static GyverHub *self;
        self = this;
        struct L {
            static void _send1(const String &answ1) {
                self->_answer(answ1, false);
            };
        };

        if (!build_cb) return answerType();
        bool chunked = buf_size;

#if GHI_ESP_BUILD
        if (client_ptr->from == GH_WS || client_ptr->from == GH_MQTT) chunked = false;
#endif

        gyverhub::Json answ;
        answ.reserve((chunked ? buf_size : gyverhub::Builder::buildCount(build_cb, *client_ptr)) + 100);
        answ.begin();
        answ.key(F("controls"));
        answ += '[';
        gyverhub::Builder::buildUi(build_cb, &answ, *client_ptr, chunked ? buf_size : 0, L::_send1);
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
#if GHC_FS != GHC_FS_NONE
        gyverhub::Json answ;
        answ.reserve(100);
        
        uint16_t count = 0;
        GH_showFiles(answ, "/", GHC_FS_MAX_DEPTH, &count);

        answ.clear();
        answ.reserve(count + 50);
        answ.begin();
        answ.key(F("fs"));
        answ += '{';
        answ.itemInteger(F("/"), 0);
        GH_showFiles(answ, "/", GHC_FS_MAX_DEPTH);
        answ[answ.length() - 1] = '}';  // ',' = '}'
        answ += ',';

        answ.appendId(id);
        answ.itemString(F("type"), F("fsbr"));

#ifdef ESP8266
        FSInfo fs_info;
        GHI_FS.info(fs_info);
        answ.itemInteger(F("total"), fs_info.totalBytes);
        answ.itemInteger(F("used"), fs_info.usedBytes);
#else
        answ.itemInteger(F("total"), GHI_FS.totalBytes());
        answ.itemInteger(F("used"), GHI_FS.usedBytes());
#endif
        answ.end();
        _answer(answ);
#else
        answerDsbl();
#endif
    }

    // ======================= DISCOVER ========================
    void answerDiscover() {
        gyverhub::Json answ;
        answ.reserve(120);
        answ.begin();
        answ.appendId(id);
        answ.itemString(F("type"), F("discover"));
        answ.itemString(F("name"), name);
        answ.itemString(F("icon"), icon);
        answ.itemInteger(F("PIN"), pinHash);
        answ.itemString(F("version"), version);
        answ.itemInteger(F("max_upl"), GHC_UPLOAD_CHUNK_SIZE);
#ifdef ATOMIC_FS_UPDATE
        answ.itemString(F("ota_t"), F("gz"));
#else
        answ.itemString(F("ota_t"), F("bin"));
#endif
        answ.itemInteger(F("modules"), GHC_MODS_DISABLED);
        answ.end();
        _answer(answ);
    }

    // ======================= CHUNK ========================
    void answerChunk() {
#if GHC_FS != GHC_FS_NONE
        gyverhub::Json answ;
        answ.reserve(GHC_FETCH_CHUNK_SIZE + 100);
        answ.begin();
        answ.appendId(id);
        fetch.nextChunk(answ);
        answ.end();
        _answer(answ);
#endif
    }

    // ======================= ANSWER ========================
    void _answer(const String& answ, bool close = true) {
        if (!client_ptr) return;
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
        if (close) client_ptr = nullptr;
    }

    // ======================= SEND ========================
    void _send(const String& answ, bool broadcast = false) {
        client_ptr = nullptr;
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

    // ========================== MISC ==========================
    void setFocus(GHconn_t from) {
        focus_arr[from] = GHC_CONN_TOUT;
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
    uint32_t pinHash = 0;
    char id[9];

    void (*data_cb)(const char* name, const char* value) = nullptr;
    gyverhub::BuildCallback build_cb = nullptr;
    bool (*req_cb)(const char* name, const char* value, GHclient nclient, GHcommand cmd) = nullptr;
    gyverhub::InfoCallback info_cb = nullptr;
    void (*cli_cb)(String& str) = nullptr;
    void (*manual_cb)(const String& s, bool broadcast) = nullptr;
    GHclient* client_ptr = nullptr;

    bool running_f = 0;

    uint16_t buf_size = 0;

    uint16_t focus_tmr = 0;
    int8_t focus_arr[GH_CONN_AMOUNT] = {};
    bool autoUpd_f = true;

#if GHI_ESP_BUILD
    void (*reboot_cb)(GHreason_t r) = nullptr;
#if GH_MQTT_IMPL != GH_IMPL_NONE
    bool autoGet_f = 0;
#endif
    GHreason_t reboot_f = GH_REB_NONE;

#if GHI_MOD_ENABLED(GH_MOD_OTA_URL)
    bool ota_url_f = 0;
#endif
#if GHI_MOD_ENABLED(GH_MOD_OTA)
    bool ota_f = false;
    gyverhub::Timer ota_tmr {};
    GHclient ota_client;
#endif
#endif
#if GHC_FS != GHC_FS_NONE
    bool fs_mounted = 0;
    // upload
    GHclient fs_upload_client;
    gyverhub::Timer fs_upload_tmr {};
    File fs_upload_file;
    // fetch
    GHclient fs_client;
    gyverhub::Timer fs_tmr {};
    gyverhub::FetchBuilder fetch {};
#endif
};
