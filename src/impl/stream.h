#pragma once
#ifndef GHI_IMPL_SELECT
# error Never include implementation-specific files directly, use "impl/impl_select.h"
#endif
#include "hub/types.h"

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
            parse((char*)str.c_str(), gyverhub::ConnectionType::STREAM);
        }
    }

    void sendStream(const String& answ) {
        if (stream) stream->print(answ);
    }

    virtual void parse(char* url, gyverhub::ConnectionType from) = 0;

    // ============ PRIVATE =============
   private:
    Stream* stream = nullptr;
};
