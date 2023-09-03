#pragma once

#include "macro.hpp"
#include "utils/json.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#endif


namespace gyverhub {
    enum class InfoGroup {
        VERSION,
        NETWORK,
        MEMORY,
        SYSTEM,
    };

    class InfoBuilder;
    typedef void (*InfoCallback)(InfoBuilder*);

    class InfoBuilder {
    private:
        Json *sptr;

        static void buildGroup(InfoCallback handler, Json &answ, InfoGroup group) {
            InfoBuilder b {group, &answ};
            if (handler != nullptr) handler(&b);

            if (answ[answ.length() - 1] == ',') answ[answ.length() - 1] = '}';
            else answ.concat('}');;
        }

    public:
        const InfoGroup group;

        InfoBuilder(InfoGroup group, Json *sptr) : sptr(sptr), group(group) {}

        void addField(const String& label, const String& text) {
            addField(label.c_str(), text.c_str());
        }

        void addField(const String& label, const char *text) {
            addField(label.c_str(), text);
        }

        void addField(const String& label, FSTR text) {
            addField(label.c_str(), text);
        }

        void addField(const char *label, const String& text) {
            addField(label, text.c_str());
        }

        void addField(FSTR label, const String& text) {
            addField(label, text.c_str());
        }

#define GH__addField                                \
    sptr->concat('"');                              \
    sptr->appendEscaped(label);                     \
    sptr->concat(F("\":\""));                       \
    sptr->appendEscaped(text);                      \
    sptr->concat(F("\","));

        void addField(const char *label, const char *text) {
            GH__addField
        }

        void addField(const char *label, FSTR text) {
            GH__addField
        }

        void addField(FSTR label, const char *text) {
            GH__addField
        }

        void addField(FSTR label, FSTR text) {
            GH__addField
        }
        

        static void build(InfoCallback handler, Json &answ, const char *version = nullptr) {
            answ.concat(F("\"info\":{\"version\":{\"Library\":\"" GHC_LIB_VERSION "\","));
            if (version) {
                answ.concat(F("\"Firmware\":\""));
                answ.concat(version);
                answ.concat(F("\","));
            }
            buildGroup(handler, answ, InfoGroup::VERSION);


            answ.concat(F(",\"net\":{"));
#if GHI_ESP_BUILD
            switch (WiFi.getMode()) {
            case WIFI_AP:
                answ.concat(F("\"Mode\":\"AP\",\"IP\":\""));
                answ.concat(WiFi.localIP().toString());
                answ.concat(F("\","));
                break;

            case WIFI_STA:
                answ.concat(F("\"Mode\":\"STA\",\"IP\":\""));
                answ.concat(WiFi.softAPIP().toString());
                answ.concat(F("\","));
                break;

            case WIFI_AP_STA:
                answ.concat(F("\"Mode\":\"AP+STA\",\"IP\":\""));
                answ.concat(WiFi.localIP().toString());
                answ.concat(F("\",\"AP IP\":\""));
                answ.concat(WiFi.softAPIP().toString());
                answ.concat(F("\","));
                break;
            
            default: break;
            }
            answ.concat(F("\"MAC\":\""));
            answ.concat(WiFi.macAddress());
            answ.concat(F("\",\"SSID\":\""));
            answ.appendEscaped(WiFi.SSID());
            answ.concat(F("\",\"RSSI\":\""));
            answ.concat(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
            answ.concat(F("%\","));
#endif
            buildGroup(handler, answ, InfoGroup::NETWORK);
            

            answ.concat(F(",\"memory\":{"));

#if GHI_ESP_BUILD
            answ.concat(F("\"RAM\":["));
            answ.concat(ESP.getFreeHeap());
            answ.concat(F(",0],"));

#if GHC_FS != GHC_FS_NONE
            answ.concat(F("\"FS\":["));
#ifdef ESP8266
            FSInfo fs_info;
            GHI_FS.info(fs_info);
            answ.concat(fs_info.usedBytes);
            answ.concat(',');
            answ.concat(fs_info.totalBytes);
#else
            answ.concat(GHI_FS.usedBytes());
            answ.concat(',');
            answ.concat(GHI_FS.totalBytes());
#endif
            answ.concat(F("],"));
#endif
    
            answ.concat(F("\"Sketch\":["));
            answ.concat(ESP.getSketchSize());
            answ.concat(',');
            answ.concat(ESP.getFreeSketchSpace());
            answ.concat(F("],"));
#endif
            buildGroup(handler, answ, InfoGroup::MEMORY);


            answ.concat(F(",\"system\":{\"Uptime\":"));
            answ.concat(millis() / 1000ul);
            answ.concat(F(",\"Platform\":\"" GHI_PLATFORM_STR "\","));

#if GHI_ESP_BUILD
            answ.concat(F("\"CPU freq\":"));
            answ.concat(ESP.getCpuFreqMHz());
            answ.concat(F(",\"Flash chip size\":\""));
            answ.concat(String(ESP.getFlashChipSize() / 1000.0, 1));
            answ.concat(F(" kB\","));
#endif

            buildGroup(handler, answ, InfoGroup::SYSTEM);
            answ.concat(F("}}\n"));
        }
    };
}
