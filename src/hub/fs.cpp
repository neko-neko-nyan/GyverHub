#include "fs.h"

void GH_showFiles(gyverhub::Json& answ, uint16_t* count, const String& path, GHI_UNUSED uint8_t levels) {
#if GHC_FS != GHC_FS_NONE
#ifdef ESP8266
#if GHC_FS == GHC_FS_SPIFFS
    (void) path;
    (void) levels;

    Dir dir = GHI_FS.openDir("");
    while (dir.next()) {
        answ.concat('"');
        answ.appendEscaped(dir.fileName());
        answ.concat(F("\":"));
        answ.concat(dir.fileSize());
        answ.concat(',');

        if (count) {
            *count += answ.length();
            answ.clear();
        }
    }

#else  // non-spiffs on ESP8266

    Dir dir = GHI_FS.openDir(path);
    while (dir.next()) {
        answ.concat('"');

        if (dir.isDirectory()) {
            String p(path);
            p.concat(dir.fileName());
            p.concat('/');

            answ.concat(p);
            answ.concat(F("\":0,"));

            if (count) {
                *count += answ.length();
                answ.clear();
            }

            if (levels) 
                GH_showFiles(answ, count, p, levels - 1);

        } else {
            answ.concat(path);
            answ.concat(dir.fileName());
            answ.concat(F("\":"));
            answ.concat(dir.fileSize());
            answ.concat(',');

            if (count) {
                *count += answ.length();
                answ.clear();
            }
        }
    }

#endif
#else  // ESP32

    File root = GHI_FS.open(path.c_str());
    if (!root || !root.isDirectory()) return;
    File file;
    while (file = root.openNextFile()) {
        answ.concat('"');
        answ.appendEscaped(file.path());
        answ.concat(F("\":"));
        answ.concat(file.size());
        answ.concat(',');

        if (count) {
            *count += answ.length();
            answ.clear();
        }

        if (file.isDirectory() && levels)
            GH_showFiles(answ, count, file.path(), levels - 1);
    }
#endif
#endif
}
