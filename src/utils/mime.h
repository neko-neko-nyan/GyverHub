#pragma once

#include <Arduino.h>

namespace gyverhub {
    /**
     * Получить MIME тип по расширению
     * @param ext Расшиерние (без точки)
     * @param length Длинна расширения
     * @param isGzip Если не nullptr, а расширение = gz/gzip, то в *isGzip записывается true.
     * @returns MIME тип файла (например, text/plain)
     */
    PGM_P getMimeByExt(const char *ext, size_t length, bool *isGzip = nullptr);

    /**
     * Получить MIME тип файла по имени.
     * @param path Имя или путь к файлу.
     * @param length Длинна параметра path
     * @param isGzip Если не nullptr, то второе расширение gz/gzip удаляется, а в *isGzip записывается true.
     * @returns MIME тип файла (например, text/plain)
     */
    PGM_P getMimeByPath(const char *path, size_t length, bool *isGzip = nullptr);
}
