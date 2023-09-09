#pragma once
#ifndef GHI_IMPL_SELECT
# error Never include implementation-specific files directly, use "impl/impl_select.h"
#endif
#include "macro.hpp"
#if !defined(ESP32)
# error This implementation only available on ESP32
#endif
#include "hub/types.h"
#include "hub/portal.h"
#include "utils/mime.h"
#include "utils/files.h"

#include <esp_http_server.h>
#include <fcntl.h>

#ifndef GH_NO_OTA
#include <Update.h>
#endif

#define GH_HTTP_UPLOAD "1"
#define GH_HTTP_DOWNLOAD "1"
#define GH_HTTP_OTA "1"

#ifdef GH_NO_HTTP_UPLOAD
#define GH_HTTP_UPLOAD "0"
#endif
#ifdef GH_NO_HTTP_DOWNLOAD
#define GH_HTTP_DOWNLOAD "0"
#endif
#ifdef GH_NO_HTTP_OTA
#define GH_HTTP_OTA "0"
#endif

#define SCRATCH_BUFSIZE 8192

class HubHTTP {
private:
    httpd_handle_t server = NULL;
    char buffer[SCRATCH_BUFSIZE];

    static esp_err_t setCorsHeaders(httpd_req_t *req) {
        esp_err_t res = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        if (res != ESP_OK) return res;

        res = httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "*");
        if (res != ESP_OK) return res;
        
        res = httpd_resp_set_hdr(req, "Access-Control-Allow-Private-Network", "true");
        return res;
    }

    static esp_err_t handlerSendString(httpd_req_t *req) {
        esp_err_t res = setCorsHeaders(req);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_type(req, "application/json");
        if (res != ESP_OK) return res;
        
        return httpd_resp_sendstr(req, (const char *) req->user_ctx);
    }

    static esp_err_t handlerSendBinData(httpd_req_t *req) {
        String *data = (String*) req->user_ctx;
        
        esp_err_t res = setCorsHeaders(req);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        if (res != ESP_OK) return res;

        res = httpd_resp_set_hdr(req, "Cache-Control", GHC_PORTAL_CACHE);
        if (res != ESP_OK) return res;

        const char *p = strrchr(req->uri, '?');
        size_t length = p ? p - req->uri : strlen(req->uri);

        res = httpd_resp_set_type(req, gyverhub::getMimeByPath(req->uri, length));
        if (res != ESP_OK) return res;

        return httpd_resp_send(req, data->c_str(), data->length());
    }

    static esp_err_t handlerSendFile(httpd_req_t *req) {
        const char *name = (const char*) req->user_ctx;

        int fd = open(name, O_RDONLY);
        // File file = GHI_FS.open(name);

        esp_err_t res = setCorsHeaders(req);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_hdr(req, "Cache-Control", GHC_PORTAL_CACHE);
        if (res != ESP_OK) return res;

        bool gzip;
        res = httpd_resp_set_type(req, gyverhub::getMimeByPath(name, strlen(name), &gzip));
        if (res != ESP_OK) return res;
        
        if (gzip) {
            res = httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
            if (res != ESP_OK) return res;
        }

        char *buf = (char*)malloc(SCRATCH_BUFSIZE);
        size_t chunksize;
        do {
            chunksize = read(fd, buf, SCRATCH_BUFSIZE);
            // chunksize = file.read((uint8_t*) buf, SCRATCH_BUFSIZE);

            if (chunksize > 0) {
                if (httpd_resp_send_chunk(req, buf, chunksize) != ESP_OK) {
                    close(fd);
                    free(buf);
                    // file.close();
                    
                    httpd_resp_sendstr_chunk(req, NULL);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                    return ESP_FAIL;
                }
            }
        } while (chunksize != 0);

        close(fd);
        free(buf);
        // file.close();
        return httpd_resp_send_chunk(req, NULL, 0);
    }

    static esp_err_t handlerDownload(httpd_req_t *req) {
        HubHTTP *self = (HubHTTP*) req->user_ctx;

        File file = GHI_FS.open(req->uri);  // TODO: uri
        if (!file) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
            return ESP_FAIL;
        }
        
        esp_err_t res = setCorsHeaders(req);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_type(req, gyverhub::getMimeByPath(file.path(), strlen(file.path())));
        if (res != ESP_OK) return res;

        {
            char buf[16];
            sprintf(buf, "%d", file.size());
            res = httpd_resp_set_hdr(req, "Content-Length", buf);
            if (res != ESP_OK) return res;
        }

        char *buf = self->buffer;
        size_t chunksize;
        do {
            chunksize = file.read((uint8_t*) buf, SCRATCH_BUFSIZE);

            if (chunksize > 0) {
                if (httpd_resp_send_chunk(req, buf, chunksize) != ESP_OK) {
                    file.close();
                    
                    httpd_resp_sendstr_chunk(req, NULL);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                    return ESP_FAIL;
                }
            }
        } while (chunksize != 0);

        file.close();
        return httpd_resp_send_chunk(req, NULL, 0);
    }

    static esp_err_t handlerUpload(httpd_req_t *req) {
        HubHTTP *self = (HubHTTP*) req->user_ctx;

        size_t buf_size = httpd_req_get_url_query_len(req) + 1;
        char *filename = (char*) malloc(buf_size);
        filename[buf_size-1] = '\0';
        esp_err_t res = httpd_req_get_url_query_str(req, filename, buf_size);
        if (res != ESP_OK) {
            free(filename);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing query string");
            return res;
        }
        Serial.println(filename);

        gyverhub::mkdirRecursive(filename);
        File file = GHI_FS.open(filename, "w");
        if (!file) {
            free(filename);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
            return ESP_FAIL;
        }

        char *buf = self->buffer;
        int remaining = req->content_len;
        int received;

        while (remaining > 0) {
            if ((received = httpd_req_recv(req, buf, min(remaining, SCRATCH_BUFSIZE))) <= 0) {
                if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue;
                }

                file.close();
                GHI_FS.remove(filename);
                free(filename);

                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
                return ESP_FAIL;
            }

            if (received && (received != file.write((uint8_t*) buf, received))) {
                file.close();
                GHI_FS.remove(filename);
                free(filename);

                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
                return ESP_FAIL;
            }

            remaining -= received;
        }

        file.close();
        free(filename);
        
        res = setCorsHeaders(req);
        if (res != ESP_OK) return res;
        
        res = httpd_resp_set_type(req, "text/plain");
        if (res != ESP_OK) return res;
        
        return httpd_resp_sendstr(req, "OK");
    }

    static esp_err_t handlerOta(httpd_req_t *req) {
        HubHTTP *self = (HubHTTP*) req->user_ctx;

        int ota_type = 0;
        char type[6] = {};

        {
            size_t buf_size = httpd_req_get_url_query_len(req) + 1;
            char *buf = (char*) malloc(buf_size);
            buf[buf_size-1] = '\0';
            esp_err_t res = httpd_req_get_url_query_str(req, buf, buf_size);
            if (res != ESP_OK) {
                free(buf);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing query string");
                return res;
            }

            res = httpd_query_key_value(buf, "type", type, sizeof(type));
            if (res != ESP_OK) {
                free(buf);
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing type field");
                return res;
            }
            free(buf);
        }

        if (strncmp(type, "flash", sizeof(type)) == 0) ota_type = 1;
        else if (strncmp(type, "fs", sizeof(type)) == 0) ota_type = 2;

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

        char *buf = self->buffer;
        int remaining = req->content_len;
        int received;

        while (remaining > 0) {
            if ((received = httpd_req_recv(req, buf, min(remaining, SCRATCH_BUFSIZE))) <= 0) {
                if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue;
                }

                Update.abort();

                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive update");
                return ESP_FAIL;
            }

            if (received && (received != Update.write((uint8_t*) buf, received))) {
                Update.abort();

                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
                return ESP_FAIL;
            }

            remaining -= received;
        }
        
        Update.end(true);
        self->_rebootOTA();
        
        esp_err_t res = setCorsHeaders(req);
        if (res != ESP_OK) return res;

        res = httpd_resp_set_type(req, "text/plain");
        if (res != ESP_OK) return res;
        
        return httpd_resp_sendstr(req, Update.hasError() ? "FAIL" : "OK");
    }

protected:
    virtual void _rebootOTA() = 0;


#define GH__SETH(_method, _uri, _handler, _arg) do {                                \
    httpd_uri_t *_u = (httpd_uri_t *)calloc(1, sizeof(httpd_uri_t));                \
    _u->uri = (_uri);                                                               \
    _u->method = (_method);                                                         \
    _u->handler = (_handler);                                                       \
    _u->user_ctx = (void*) (_arg);                                                  \
    httpd_register_uri_handler(server, _u);                                         \
    free(_u);                                                                       \
    } while (0)


    void beginHTTP() {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = config.max_resp_headers = 10;
        config.uri_match_fn = httpd_uri_match_wildcard;
        // config.lru_purge_enable = true;

        if (httpd_start(&server, &config) != ESP_OK) {
            log_e("HTTPD server start failed");
            abort();
        }

        GH__SETH(HTTP_GET, "/hub_discover_all", HubHTTP::handlerSendString, "OK");
        

#if GHC_FS != GHC_FS_NONE
        GH__SETH(HTTP_GET, "/hub_http_cfg", HubHTTP::handlerSendString, "{\"upload\":" GH_HTTP_UPLOAD ",\"download\":" GH_HTTP_DOWNLOAD ",\"ota\":" GH_HTTP_OTA ",\"path\":\"" GHC_PUBLIC_PATH "\"}");

#ifndef GH_NO_HTTP_DOWNLOAD
        GH__SETH(HTTP_GET, GHC_PUBLIC_PATH "/*", HubHTTP::handlerDownload, this);
#endif

#ifndef GH_NO_HTTP_UPLOAD
        GH__SETH(HTTP_POST, "/upload", HubHTTP::handlerUpload, this);
        
#endif

#ifndef GH_NO_HTTP_OTA
        GH__SETH(HTTP_POST, "/ota", HubHTTP::handlerOta, this);
#endif
#endif


#if GHC_PORTAL == GHC_PORTAL_FS
        GH__SETH(HTTP_GET, "/favicon.svg", HubHTTP::handlerSendString, "");
        GH__SETH(HTTP_GET, "/", HubHTTP::handlerSendFile, "/hub/index.html.gz");
        GH__SETH(HTTP_GET, "/script.js", HubHTTP::handlerSendFile, "/hub/script.js.gz");
        GH__SETH(HTTP_GET, "/style.css", HubHTTP::handlerSendFile, "/hub/style.css.gz");
#elif GHC_PORTAL == GHC_PORTAL_BUILTIN
        GH__SETH(HTTP_GET, "/favicon.svg", HubHTTP::handlerSendString, "");
        GH__SETH(HTTP_GET, "/", HubHTTP::handlerSendBinData, new String(gyverhub::portal::index, gyverhub::portal::index_size));
        GH__SETH(HTTP_GET, "/script.js", HubHTTP::handlerSendBinData, new String(gyverhub::portal::script, gyverhub::portal::script_size));
        GH__SETH(HTTP_GET, "/style.css", HubHTTP::handlerSendBinData, new String(gyverhub::portal::style, gyverhub::portal::style_size));
#endif


#undef GH__SETH
    }

    void answerHTTP(const String &answ) {
        // TODO
    }
    
    void endHTTP() {
        httpd_stop(server);
        server = nullptr;
    }

    void tickHTTP() {}
};
