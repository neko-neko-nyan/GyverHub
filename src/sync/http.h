#pragma once
#include <Arduino.h>

#include "../config.hpp"
#include "../macro.hpp"
#include "../utils/client.h"
#include "../utils/mime.h"
#include "../utils/stats.h"
#include "../utils/modules.h"


#ifdef ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#define GH_SERVER_T ESP8266WebServer
#else
#include <WebServer.h>
#include <WiFi.h>
#define GH_SERVER_T WebServer
#ifndef GH_NO_HTTP_OTA
#include <Update.h>
#endif
#endif

#ifndef GH_NO_FS
#if (GH_FS == LittleFS)
#include <LittleFS.h>
#elif (GH_FS == SPIFFS)
#include <SPIFFS.h>
#endif
#endif

#ifndef GH_NO_DNS
#include <DNSServer.h>
#endif

#ifdef GH_INCLUDE_PORTAL
#include "../esp_inc/index.h"
#include "../esp_inc/script.h"
#include "../esp_inc/style.h"
#endif

class HubHTTP {
   public:
    GH_SERVER_T server;

    HubHTTP() : server(GH_HTTP_PORT) {}

   protected:
    void answerHTTP(const String &answ) {
        handled = true;
        server.send(200, F("text/plain"), answ);
    }

    void beginHTTP() {
        server.onNotFound([this]() {
            // command uri
            if (server.uri().startsWith(F("/hub/"))) {
                handled = false;
                parse(((char *)server.uri().c_str()) + 5, GH_HTTP);  // +5 == "/hub/"
                if (handled) return;
            }

// fetch file from GH_PUBLIC_PATH
#if !defined(GH_NO_HTTP_PUBLIC) && !defined(GH_NO_FS)
            else if (server.uri().indexOf('.') > 0) {
                String path(GH_PUBLIC_PATH);
                path += server.uri();
                if (_handleFetch(path)) return;
            }
#endif

// captive portal
#ifndef GH_NO_DNS
            else {
#if defined(GH_INCLUDE_PORTAL)
                gzip_h();
                cache_h();
                server.send_P(200, "text/html", (PGM_P)hub_index_h, (size_t)hub_index_h_len);
                return;

#elif defined(GH_FILE_PORTAL)
                File f = GH_FS.open(F("/hub/index.html.gz"), "r");
                if (f) server.streamFile(f, F("text/html"));
                else server.send(404);
                return;
#endif
            }
#endif
            server.send(404);
        });  // onNotFound

// fetch /hub/fetch?path=...
#if !defined(GH_NO_HTTP_FETCH) && !defined(GH_NO_FS)
        server.on("/hub/fetch", HTTP_GET, [this]() {
            String path = server.arg(F("path"));
            if (path.length() && _handleFetch(path)) return;
            server.send(404);
        });
#endif

// upload /hub/upload?path=...
#if !defined(GH_NO_HTTP_UPLOAD) && !defined(GH_NO_FS)
        server.on(
            "/hub/upload", HTTP_POST, [this]() { server.send(200); }, [this]() {
                HTTPUpload& upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    String path = server.arg(F("path"));
                    if (!path.length()) {
                        server.send(500);
                        return;
                    }
                    if (!_checkModule(GH_MOD_UPLOAD)) {
                        server.send(503);
                        return;
                    }
                    if (!_reqHook(path.c_str(), "", GHclient(GH_HTTP, server.arg(F("client_id")).c_str()), GH_UPLOAD)) {
                        server.send(503);
                        return;
                    }
                    sendEvent(GH_UPLOAD, GH_HTTP);
                    GH_mkdir_pc(path.c_str());
                    file = GH_FS.open(path.c_str(), "w");
                    if (!file) server.send(500);

                } else if (upload.status == UPLOAD_FILE_WRITE) {
                    if (file) {
                        size_t bytesWritten = file.write(upload.buf, upload.currentSize);
                        if (bytesWritten != upload.currentSize) server.send(500);
                    }

                } else if (upload.status == UPLOAD_FILE_END) {
                    if (file) file.close();
                } });
#endif

// upload /hub/ota?type=...
#if !defined(GH_NO_HTTP_OTA) && !defined(GH_NO_FS)
        server.on(
            "/hub/ota", HTTP_POST, [this]() {
        server.sendHeader(F("Connection"), F("close"));
        server.send(Update.hasError() ? 500 : 200);
        _rebootOTA(); },
            [this]() {
                HTTPUpload &upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    if (!_checkModule(GH_MOD_OTA)) {
                        server.send(503);
                        return;
                    }
                    if (!_reqHook("", "", GHclient(GH_HTTP, server.arg(F("client_id")).c_str()), GH_OTA)) {
                        server.send(503);
                        return;
                    }
                    sendEvent(GH_OTA, GH_HTTP);

                    String type_s(server.arg(F("type")));
                    int ota_type = 0;
                    if (type_s == F("flash")) ota_type = 1;
                    else if (type_s == F("fs")) ota_type = 2;

                    if (ota_type) {
                        size_t ota_size;
                        if (ota_type == 1) {
                            ota_type = U_FLASH;
#ifdef ESP8266
                            ota_size = (size_t)((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#else
                            ota_size = UPDATE_SIZE_UNKNOWN;
#endif
                        } else {
#ifdef ESP8266
                            ota_type = U_FS;
                            close_all_fs();
                            ota_size = (size_t)&_FS_end - (size_t)&_FS_start;
#else
                            ota_type = U_SPIFFS;
                            ota_size = UPDATE_SIZE_UNKNOWN;
#endif
                        }
                        updating = Update.begin(ota_size, ota_type);
                        if (updating) return;
                    }
                    server.send(500);

                } else if (upload.status == UPLOAD_FILE_WRITE) {
                    if (updating && Update.write(upload.buf, upload.currentSize) != upload.currentSize) server.send(500);

                } else if (upload.status == UPLOAD_FILE_END) {
                    if (updating) Update.end(true);
                }
                yield();
            });
#endif

// portal
#if defined(GH_INCLUDE_PORTAL)
        server.on("/", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/html", (PGM_P)hub_index_h, (size_t)hub_index_h_len);
        });
        server.on("/script.js", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/javascript", (PGM_P)hub_script_h, (size_t)hub_script_h_len);
        });
        server.on("/style.css", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/css", (PGM_P)hub_style_h, (size_t)hub_style_h_len);
        });

#elif defined(GH_FILE_PORTAL) && !defined(GH_NO_FS)
        server.on("/", [this]() {
            File f = GH_FS.open(F("/hub/index.html.gz"), "r");
            if (f) server.streamFile(f, F("text/html"));
            else server.send(404);
        });
        server.on("/script.js", [this]() {
            cache_h();
            File f = GH_FS.open(F("/hub/script.js.gz"), "r");
            if (f) server.streamFile(f, F("text/javascript"));
            else server.send(404);
        });
        server.on("/style.css", [this]() {
            cache_h();
            File f = GH_FS.open(F("/hub/style.css.gz"), "r");
            if (f) server.streamFile(f, F("text/css"));
            else server.send(404);
        });
#endif

// DNS for captive portal
#if defined(GH_INCLUDE_PORTAL) && !defined(GH_NO_DNS)
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            dns_f = 1;
            dns.start(53, "*", WiFi.softAPIP());
        }
#endif

        server.begin(GH_HTTP_PORT);
        server.enableCORS(true);
    }

    void endHTTP() {
        server.stop();
#ifndef GH_NO_DNS
        if (dns_f) dns.stop();
#endif
    }

    void tickHTTP() {
        server.handleClient();
#ifndef GH_NO_DNS
        if (dns_f) dns.processNextRequest();
#endif
    }

    virtual void parse(char *url, GHconn_t from) = 0;
    virtual void sendEvent(GHevent_t state, GHconn_t from) = 0;
    virtual void _rebootOTA() = 0;
    virtual bool _reqHook(const char *name, const char *value, GHclient client, GHevent_t event) = 0;
    virtual bool _checkModule(GHmodule_t mod) = 0;

#ifndef GH_NO_FS
    virtual void _fetchStartHook(String &path, File **file, const uint8_t **bytes, uint32_t *size, bool *pgm) = 0;
    virtual void _fetchEndHook(String &path) = 0;
#endif

   private:
    bool handled = false;

    void gzip_h() {
        server.sendHeader(F("Content-Encoding"), F("gzip"));
    }
    void cache_h() {
        server.sendHeader(F("Cache-Control"), F(GH_CACHE_PRD));
    }

#if !defined(GH_NO_HTTP_FETCH) && !defined(GH_NO_FS)
    bool _handleFetch(String &path) {
        if (!_checkModule(GH_MOD_FETCH)) {
            server.send(403);
            return 1;
        }
        if (!_reqHook(path.c_str(), "", GHclient(GH_HTTP, server.arg(F("client_id")).c_str()), GH_FETCH)) {
            server.send(403);
            return 1;
        }
        sendEvent(GH_FETCH, GH_HTTP);

        File *file_p = nullptr;
        const uint8_t *bytes = nullptr;
        uint32_t size = 0;
        bool pgm = 0;
        _fetchStartHook(path, &file_p, &bytes, &size, &pgm);

        if (bytes && size) {
            server.setContentLength(size);
            server.send(200, GHmime(path).c_str(), "");
            if (pgm) server.sendContent_P((PGM_P)bytes, size);
            else server.sendContent((PGM_P)bytes, size);
            _fetchEndHook(path);
            return 1;
        }

        if (file_p && *file_p) {
            server.streamFile(*file_p, GHmime(path));
            _fetchEndHook(path);
            return 1;
        }

        File f = GH_FS.open(path.c_str(), "r");
        if (f) {
            server.streamFile(f, GHmime(path));
            return 1;
        }

        return 0;
    }
#endif

#ifndef GH_NO_FS
    File file;
    bool updating = false;
#endif

#ifndef GH_NO_DNS
    bool dns_f = false;
    DNSServer dns;
#endif
};
