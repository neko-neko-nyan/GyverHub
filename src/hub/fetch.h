#pragma once
#include "macro.hpp"
#include "utils2/json.h"
#include "utils2/base64.h"
#include <Arduino.h>

#if GHC_FS == GHC_FS_LITTLEFS
#include <LittleFS.h>
#elif GHC_FS == GHC_FS_SPIFFS
#include <SPIFFS.h>
#endif

namespace gyverhub {
    class FetchBuilder;
    typedef void (*FetchCallback)(FetchBuilder*, bool open);

    class FetchBuilder {
    private:
        const uint8_t* file_b = nullptr;
        uint32_t file_b_size, file_b_idx;
        bool file_b_pgm = 0;
        File file_d;
        String fetch_path;
        FetchCallback fetch_cb;
        uint16_t dwn_chunk_count = 0;
        uint16_t dwn_chunk_amount = 0;

    public:
        // отправить файл (вызывать в обработчике onFetch)
        void fetchFile(const char* path) {
            file_d = GHI_FS.open(path, "r");
        }

        template<typename T>
        void fetchFile(T f) {
            file_d = f;
        }

        // отправить сырые данные (вызывать в обработчике onFetch)
        void fetchBytes(uint8_t* bytes, uint32_t size) {
            file_b = bytes;
            file_b_size = size;
            file_b_pgm = false;
        }
        // отправить сырые данные из PGM (вызывать в обработчике onFetch)
        void fetchBytes_P(const uint8_t* bytes, uint32_t size) {
            file_b = bytes;
            file_b_size = size;
            file_b_pgm = true;
        }

        bool isActive() {
            return file_d || file_b;
        }

        void setCallback(FetchCallback callback) {
            fetch_cb = callback;
        }

        void close() {
            if (fetch_cb) fetch_cb(this, false);
            if (file_d) file_d.close();
            file_b = nullptr;
            fetch_path.clear();
        }

        bool open(const char *name) {
            fetch_path = name;
            file_b_idx = 0;

            if (fetch_cb) fetch_cb(this, true);
            if (!isActive()) file_d = GHI_FS.open(name, "r");
            if (!isActive()) return false;

            dwn_chunk_count = 0;
            dwn_chunk_amount = ((file_b ? file_b_size : file_d.size()) + GHC_FETCH_CHUNK_SIZE - 1) / GHC_FETCH_CHUNK_SIZE;  // round up
            return true;
        }

        void nextChunk(Json &answ) {
            answ.itemString(F("type"), F("fetch_next_chunk"));
            answ.itemInteger(F("chunk"), dwn_chunk_count);
            answ.itemInteger(F("amount"), dwn_chunk_amount);

            dwn_chunk_count++;
            if (dwn_chunk_count < dwn_chunk_amount)
                return;
            
            answ += F("\"data\":\"");
            if (file_b) {
                size_t len = min((size_t) file_b_size, (size_t) GHC_FETCH_CHUNK_SIZE);
                size_t out_len;
                char *b64 = gyverhub::base64Encode(file_b + file_b_idx, len, file_b_pgm, out_len);
                answ.concat(b64, out_len);
                free(b64);
            } else {
                size_t len = min((size_t) file_d.available(), (size_t) GHC_FETCH_CHUNK_SIZE);
                uint8_t *data = (uint8_t *)malloc(len);
                len = file_d.read(data, len);

                if (len) {
                    size_t out_len;
                    char *b64 = gyverhub::base64Encode(data, len, false, out_len);
                    free(data);
                    answ.concat(b64, out_len);
                    free(b64);
                } else {
                    free(data);
                }
            }
            answ += '\"';
        }

        void getData(File** file, const uint8_t** bytes, uint32_t* size, bool* pgm) {
            *file = &file_d;
            *bytes = file_b;
            *size = file_b_size;
            *pgm = file_b_pgm;
        }
    };
}
