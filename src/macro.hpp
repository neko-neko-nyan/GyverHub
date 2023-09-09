/**
 * macro.h - макросы GyverHub
 * 
 * GHC_* - Макросы конфигурации библиотеки, указываются в config.h
 * GHI_* - Внутренние макросы (конфигурация и утилиты), указываются здесь.
 * GH_* - Макросы конфигурации, указываемые пользователем перед include <GyverHub.h>
 * GH_* - Утилиты для пользователей библиотеки
 */
#pragma once
#include "config.hpp"
#include <Arduino.h>


// ============================================================================
// Утилиты для пользователей библиотеки
// ============================================================================

#define GH_NO_LABEL F("_no")
#define GH_NUMBERS F("^\\d+$")
#define GH_LETTERS F("^[a-zA-Z]+$")
#define GH_LETTERS_S F("^[a-z]+$")
#define GH_LETTERS_C F("^[A-Z]+$")

#define VSPTR const void*
#define CSREF const String&
#define FSTR const __FlashStringHelper*


// ============================================================================
// Внутренние утилиты
// ============================================================================

#define GHI_UNUSED __attribute__((unused))
#define GHI_PGM(name, str) static const char name[] PROGMEM = str
#define GHI_PGM_LIST(name, ...) static const char* const name[] PROGMEM = {__VA_ARGS__}
#define GHI_MOD_ENABLED(MODULE) ((GHC_MODS_ENABLED & (MODULE)) != 0)


// ============================================================================
// Конфигурация
// ============================================================================

#if defined(ESP8266) || defined(ESP32)
#define GHI_ESP_BUILD 1
#else
#define GHI_ESP_BUILD 0
#endif

#if defined(ESP32)
#define GHI_PLATFORM_STR "ESP32"
#elif defined(ESP8266)
#define GHI_PLATFORM_STR "ESP8266"
#elif defined(__AVR_ATmega328P__)
#define GHI_PLATFORM_STR "ATmega328"
#else
#define GHI_PLATFORM_STR "Unknown"
#endif

#if GHC_LIB_DEBUG
#define GHI_DEBUG_LOG(fmt, ...) do { Serial.printf("%s:%d in %s: " fmt "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## __VA_ARGS__); } while(0)
#else
#define GHI_DEBUG_LOG(fmt, ...) do {} while(0)
#endif


// Modules

#define GH_MOD_INFO (1ul << 0)
#define GH_MOD_FSBR (1ul << 1)
#define GH_MOD_FORMAT (1ul << 2)
#define GH_MOD_FETCH (1ul << 3)
#define GH_MOD_UPLOAD (1ul << 4)
#define GH_MOD_OTA (1ul << 5)
#define GH_MOD_OTA_URL (1ul << 6)
#define GH_MOD_REBOOT (1ul << 7)
#define GH_MOD_SET (1ul << 8)
#define GH_MOD_READ (1ul << 9)
#define GH_MOD_DELETE (1ul << 10)
#define GH_MOD_RENAME (1ul << 11)

#define GHC_MODS_ENABLED 0xffffffff
#define GHC_MODS_DISABLED ((~GHC_MODS_ENABLED) & 0xFFFF)

// FS

#define GHC_FS_NONE 0
#define GHC_FS_LITTLEFS 1
#define GHC_FS_SPIFFS 2

#if !GHI_ESP_BUILD
#undef GHC_FS
#define GHC_FS GHC_FS_NONE
#endif

#if GHC_FS == GHC_FS_LITTLEFS

#include <LittleFS.h>
#define GHI_FS LittleFS

#elif GHC_FS == GHC_FS_SPIFFS

#ifdef ESP8266
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <FS.h>
#else
#include <SPIFFS.h>
#endif

#define GHI_FS SPIFFS

#else
#define GHI_FS !!!
#endif

// Portal

#define GHC_PORTAL_NONE 0
#define GHC_PORTAL_BUILTIN 1
#define GHC_PORTAL_FS 2

#if GHC_PORTAL == GHC_PORTAL_FS && GHC_FS == GHC_FS_NONE
#error Cannot use GHC_PORTAL_FS with GHC_FS_NONE!
#endif

#if GHC_PORTAL == GHC_PORTAL_NONE
#undef GHC_DNS_SERVER
#define GHC_DNS_SERVER 0
#endif


// IMPL

#define GHC_IMPL_NONE 0
#define GHC_IMPL_SYNC 1
#define GHC_IMPL_ASYNC 2
#define GHC_IMPL_NATIVE 3

#ifndef GHC_STREAM_IMPL
#if defined(GHC_IMPL) && GHC_IMPL == GHC_IMPL_NONE
#define GHC_STREAM_IMPL GHC_IMPL_NONE
#else
#define GHC_STREAM_IMPL GHC_IMPL_NATIVE
#endif
#endif

#ifndef GHC_BLUETOOTH_IMPL
#define GHC_BLUETOOTH_IMPL GHC_IMPL_NONE
#endif

#ifndef GHC_IMPL

#if GHI_ESP_BUILD
#define GHC_IMPL GHC_IMPL_SYNC
#else
#define GHC_IMPL GHC_IMPL_NONE
#endif

#endif // !defined(GHC_IMPL)

#ifndef GHC_MQTT_IMPL
#define GHC_MQTT_IMPL GHC_IMPL
#endif

#ifndef GHC_HTTP_IMPL
#define GHC_HTTP_IMPL GHC_IMPL
#endif
