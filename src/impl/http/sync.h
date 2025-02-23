#pragma once
#ifndef GHI_IMPL_SELECT
# error Never include implementation-specific files directly, use "impl/impl_select.h"
#endif
#include "macro.hpp"
#if !GHI_ESP_BUILD
# error This implementation only available on ESP32 or ESP8266
#endif
#ifdef ESP32
#if !__has_include(<WebServer.h>)
# error Missing dependency: WebServer
#endif
#else
#if !__has_include(<ESP8266WebServer.h>)
# error Missing dependency: ESP8266WebServer
#endif
#endif
#include "hub/types.h"
#include "hub/client.h"
#include "utils/mime.h"
#include "utils/files.h"
#include "hub/portal.h"

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

#if GHC_DNS_SERVER
#include <DNSServer.h>
#endif

class HubHTTP {
   public:
    GH_SERVER_T server;

    HubHTTP() : server(GHC_HTTP_PORT) {}

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
                parse(((char *)server.uri().c_str()) + 5, gyverhub::ConnectionType::HTTP);  // +5 == "/hub/"
                if (handled) return;
            }

// fetch file from GHC_PUBLIC_PATH
#if !defined(GH_NO_HTTP_PUBLIC) && GHC_FS != GHC_FS_NONE
            else if (server.uri().indexOf('.') > 0) {
                String path(GHC_PUBLIC_PATH);
                path += server.uri();
                if (_handleFetch(path)) return;
            }
#endif

// captive portal
#if GHC_DNS_SERVER
            else {
                gzip_h();
#if GHC_PORTAL == GHC_PORTAL_FS
                File f = GHI_FS.open(F("/hub/index.html.gz"), "r");
                if (f) server.streamFile(f, F("text/html"));
                else server.send(404);
#else
                server.send_P(200, "text/html", (PGM_P)gyverhub::portal::index, gyverhub::portal::index_size);
#endif
                return;
            }
#endif
            server.send(404);
        });  // onNotFound

// fetch /hub/fetch?path=...
#if !defined(GH_NO_HTTP_FETCH) && GHC_FS != GHC_FS_NONE
        server.on("/hub/fetch", HTTP_GET, [this]() {
            String path = server.arg(F("path"));
            if (path.length() && _handleFetch(path)) return;
            server.send(404);
        });
#endif

// upload /hub/upload?path=...
#if !defined(GH_NO_HTTP_UPLOAD) && GHC_FS != GHC_FS_NONE
        server.on(
            "/hub/upload", HTTP_POST, [this]() { server.send(200); }, [this]() {
                HTTPUpload& upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    String path = server.arg(F("path"));
                    if (!path.length()) {
                        server.send(500);
                        return;
                    }
                    if (!_reqHook(path.c_str(), "", GHclient(gyverhub::ConnectionType::HTTP, server.arg(F("client_id")).c_str()), gyverhub::Command::HTTP_UPLOAD)) {
                        server.send(503);
                        return;
                    }
                    GHI_DEBUG_LOG("HTTP upload");
                    gyverhub::mkdirRecursive(path.c_str());
                    file = GHI_FS.open(path.c_str(), "w");
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
#if !defined(GH_NO_HTTP_OTA) && GHC_FS != GHC_FS_NONE
        server.on(
            "/hub/ota", HTTP_POST, [this]() {
        server.sendHeader(F("Connection"), F("close"));
        server.send(Update.hasError() ? 500 : 200);
        _rebootOTA(); },
            [this]() {
                HTTPUpload &upload = server.upload();
                if (upload.status == UPLOAD_FILE_START) {
                    if (!_reqHook("", "", GHclient(gyverhub::ConnectionType::HTTP, server.arg(F("client_id")).c_str()), gyverhub::Command::HTTP_OTA)) {
                        server.send(503);
                        return;
                    }
                    GHI_DEBUG_LOG("HTTP ota");

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
                            ota_size = (size_t)FS_end - (size_t)FS_start;
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
#if GHC_PORTAL == GHC_PORTAL_FS
        server.on("/", [this]() {
            File f = GHI_FS.open(F("/hub/index.html.gz"), "r");
            if (f) server.streamFile(f, F("text/html"));
            else server.send(404);
        });
        server.on("/script.js", [this]() {
            cache_h();
            File f = GHI_FS.open(F("/hub/script.js.gz"), "r");
            if (f) server.streamFile(f, F("text/javascript"));
            else server.send(404);
        });
        server.on("/style.css", [this]() {
            cache_h();
            File f = GHI_FS.open(F("/hub/style.css.gz"), "r");
            if (f) server.streamFile(f, F("text/css"));
            else server.send(404);
        });
#elif GHC_PORTAL == GHC_PORTAL_BUILTIN
        server.on("/", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/html", (PGM_P)gyverhub::portal::index, gyverhub::portal::index_size);
        });
        server.on("/script.js", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/javascript", (PGM_P)gyverhub::portal::script, gyverhub::portal::script_size);
        });
        server.on("/style.css", [this]() {
            gzip_h();
            cache_h();
            server.send_P(200, "text/css", (PGM_P)gyverhub::portal::style, gyverhub::portal::style_size);
        });
#endif

// DNS for captive portal
#if GHC_DNS_SERVER
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            dnsEnabled =  dns.start(53, "*", WiFi.softAPIP());
        }
#endif

        server.begin(GHC_HTTP_PORT);
        server.enableCORS(true);
    }

    void endHTTP() {
        server.stop();
#if GHC_DNS_SERVER
        if (dnsEnabled) dns.stop();
#endif
    }

    void tickHTTP() {
        server.handleClient();
#if GHC_DNS_SERVER
        if (dnsEnabled) dns.processNextRequest();
#endif
    }

    virtual void parse(char *url, gyverhub::ConnectionType from) = 0;
    virtual void _rebootOTA() = 0;
    virtual bool _reqHook(const char *name, const char *value, GHclient client, gyverhub::Command cmd) = 0;

#if GHC_FS != GHC_FS_NONE
    virtual void _fetchStartHook(String &path, File **file, const uint8_t **bytes, uint32_t *size, bool *pgm) = 0;
    virtual void _fetchEndHook() = 0;
#endif

   private:
    bool handled = false;

    void gzip_h() {
        server.sendHeader(F("Content-Encoding"), F("gzip"));
    }
    void cache_h() {
        server.sendHeader(F("Cache-Control"), F(GHC_PORTAL_CACHE));
    }

#if !defined(GH_NO_HTTP_FETCH) && GHC_FS != GHC_FS_NONE
    bool _handleFetch(String &path) {
        if (!_reqHook(path.c_str(), "", GHclient(gyverhub::ConnectionType::HTTP, server.arg(F("client_id")).c_str()), gyverhub::Command::HTTP_FETCH)) {
            server.send(403);
            return 1;
        }
        GHI_DEBUG_LOG("HTTP fetch");

        File *file_p = nullptr;
        const uint8_t *bytes = nullptr;
        uint32_t size = 0;
        bool pgm = 0;
        _fetchStartHook(path, &file_p, &bytes, &size, &pgm);

        if (bytes && size) {
            server.setContentLength(size);
            server.send(200, gyverhub::getMimeByPath(path.c_str(), path.length()), "");
            if (pgm) server.sendContent_P((PGM_P)bytes, size);
            else server.sendContent((const char *) bytes, size);
            _fetchEndHook();
            return 1;
        }

        if (file_p && *file_p) {
            server.streamFile(*file_p, gyverhub::getMimeByPath(path.c_str(), path.length()));
            _fetchEndHook();
            return 1;
        }

        File f = GHI_FS.open(path.c_str(), "r");
        if (f) {
            server.streamFile(f, gyverhub::getMimeByPath(path.c_str(), path.length()));
            return 1;
        }

        return 0;
    }
#endif

#if GHC_FS != GHC_FS_NONE
    File file;
    bool updating = false;
#endif

#if GHC_DNS_SERVER
    bool dnsEnabled = false;
    DNSServer dns;
#endif
};

