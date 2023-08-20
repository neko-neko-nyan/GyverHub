#pragma once
#include "config.hpp"
#include "macro.hpp"

#ifdef GH_NO_STREAM
class HubStream {
   public:
};
#else

#include <Arduino.h>
#include <Stream.h>

#include "utils/stats.h"

class HubStream {
   public:
    // подключить Stream для связи
    void setupStream(Stream* nstream) {
        stream = nstream;
    }

   protected:
    void tickStream() {
        if (stream && stream->available()) {
            String str = stream->readStringUntil('\0');
            parse((char*)str.c_str(), GH_SERIAL);
        }
    }

    void sendStream(const String& answ) {
        if (stream) stream->print(answ);
    }

    virtual void parse(char* url, GHconn_t from) = 0;

    // ============ PRIVATE =============
   private:
    Stream* stream = nullptr;
};
#endif