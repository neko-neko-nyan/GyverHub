#pragma once

#include <Arduino.h>

namespace gyverhub {
    class Json : public String {
    public:
        void appendEscaped(const char *str, char sym = '\"');
        void appendEscaped(FSTR str, char sym = '\"');
        void appendEscaped(const void *str, bool fstr, char sym = '\"');

        void begin() {
            this->concat(F("\n{"));
        }

        void end() {
            this->concat(F("}\n"));
        }

        void appendStringRaw(FSTR data) {
            this->concat("\"", 1);
            this->concat(data);
            this->concat("\"", 1);
        }

        void appendStringRaw(const char *data) {
            this->concat("\"", 1);
            this->concat(data);
            this->concat("\"", 1);
        }

        void appendString(const char *data) {
            this->concat("\"", 1);
            appendEscaped(data);
            this->concat("\"", 1);
        }

        void key(FSTR key) {
            appendStringRaw(key);
            this->concat(":", 1);
        }

        void key(const char *key) {
            appendStringRaw(key);
            this->concat(":", 1);
        }

        void item(FSTR key, const String& value, bool last = false) {
            this->key(key);
            this->concat(value.c_str());
            if (!last) this->concat(",", 1);
        }

        void itemString(FSTR key, const String& value, bool last = false) {
            this->key(key);
            appendString(value.c_str());
            if (!last) this->concat(",", 1);
        }

        void itemString(const char *key, const String& value, bool last = false) {
            this->key(key);
            appendString(value.c_str());
            if (!last) this->concat(",", 1);
        }

        void appendId(const char *id, bool last = false) {
            this->concat(F("\"id\":\""));
            this->concat(id);
            this->concat("\"", 1);
            if (!last) this->concat(",", 1);
        }

        void itemInteger(FSTR key, uint64_t value) {
            this->key(key);
            this->concat(value);
            this->concat(",", 1);
        }
    };
}

