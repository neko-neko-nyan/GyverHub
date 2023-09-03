#pragma once
#include "hub/types.h"
#include <ESPAsyncWebServer.h>

class HubWS {
    // ============ PROTECTED =============
   protected:
    HubWS() : server(GHC_WS_PORT), ws("/") {
        server.addHandler(&ws);
    }

    virtual void parse(char* url, gyverhub::ConnectionType from) = 0;

    void beginWS() {
        ws.onEvent([this](GHI_UNUSED AsyncWebSocket* server, GHI_UNUSED AsyncWebSocketClient* client, AwsEventType etype, void* arg, uint8_t* data, size_t len) {
            switch (etype) {
                case WS_EVT_CONNECT:
                    GHI_DEBUG_LOG("WS connected");
                    break;

                case WS_EVT_DISCONNECT:
                    GHI_DEBUG_LOG("WS disconnected");
                    break;

                case WS_EVT_ERROR:
                    GHI_DEBUG_LOG("WS error");
                    break;

                case WS_EVT_DATA: {
                    AwsFrameInfo* ws_info = (AwsFrameInfo*)arg;
                    if (ws_info->final && ws_info->index == 0 && ws_info->len == len && ws_info->opcode == WS_TEXT) {
                        clientID = client->id();
                        parse((char*)data, gyverhub::ConnectionType::WEBSOCKET);
                    }
                } break;

                default:
                    break;
            }
        });

        server.begin();
    }

    void endWS() {
        server.end();
    }

    void tickWS() {
        ws.cleanupClients();
    }

    void sendWS(const String& answ) {
        ws.textAll(answ.c_str());
    }

    void answerWS(const String& answ) {
        ws.text(clientID, answ.c_str());
    }

    // ============ PRIVATE =============
   private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    uint32_t clientID = 0;
};
