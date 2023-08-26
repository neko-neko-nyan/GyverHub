#pragma once
#include <stdint.h>

// тип info
enum GHinfo_t {
    GH_INFO_VERSION,
    GH_INFO_NETWORK,
    GH_INFO_MEMORY,
    GH_INFO_SYSTEM,
};

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

// системные события
enum GHevent_t {
    GH_FOCUS,
    GH_PING,
    GH_UNFOCUS,
    GH_INFO,
    GH_FSBR,
    GH_FORMAT,
    GH_REBOOT,
    GH_FETCH_CHUNK,
    GH_FETCH_ABORTED,
    
    GH_DATA,
    GH_SET,
    GH_CLI,
    GH_DELETE,
    GH_RENAME,
    GH_FETCH,
    GH_UPLOAD,
    GH_UPLOAD_CHUNK,
    GH_OTA,
    GH_OTA_CHUNK,
    GH_OTA_URL,

    GH_IDLE,
    GH_START,
    GH_STOP,

    GH_CONNECTING,
    GH_CONNECTED,
    GH_DISCONNECTED,
    GH_ERROR,

    GH_UNKNOWN,
    GH_DISCOVER_ALL,
    GH_DISCOVER,

    GH_READ_HOOK,
    GH_SET_HOOK,

    GH_FETCH_ERROR,
    GH_FETCH_FINISH,

    GH_UPLOAD_ERROR,
    GH_UPLOAD_ABORTED,
    GH_UPLOAD_FINISH,

    GH_OTA_ERROR,
    GH_OTA_ABORTED,
    GH_OTA_FINISH,
};

GHcommand GH_getCmd(const char* str);
