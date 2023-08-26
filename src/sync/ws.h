#pragma once
#include "../config.hpp"
#include "../macro.hpp"

#include <Arduino.h>
#include <WebSocketsServer.h>

#include "../utils/stats.h"

class HubWS {
    // ============ PROTECTED =============
   protected:
    HubWS() : ws(GHC_WS_PORT, "", "hub") {}

    virtual void parse(char* url, GHconn_t from) = 0;

    void beginWS() {
        ws.onEvent([this](uint8_t num, WStype_t type, uint8_t* data, GHI_UNUSED size_t len) {
            switch (type) {
                case WStype_CONNECTED:
                    GHI_DEBUG_LOG("WS connected");
                    break;

                case WStype_DISCONNECTED:
                    GHI_DEBUG_LOG("WS disconnected");
                    break;

                case WStype_ERROR:
                    GHI_DEBUG_LOG("WS error");
                    break;

                case WStype_TEXT: {
                    clientID = num;
                    /*char buf[len + 1];
                    memcpy(buf, data, len);
                    buf[len] = 0;*/
                    parse((char*)data, GH_WS);
                } break;

                default:
                    break;
            }
        });

        ws.begin();
    }

    void endWS() {
        ws.close();
    }

    void tickWS() {
        ws.loop();
    }

    void sendWS(const String& answ) {
        ws.broadcastTXT(answ.c_str(), answ.length());
    }

    void answerWS(const String& answ) {
        ws.sendTXT(clientID, answ.c_str(), answ.length());
    }

    // ============ PRIVATE =============
   private:
    WebSocketsServer ws;
    uint8_t clientID = 0;
};
