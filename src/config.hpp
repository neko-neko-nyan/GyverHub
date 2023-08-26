/**
 * config.h - конфигурация GyverHub
 */
#pragma once

/// Отладка библиотеки
#define GHC_LIB_DEBUG 0

/// Файловая система
#define GHC_FS GHC_FS_LITTLEFS

/// версия библиотеки
#define GHC_LIB_VERSION "v0.1b"

// таймаут соединения, с
#define GHC_CONN_TOUT 5

// период переподключения MQTT
#define GHC_MQTT_RECONNECT 10000

// размер чанка при скачивании с платы
#define GHC_FETCH_CHUNK_SIZE 512

// размер чанка при загрузке на плату
#define GHC_UPLOAD_CHUNK_SIZE 200

// глубина сканирования файловой системы (esp32)
#define GHC_FS_MAX_DEPTH 5

// период кеширования файлов для портала
#define GHC_PORTAL_CACHE "max-age=604800"

// http порт
#define GHC_HTTP_PORT 80

// websocket порт
#define GHC_WS_PORT 81

// путь к папке с файлами с HTTP доступом
#define GHC_PUBLIC_PATH "/www"
