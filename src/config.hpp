#pragma once

#define GH_LIB_VERSION "v0.1b"         // версия библиотеки
#define GH_PUBLIC_PATH "/www"          // путь к папке с файлами с HTTP доступом
#define GH_CONN_TOUT 5                 // таймаут соединения, с
#define GH_HTTP_PORT 80                // http порт
#define GH_WS_PORT 81                  // websocket порт
#define GH_DOWN_CHUNK_SIZE 512         // размер чанка при скачивании с платы
#define GH_UPL_CHUNK_SIZE 200          // размер чанка при загрузке на плату
#define GH_FS_DEPTH 5                  // глубина сканирования файловой системы (esp32)
#define GH_FS LittleFS                 // файловая система
#define GH_MQTT_RECONNECT 10000        // период переподключения MQTT
#define GH_CACHE_PRD "max-age=604800"  // период кеширования файлов для портала

#if (defined(ESP8266) || defined(ESP32))
#define GH_ESP_BUILD
#endif


#define GH_IMPL_NONE 0
#define GH_IMPL_SYNC 1
#define GH_IMPL_ASYNC 2
#define GH_IMPL_NATIVE 3

#ifndef GH_IMPL

#if !defined(GH_ESP_BUILD)
#define GH_IMPL GH_IMPL_NONE
#elif defined(GH_ASYNC)
#define GH_IMPL GH_IMPL_ASYNC
#else
#define GH_IMPL GH_IMPL_SYNC
#endif

#endif // !defined(GH_IMPL)

#ifndef GH_MQTT_IMPL
#define GH_MQTT_IMPL GH_IMPL
#endif

#ifndef GH_HTTP_IMPL
#define GH_HTTP_IMPL GH_IMPL
#endif
