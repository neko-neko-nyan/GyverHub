#pragma once
#ifndef GHI_IMPL_SELECT
# error Never include implementation-specific files directly, use "impl/impl_select.h"
#endif
#include "macro.hpp"
#if !defined(ESP32)
# error This implementation only available on ESP32
#endif
#include "hub/types.h"
#include <esp_http_server.h>

class HubWS {
private:
    struct async_resp_arg {
        httpd_handle_t hd;
        int fd;
        char *text;
    };

    httpd_handle_t server = NULL;

    static esp_err_t handler(httpd_req_t *req) {
        HubWS *self = (HubWS *) req->user_ctx;
        return self->_handler2(req);
    }

    esp_err_t _handler2(httpd_req_t *req) {
        if (req->method == HTTP_GET) {
            GHI_DEBUG_LOG("WS connected");
            return ESP_OK;
        }

        ESP_LOGW("ws", "Data");
        httpd_ws_frame_t ws_pkt;
        uint8_t *buf = NULL;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        /* Set max_len = 0 to get the frame len */
        esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
        if (ret != ESP_OK) {
            ESP_LOGE("ws", "httpd_ws_recv_frame failed to get frame len with %d", ret);
            return ret;
        }

        ESP_LOGI("ws", "frame len is %d", ws_pkt.len);

        if (ws_pkt.len) {
            /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
            buf = (uint8_t*) calloc(1, ws_pkt.len + 1);
            if (buf == NULL) {
                ESP_LOGE("ws", "Failed to calloc memory for buf");
                return ESP_ERR_NO_MEM;
            }

            ws_pkt.payload = buf;
            /* Set max_len = ws_pkt.len to get the frame payload */
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret != ESP_OK) {
                ESP_LOGE("ws", "httpd_ws_recv_frame failed with %d", ret);
                free(buf);
                return ret;
            }
            ESP_LOGI("ws", "Got packet with message: %s", ws_pkt.payload);
        }

        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            // clientID = client->id();
            ESP_LOGI("ws", "Processing data ");
            parse((char*)ws_pkt.payload, gyverhub::ConnectionType::WEBSOCKET);
        }

        free(buf);
        return ret;
    }

    static void send_hello(void *arg) {
        async_resp_arg *resp_arg = (async_resp_arg *) arg;
        httpd_handle_t hd = resp_arg->hd;
        int fd = resp_arg->fd;
        httpd_ws_frame_t ws_pkt;
        memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
        ws_pkt.payload = (uint8_t*)resp_arg->text;
        ws_pkt.len = strlen(resp_arg->text);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        ESP_LOGI("ws", "Sending %s", resp_arg->text);
        esp_err_t ret = httpd_ws_send_frame_async(hd, fd, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE("ws", "httpd_ws_send_frame_async failed with %d", ret);
        }

        // free(resp_arg->text);
        free(resp_arg);
    }

protected:
    virtual void parse(char* url, gyverhub::ConnectionType from) = 0;

    void beginWS() {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 81;
        config.ctrl_port = 81;

        // Start the httpd server
        if (httpd_start(&server, &config) != ESP_OK) {
            log_e("Ws server start failed");
            abort();
        }
        
        httpd_uri_t *_u = (httpd_uri_t *)calloc(1, sizeof(httpd_uri_t));
        _u->uri = "/";
        _u->method = HTTP_GET;
        _u->handler = HubWS::handler;
        _u->user_ctx = (void*) this;
        _u->is_websocket = true;
        _u->supported_subprotocol = "hub";
        httpd_register_uri_handler(server, _u);
        free(_u);
    }

    void endWS() {
        httpd_stop(server);
        server = nullptr;
    }

    void tickWS() {}

    void sendWS(const String& answ) {
        size_t clients = 16;
        int    client_fds[16];
        if (httpd_get_client_list(server, &clients, client_fds) != ESP_OK)
            return;
        
        for (size_t i=0; i < clients; ++i) {
            int sock = client_fds[i];
            if (httpd_ws_get_fd_info(server, sock) != HTTPD_WS_CLIENT_WEBSOCKET)
                continue;
                
            async_resp_arg *resp_arg = (async_resp_arg *) malloc(sizeof(async_resp_arg));
            resp_arg->hd = server;
            resp_arg->fd = sock;
            resp_arg->text = strdup(answ.c_str());
            if (httpd_queue_work(resp_arg->hd, send_hello, resp_arg) != ESP_OK) {
                break;
            }
        }
    }

    void answerWS(const String& answ) {
        sendWS(answ);
    }
};
