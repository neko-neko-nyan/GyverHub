/**
 * config.h - конфигурация GyverHub
 */
#pragma once

/// Отладка библиотеки
#define GHC_LIB_DEBUG 0

/**
 * Файловая система
 * 
 * Доступные значения:
 * 1. GHC_FS_NONE - Не использовать ФС.
 * 2. GHC_FS_LITTLEFS - LittleFS (по умолчанию).
 * 3. GHC_FS_SPIFFS - SPIFFS.
 */
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

/**
 * Реализация всех протоклов.
 * 
 * Доступные значения:
 * 1. GHC_IMPL_NONE - Отключить все протоколы.
 * 2. GHC_IMPL_SYNC - Синхронный.
 * 3. GHC_IMPL_ASYNC - Асинхронный.
 * 4. GHC_IMPL_NATIVE - Нативный.
 * 
 * Подробнее работа каждого значения описана в документации
 * соответствующего протокола.
 */
// #define GHC_IMPL

/**
 * Реализация протокола MQTT. См. GHC_IMPL.
 * 
 * Доступные значения:
 * 1. GHC_IMPL_NONE - Отключить протокол (по умолчанию для AVR).
 * 2. GHC_IMPL_SYNC - Синхронный (по умолчанию для ESP32 и ESP8266).
 * 3. GHC_IMPL_ASYNC - Асинхронный.
 * 4. GHC_IMPL_NATIVE - Только для ESP32. Нативный асинхронный (esp-idf).
 */
// #define GHC_MQTT_IMPL GHC_IMPL

/**
 * Реализация протокола HTTP. См. GHC_IMPL.
 * 
 * Доступные значения:
 * 1. GHC_IMPL_NONE - Отключить протокол (по умолчанию для AVR).
 * 2. GHC_IMPL_SYNC - Синхронный (по умолчанию для ESP32 и ESP8266).
 * 3. GHC_IMPL_ASYNC - Асинхронный.
 * 4. GHC_IMPL_NATIVE - Только для ESP32. Нативный асинхронный (esp-idf).
 */
// #define GHC_HTTP_IMPL GHC_IMPL

/**
 * Реализация протокола stram (serial). См. GHC_IMPL.
 * 
 * Доступные значения:
 * 1. GHC_IMPL_NONE - Отключить протокол.
 * 2. GHC_IMPL_NATIVE - Нативный синхронный (по умолчанию).
 * 
 * Другие значения GHC_IMPL неявно обозначают GHC_IMPL_NATIVE.
 */
// #define GHC_STREAM_IMPL GHC_IMPL

/**
 * Реализация протокола Bluetooth. См. GHC_IMPL.
 * 
 * Доступные значения:
 * 1. GHC_IMPL_NONE - Отключить протокол.
 * 
 * Другие значения GHC_IMPL неявно обозначают GHC_IMPL_NONE.
 */
// #define GHC_BLUETOOTH_IMPL GHC_IMPL
