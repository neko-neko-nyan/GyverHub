#pragma once
#include "../config.hpp"
#include "../macro.hpp"

#include <Arduino.h>
#include <PubSubClient.h>

#include "../utils/stats.h"

class HubMQTT {
    // ============ PUBLIC =============
   public:
    // настроить MQTT (хост брокера, порт, логин, пароль, QoS, retained)
    void setupMQTT(const char* host, uint16_t port, const char* login = nullptr, const char* pass = nullptr, uint8_t nqos = 0, bool nret = 0) {
        if (!strlen(host)) return;
        mqtt.setServer(host, port);
        _setupMQTT(login, pass, nqos, nret);
    }

    void setupMQTT(IPAddress ip, uint16_t port, const char* login = nullptr, const char* pass = nullptr, uint8_t nqos = 0, bool nret = 0) {
        mqtt.setServer(ip, port);
        _setupMQTT(login, pass, nqos, nret);
    }

    // MQTT подключен
    bool online() {
        return mqtt.connected();
    }

    // ============ PROTECTED =============
   protected:
    virtual void parse(char* url, const char *value, GHconn_t from) = 0;
    virtual const char* getPrefix() = 0;
    virtual const char* getID() = 0;
    virtual void sendEvent(GHevent_t state, GHconn_t from) = 0;

    void beginMQTT() {
        mqtt.setCallback([this](char* topic, uint8_t* data, uint16_t len) {
            char buf[len + 1];
            memcpy(buf, data, len);
            buf[len] = 0;
            parse(topic, buf, GH_MQTT);
        });
    }

    void endMQTT() {
        mqtt.disconnect();
    }

    void tickMQTT() {
        if (mq_configured) {
            if (!mqtt.connected() && (!mqtt_tmr || millis() - mqtt_tmr > GH_MQTT_RECONNECT)) {
                mqtt_tmr = millis();
                sendEvent(GH_CONNECTING, GH_MQTT);
                connectMQTT();
            }
            mqtt.loop();
        }
    }

    void sendMQTT(const String& topic, const String& msg) {
        if (!mqtt.connected()) return;
        mqtt.beginPublish(topic.c_str(), msg.length(), ret);
        mqtt.write((uint8_t*)msg.c_str(), msg.length());
        mqtt.endPublish();
    }

    void sendMQTT(const String& msg) {
        String topic(getPrefix());
        topic += F("/hub");
        sendMQTT(topic, msg);
    }

    void answerMQTT(const String& msg, const char* hubID) {
        String topic(getPrefix());
        topic += F("/hub/");
        topic += hubID;
        topic += '/';
        topic += getID();
        sendMQTT(topic, msg);
    }

    // ============ PRIVATE =============
   private:
    void connectMQTT() {
        String m_id("DEV-");
        m_id += String(random(0xffffff), HEX);
        bool ok = 0;

        String status(getPrefix());
        status += F("/hub/");
        status += getID();
        status += F("/status");

        String offline(F("offline"));

        if (mq_login) ok = mqtt.connect(m_id.c_str(), mq_login, mq_pass, status.c_str(), qos, ret, offline.c_str());
        else ok = mqtt.connect(m_id.c_str(), status.c_str(), qos, ret, offline.c_str());
        // if (mq_login) from = mqtt.connect(m_id.c_str(), mq_login, mq_pass);
        // else from = mqtt.connect(m_id.c_str());

        if (ok) {
            String online(F("online"));
            sendMQTT(status, online);

            String sub_topic(getPrefix());
            mqtt.subscribe(sub_topic.c_str(), qos);

            sub_topic += '/';
            sub_topic += getID();
            sub_topic += "/#";
            mqtt.subscribe(sub_topic.c_str(), qos);
        }
        sendEvent(ok ? GH_CONNECTED : GH_ERROR, GH_MQTT);
    }
    void _setupMQTT(const char* login, const char* pass, uint8_t nqos, bool nret) {
        mqtt.setClient(mclient);
        qos = nqos;
        ret = nret;
        mq_login = login;
        mq_pass = pass;
        mq_configured = true;
    }

    PubSubClient mqtt;
    WiFiClient mclient;
    bool mq_configured = false;
    uint32_t mqtt_tmr = 0;
    uint8_t qos = 0;
    bool ret = 0;
    const char* mq_login;
    const char* mq_pass;
};
