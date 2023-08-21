#include "misc.h"

String _GH_empty_str;

char* GH_splitter(char* list, char div) {
    static uint8_t prev, end;
    if (list == NULL) prev = end = 0;
    else {
        if (prev) *(list + prev - 1) = div;
        char* cur = strchr(list + prev, div);
        if (cur) {
            *cur = '\0';
            uint8_t b = prev;
            prev = cur - list + 1;
            return list + b;
        } else if (!end) {
            end = 1;
            return list + prev;
        }
    }
    return NULL;
}
String GH_listIdx(const String& li, int idx, char div) {
    int cnt = 0, p = 0, i = 0;
    while (1) {
        if (li[i] == div || !li[i]) {
            if (cnt == idx) return li.substring(p, i);
            if (!li[i]) return _GH_empty_str;
            cnt++;
            p = i + 1;
        }
        i++;
    }
    return _GH_empty_str;
}

void GH_escapeChar(String* s, char c) {
    switch (c) {
        case 0 ... 13:
            *s += '\\';
            switch (c) {
                case '\r':
                    *s += 'r';
                    break;
                case '\n':
                    *s += 'n';
                    break;
                case '\t':
                    *s += 't';
                    break;
            }
            break;

        case '\"':
            *s += '\\';
            *s += '\"';
            break;

        case '\\':
            *s += '\\';
            *s += '\\';
            break;

        default:
            *s += c;
    }
}

void GH_addEsc(String* s, VSPTR str, bool fstr, char sym) {
    if (!str) return;
    PGM_P cstr = (PGM_P)str;
    if (fstr) {
        uint16_t len = strlen_P((PGM_P)str);

#ifdef ESP32
        if (memchr(cstr, '\"', len)) {
#else
        if (memchr_P(cstr, '\"', len)) {
#endif
            char c;
            for (uint16_t i = 0; i < len; i++) {
                c = pgm_read_byte(cstr + i);
                if (c == sym) *s += '\\';
                *s += c;
            }
        } else {
            *s += (FSTR)str;
        }
    } else {
        if (strchr(cstr, sym)) {
            uint16_t len = strlen(cstr);
            for (uint16_t i = 0; i < len; i++) {
                if (cstr[i] == sym) *s += '\\';
                *s += cstr[i];
            }
        } else {
            *s += cstr;
        }
    }
}
/*
void GH_addEsc(String* s, VSPTR str, bool fstr, const char* sym) {
    if (!str) return;
    uint8_t am = strlen(sym);
    PGM_P cstr = (PGM_P)str;
    if (fstr) {
        uint16_t len = strlen_P((PGM_P)str);
        bool esc = false;
        if (am == 1) esc = memchr_P(cstr, sym[0], len);
        else {
            for (uint8_t i = 0; i < am; i++) {
                if (memchr_P(cstr, sym[i], len)) {
                    esc = 1;
                    break;
                }
            }
        }
        if (esc) {
            char c;
            for (uint16_t i = 0; i < len; i++) {
                c = pgm_read_byte(cstr + i);
                if (am == 1) {
                    if (c == sym[0]) *s += '\\';
                } else {
                    for (uint8_t ii = 0; ii < am; ii++) {
                        if (c == sym[ii]) *s += '\\';
                    }
                }
                *s += c;
            }
        } else {
            *s += (FSTR)str;
        }
    } else {
        bool esc = false;
        if (am == 1) esc = strchr(cstr, sym[0]);
        else {
            for (uint8_t i = 0; i < am; i++) {
                if (strchr(cstr, sym[i])) {
                    esc = 1;
                    break;
                }
            }
        }
        if (esc) {
            uint16_t len = strlen(cstr);
            for (uint16_t i = 0; i < len; i++) {
                if (am == 1) {
                    if (cstr[i] == sym[0]) *s += '\\';
                } else {
                    for (uint8_t ii = 0; ii < am; ii++) {
                        if (cstr[i] == sym[ii]) *s += '\\';
                    }
                }
                *s += cstr[i];
            }
        } else {
            *s += cstr;
        }
    }
}
*/
// ========================== FS ==========================
#ifdef GH_ESP_BUILD
#ifndef GH_NO_FS
void GH_listDir(String& str, const String& path, char div) {
    str = "";
#ifdef ESP8266
    Dir dir = GH_FS.openDir(path);
    while (dir.next()) {
        if (dir.isFile() && dir.fileName().length()) {
            str += path;
            str += dir.fileName();
            str += div;
        }
    }
#else  // ESP32
    File root = GH_FS.open(path.c_str());
    if (!root || !root.isDirectory()) return;
    File file;
    while (file = root.openNextFile()) {
        if (!file.isDirectory()) {
            str += path;
            str += '/';
            str += file.name();
            str += div;
        }
    }
#endif
    if (str.length()) str.remove(str.length() - 1);
}

void GH_showFiles(String& answ, const String& path, GH_UNUSED uint8_t levels, uint16_t* count) {
#ifdef ESP8266
    Dir dir = GH_FS.openDir(path);
    while (dir.next()) {
        if (dir.isDirectory()) {
            String p(path);
            p += dir.fileName();
            p += '/';
            answ += '\"';
            answ += p;
            answ += "\":0,";
            if (count) {
                *count += answ.length();
                answ = "";
            }
            Dir sdir = GH_FS.openDir(p);
            GH_showFiles(answ, p);
        }
        if (dir.isFile() && dir.fileName().length()) {
            answ += '\"';
            answ += path;
            answ += dir.fileName();
            answ += "\":";
            answ += dir.fileSize();
            answ += ',';
            if (count) {
                *count += answ.length();
                answ = "";
            }
        }
    }

#else  // ESP32
    File root = GH_FS.open(path.c_str());
    if (!root || !root.isDirectory()) return;
    File file;
    while (file = root.openNextFile()) {
        if (file.isDirectory()) {
            answ += '\"';
            answ += file.path();
            answ += "/\":0,";
            if (count) {
                *count += answ.length();
                answ = "";
            }
            if (levels) GH_showFiles(answ, file.path(), levels - 1);
        } else {
            answ += '\"';
            if (levels != GH_FS_DEPTH) answ += path;
            answ += '/';
            answ += file.name();
            answ += "\":";
            answ += file.size();
            answ += ',';
            if (count) {
                *count += answ.length();
                answ = "";
            }
        }
    }
#endif
}
#endif
#endif