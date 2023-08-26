#include "misc.h"
#include "GyverHub.h"

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

// ========================== FS ==========================
#if GHC_FS != GHC_FS_NONE
void GH_listDir(String& str, const String& path, char div) {
    str = "";
#ifdef ESP8266
    Dir dir = GHI_FS.openDir(path);
    while (dir.next()) {
        if (dir.isFile() && dir.fileName().length()) {
            str += path;
            str += dir.fileName();
            str += div;
        }
    }
#else  // ESP32
    File root = GHI_FS.open(path.c_str());
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

void GH_showFiles(String& answ, const String& path, GHI_UNUSED uint8_t levels, uint16_t* count) {
#ifdef ESP8266
    Dir dir = GHI_FS.openDir(path);
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
            Dir sdir = GHI_FS.openDir(p);
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
    File root = GHI_FS.open(path.c_str());
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
            if (levels != GHC_FS_MAX_DEPTH) answ += path;
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
