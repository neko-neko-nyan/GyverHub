#pragma once
#include "macro.hpp"

namespace gyverhub {
    // причина перезагрузки
    enum class RebootReason {
        UNKNOWN, BUTTON, OTA, OTA_URL,
        NO_REBOOT = -1
    };

    // тип подключения
    enum class ConnectionType {
        STREAM, BLUETOOTH, WEBSOCKET, HTTP, MQTT, MANUAL, 
        COUNT, 
        UNKNOWN = -1
    };

    static constexpr const size_t ConnectionTypeCount = static_cast<size_t>(ConnectionType::COUNT);

    // команда от клиента
    enum class Command : uint16_t {
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

    Command parseCommand(const char *str);
}
