#pragma once
#include <stdint.h>

// причина перезагрузки
enum GHreason_t {
    GH_REB_NONE,
    GH_REB_BUTTON,
    GH_REB_OTA,
    GH_REB_OTA_URL,
};

// тип подключения
enum GHconn_t {
    GH_SERIAL,
    GH_BT,
    GH_WS,
    GH_HTTP,
    GH_MQTT,
    GH_MANUAL,
    GH_SYSTEM,
};

#define GH_CONN_AMOUNT 6


// команда от клиента
enum class GHcommand : uint16_t {
    FOCUS = 0,
    PING,
    UNFOCUS,
    INFO,
    FSBR,
    FORMAT,
    REBOOT,
    DATA,
    SET,
    CLI,
    DELETE,
    RENAME,
    FETCH,
    FETCH_CHUNK,
    FETCH_STOP,
    UPLOAD,
    UPLOAD_CHUNK,
    OTA,
    OTA_CHUNK,
    OTA_URL,
    READ,

    HTTP_FETCH = 0xF000,
    HTTP_UPLOAD,
    HTTP_OTA,

    UNKNOWN = 0xFFFF,
};

GHcommand GH_getCmd(const char* str);
