#pragma once
#ifndef GHI_IMPL_SELECT
# error Never include implementation-specific files directly, use "impl/impl_select.h"
#endif
#include "macro.hpp"
#if !GHI_ESP_BUILD
# error This implementation only available on ESP32 or ESP8266
#endif
#if !__has_include(<ESPAsyncWebServer.h>)
# error Missing dependency: ESPAsyncWebServer
#endif
#include "hub/types.h"
#include "hub/portal.h"
#include "utils/mime.h"
#include "utils/files.h"
#include <ESPAsyncWebServer.h>

#ifndef GH_NO_HTTP_OTA
#ifdef ESP32
#include <Update.h>
#endif
#endif

#if GHI_MOD_ENABLED(GH_MOD_DNS)
#include <DNSServer.h>
#endif

#define GH_HTTP_UPLOAD "1"
#define GH_HTTP_FETCH "1"
#define GH_HTTP_OTA "1"

#ifdef GH_NO_HTTP_UPLOAD
#define GH_HTTP_UPLOAD "0"
#endif
#ifdef GH_NO_HTTP_FETCH
#define GH_HTTP_FETCH "0"
#endif
#ifdef GH_NO_HTTP_OTA
#define GH_HTTP_OTA "0"
#endif

#define GH__ENABLE_DNS ((GHI_MOD_ENABLED(GH_MOD_PORTAL) || (GHI_MOD_ENABLED(GH_MOD_FILE_PORTAL) && GHC_FS != GHC_FS_NONE)) && GHI_MOD_ENABLED(GH_MOD_DNS))

class HubHTTP {
    // ============ PUBLIC =============
   public:
    AsyncWebServer server;

    HubHTTP() : server(GHC_HTTP_PORT) {}

    // ============ PROTECTED =============
   protected:
    void beginHTTP() {
        server.on("/hub_discover_all", HTTP_GET, [this](AsyncWebServerRequest* req) {
            AsyncWebServerResponse* resp = req->beginResponse(200, F("text/plain"), F("OK"));
            req->send(resp);
        });

#if GHC_FS != GHC_FS_NONE
        server.on("/hub_http_cfg", HTTP_GET, [this](AsyncWebServerRequest* req) {
            AsyncWebServerResponse* resp = req->beginResponse(200, F("text/plain"), F("{\"upload\":" GH_HTTP_UPLOAD ",\"download\":" GH_HTTP_FETCH ",\"ota\":" GH_HTTP_OTA ",\"path\":\"" GHC_PUBLIC_PATH "\"}"));
            req->send(resp);
        });

#ifndef GH_NO_HTTP_FETCH
        server.on(GHC_PUBLIC_PATH "*", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(GHI_FS, request->url(), gyverhub::getMimeByPath(request->url().c_str(), request->url().length()));
            request->send(response);
        });
#endif

#ifndef GH_NO_HTTP_UPLOAD
        server.on(
            "/upload", HTTP_POST, [this](AsyncWebServerRequest* request) { request->send(200, F("text/plain"), F("OK")); },
            [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
                if (!index) {
                    gyverhub::mkdirRecursive(filename.c_str());
                    file = GHI_FS.open(filename, "w");
                    if (!file) {
                        AsyncWebServerResponse* resp = request->beginResponse(500, F("text/plain"), F("FAIL"));
                        request->send(resp);
                    }
                }
                if (len) {
                    if (file) {
                        size_t bytesWritten = file.write(data, len);
                        if (bytesWritten != len) {
                            AsyncWebServerResponse* resp = request->beginResponse(500, F("text/plain"), F("FAIL"));
                            request->send(resp);
                        }
                    }
                }
                if (final) {
                    if (file) file.close();
                }
            });
#endif

#ifndef GH_NO_HTTP_OTA
        server.on(
            "/ota", HTTP_POST, [this](AsyncWebServerRequest* request) { 
                AsyncWebServerResponse* resp = request->beginResponse(200, F("text/plain"), Update.hasError() ? F("FAIL") : F("OK"));
                request->send(resp);
                _rebootOTA();
                },
            [this](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
                if (!index) {
                    int ota_type = 0;
                    if (request->params()) {
                        AsyncWebParameter* p = request->getParam(0);
                        if (!strcmp_P(p->value().c_str(), PSTR("flash"))) ota_type = 1;
                        else if (!strcmp_P(p->value().c_str(), PSTR("fs"))) ota_type = 2;
                    }

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
#ifdef ESP8266
                        Update.runAsync(true);
#endif
                        Update.begin(ota_size, ota_type);
                    }
                }
                if (len) {
                    Update.write(data, len);
                }
                if (final) {
                    Update.end(true);
                }
            });
#endif
#endif

#if GHI_MOD_ENABLED(GH_MOD_FILE_PORTAL) && GHC_FS != GHC_FS_NONE
        server.on("/favicon.svg", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            request->send(response);
        });
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(GHI_FS, "/hub/index.html.gz", "text/html");
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
        server.on("/script.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(GHI_FS, "/hub/script.js.gz", "text/javascript");
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
        server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(GHI_FS, "/hub/style.css.gz", "text/css");
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
#elif GHI_MOD_ENABLED(GH_MOD_PORTAL)
        server.on("/favicon.svg", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200);
            request->send(response);
        });
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", gyverhub::portal::index, gyverhub::portal::index_size);
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
        server.on("/script.js", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, "text/javascript", gyverhub::portal::script, gyverhub::portal::script_size);
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
        server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, "text/css", gyverhub::portal::style, gyverhub::portal::style_size);
            gzip_h(response);
            cache_h(response);
            request->send(response);
        });
#endif

// DNS for captive portal
#if GH__ENABLE_DNS
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
            dns_f = 1;
            dns.start(53, "*", WiFi.softAPIP());
        }
#if GHI_MOD_ENABLED(GH_MOD_FILE_PORTAL) && GHC_FS != GHC_FS_NONE
        server.onNotFound([this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(GHI_FS, "/hub/index.html.gz", "text/html");
            gzip_h(response);
            request->send(response);
        });
#else
        server.onNotFound([this](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", gyverhub::portal::index, gyverhub::portal::index_size);
            gzip_h(response);
            request->send(response);
        });
#endif
#endif
        CORS();
        server.begin();
    }

    void answerHTTP(const String &answ) {
        // TODO
    }

    void endHTTP() {
        server.end();
#if GH__ENABLE_DNS
        if (dns_f) dns.stop();
#endif
    }

    void tickHTTP() {
#if GH__ENABLE_DNS
        if (dns_f) dns.processNextRequest();
#endif
    }

   protected:
    virtual void _rebootOTA() = 0;

    // ============ PRIVATE =============
   private:
    void CORS() {
        DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
        DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Private-Network"), F("true"));
        DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("*"));
    }
    void gzip_h(AsyncWebServerResponse* response) {
        response->addHeader("Content-Encoding", "gzip");
    }
    void cache_h(AsyncWebServerResponse* response) {
        response->addHeader(F("Cache-Control"), GHC_PORTAL_CACHE);
    }
    File file;

#if GH__ENABLE_DNS
    bool dns_f = false;
    DNSServer dns;
#endif
};

#undef GH__ENABLE_DNS
