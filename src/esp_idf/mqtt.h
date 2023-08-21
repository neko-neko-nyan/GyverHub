#pragma once
#include "../config.hpp"
#include "../macro.hpp"

#include <Arduino.h>

#ifndef ESP32
#error "Native MQTT only available for ESP32"
#endif

#include <mqtt_client.h>

#include "../utils/stats.h"

class HubMQTT {
private:
    esp_mqtt_client_handle_t client = nullptr;
    uint8_t qos = 0;
    bool ret = 0;

    static void _handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
        HubMQTT *self = (HubMQTT*) event_handler_arg;
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

        switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED: {
            String sub_topic(self->getPrefix());
            esp_mqtt_client_subscribe(self->client, sub_topic.c_str(), self->qos);

            sub_topic += '/';
            sub_topic += self->getID();
            sub_topic += "/#";
            esp_mqtt_client_subscribe(self->client, sub_topic.c_str(), self->qos);

            String status(self->getPrefix());
            status += "/hub/";
            status += self->getID();
            status += "/status";

            String online("online");
            self->sendMQTT(status, online);

            GH_DEBUG_LOG("MQTT connected");
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            GH_DEBUG_LOG("MQTT disconnected");
            break;

        case MQTT_EVENT_DATA: {
            char buf1[event->topic_len + 1];
            memcpy(buf1, event->topic, event->topic_len);
            buf1[event->topic_len] = 0;

            char buf[event->data_len + 1];
            memcpy(buf, event->data, event->data_len);
            buf[event->data_len] = 0;

            self->parse(buf1, buf, GH_MQTT);
            break;
        }

        default: break;
        }
    }

public:
    /**
     * Устанавливает настройки подключения по MQTT.
     * @param ip IP-адрес брокера
     * @param port Порт для подключения к брокеру
     * @param login Имя пользователя на брокере (nullptr если авторизация не требуется)
     * @param pass Пароль пользователя на брокере (nullptr если авторизация не требуется)
     * @param nqos QOS отправляемых и получаемых сообщений
     * @param nret Retain flag отправляемых сообщений
     */
    void setupMQTT(IPAddress ip, uint16_t port, const char* login = nullptr, const char* pass = nullptr, uint8_t nqos = 0, bool nret = false) {
        setupMQTT(ip.toString().c_str(), port, login, pass, nqos, nret);
    }

    /**
     * Устанавливает настройки подключения по MQTT.
     * @param host Имя хоста или IP-адрес брокера
     * @param port Порт для подключения к брокеру
     * @param login Имя пользователя на брокере (nullptr если авторизация не требуется)
     * @param pass Пароль пользователя на брокере (nullptr если авторизация не требуется)
     * @param nqos QOS отправляемых и получаемых сообщений
     * @param nret Retain flag отправляемых сообщений
     */
    void setupMQTT(const char* host, uint16_t port, const char* login = nullptr, const char* pass = nullptr, uint8_t nqos = 0, bool nret = false) {
        if (!*host) return;
        esp_mqtt_client_config_t *config = (esp_mqtt_client_config_t *) calloc(1, sizeof(esp_mqtt_client_config_t));
        config->host = host;
        config->port = port;
        config->username = login;
        config->password = pass;
        setupMQTT(config, nqos, nret);
        free(config);
    }

    /**
     * Устанавливает настройки подключения по MQTT.
     * 
     * Доступные форматы URI описаны https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/api-reference/protocols/mqtt.html#uri
     * @param uri Строка подключения в формате mqtt://user:pass@mqtt.broker.url:port
     * @param nqos QOS отправляемых и получаемых сообщений
     * @param nret Retain flag отправляемых сообщений
     */
    void setupMqttUri(const char *uri, uint8_t nqos = 0, bool nret = false) {
        if (!*uri) return;
        esp_mqtt_client_config_t *config = (esp_mqtt_client_config_t *) calloc(1, sizeof(esp_mqtt_client_config_t));
        config->uri = uri;
        setupMQTT(config, nqos, nret);
        free(config);
    }

    /**
     * Устанавливает настройки подключения по MQTT.
     * 
     * Поля стркутуры esp_mqtt_client_config_t описаны https://docs.espressif.com/projects/esp-idf/en/v4.4.5/esp32/api-reference/protocols/mqtt.html#_CPPv424esp_mqtt_client_config_t
     * @param config Указатель на структуру esp_mqtt_client_config_t.
     * @param nqos QOS отправляемых и получаемых сообщений
     * @param nret Retain flag отправляемых сообщений
     */
    void setupMQTT(esp_mqtt_client_config_t *config, uint8_t nqos, bool nret) {
        if (client != nullptr) {
            esp_mqtt_client_destroy(client);
        }

        String status(getPrefix());
        status += "/hub/";
        status += getID();
        status += "/status";

        config->lwt_topic = status.c_str();
        config->lwt_msg = "offline";
        config->lwt_msg_len = 7;
        config->lwt_qos = nqos;
        config->lwt_retain = nret;

        client = esp_mqtt_client_init(config);
        esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, HubMQTT::_handler, (void*) this);
        qos = nqos;
        ret = nret;
    }

    ~HubMQTT() {
        esp_mqtt_client_destroy(client);
        client = nullptr;
    }

protected:
    virtual void parse(char* url, const char* value, GHconn_t from) = 0;
    virtual const char* getPrefix() = 0;
    virtual const char* getID() = 0;

    void beginMQTT() {
        if (client == nullptr) {
            log_e("You must call hub.setupMQTT(...) before hub.begin()!");
            abort();
        }
        esp_mqtt_client_start(client);
    }

    void endMQTT() {
        // disconnect - не то, что нам нужно
        esp_mqtt_client_stop(client);
    }

    void sendMQTT(const String& topic, const String& msg) {
        if (client != nullptr) esp_mqtt_client_publish(client, topic.c_str(), msg.c_str(), msg.length(), qos, ret);
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
};
