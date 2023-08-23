#pragma once
#include "config.hpp"

#define GH_UNUSED __attribute__((unused))
#define VSPTR const void*
#define CSREF const String&
#define GH_PGM(name, str) static const char name[] PROGMEM = str
#define GH_PGM_LIST(name, ...) const char* const name[] PROGMEM = {__VA_ARGS__}
#define FSTR const __FlashStringHelper*
#define GH_NO_LABEL F("_no")
#define GH_NUMBERS F("^\\d+$")
#define GH_LETTERS F("^[a-zA-Z]+$")
#define GH_LETTERS_S F("^[a-z]+$")
#define GH_LETTERS_C F("^[A-Z]+$")

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


#ifdef GH_LIB_DEBUG
#define GH_DEBUG_LOG(fmt, ...) do { Serial.printf("%s:%d in %s: " fmt "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, ## __VA_ARGS__); } while(0)
#else
#define GH_DEBUG_LOG(fmt, ...) do {} while(0)
#endif
