#pragma once
#include "macro.hpp"

namespace gyverhub {
    template <size_t SIZE>
    struct Parser {
        static_assert(SIZE > 0);

    private:
        char* str[SIZE] {nullptr,};
        size_t size = 0;

    public:
        Parser(char* url) {
            int16_t len = strlen(url);
            for (size_t i = 0; i < SIZE; i++) {
                char* div = (char*)memchr(url, '/', len);
                str[i] = url;
                size++;
                if (div == nullptr || i >= SIZE)
                    break;
                
                size_t divlen = div - url;  // div >= url, see memchr logic
                len -= divlen + 1;
                url += divlen + 1;
                *div = '\0';
            }
        }

        inline char *get(size_t idx) const noexcept {
            return str[idx];
        }

        inline size_t length() const noexcept {
            return size;
        }

    };

    struct Splitter {
    private:
        char *list;
        char *res = nullptr;

    public:
        constexpr Splitter(char *list) : list(list) {}

        bool next(char div = ',') noexcept {
            if (list == nullptr) return false;

            char *cur = strchr(list, div);
            if (cur) {
                *cur = '\0';
                list = cur + 1;
                res = cur;
            } else {
                res = list;
                list = nullptr;
            }
            return true;
        }

        inline char *get() const noexcept {
            return res;
        }
    };
}
