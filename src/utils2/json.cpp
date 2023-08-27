#include "json.h"

#if defined(ESP32) && !defined(strchr_P)
#define strchr_P(a, b) strchr((a),(b))
#endif
#if defined(ESP8266) && !defined(strchr_P)
#define strchr_P(a, b) memchr_P((a),(b), strlen_P((a)))
#endif

void gyverhub::Json::appendEscaped(const void *str, bool fstr, char sym) {
    if (fstr) appendEscaped((FSTR) str, sym);
    else appendEscaped((const char *) str, sym);
}

void gyverhub::Json::appendEscaped(const String &str, char sym) {
    if (!str) return;

    if (str.indexOf(sym) == -1) {
        concat(str);
        return;
    }
    
    uint16_t len = str.length();
    for (uint16_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == sym) concat('\\');
        concat(c);
    }
}

void gyverhub::Json::appendEscaped(const char *str, char sym) {
    if (!str) return;

    if (strchr(str, sym) == 0) {
        concat(str);
        return;
    }
    
    uint16_t len = strlen(str);
    for (uint16_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == sym) concat('\\');
        concat(c);
    }
}

void gyverhub::Json::appendEscaped(FSTR fstr, char sym) {
    const char *str = (const char *)fstr;
    if (!str) return;

    if (strchr_P(str, sym) == 0) {
        concat(fstr);
        return;
    }

    uint16_t len = strlen_P(str);
    for (uint16_t i = 0; i < len; i++) {
        char c = pgm_read_byte(str + i);
        if (c == sym) concat('\\');
        concat(c);
    }
}
