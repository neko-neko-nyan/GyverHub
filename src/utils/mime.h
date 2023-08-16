#pragma once

#include <Arduino.h>
#include "../macro.hpp"
#include "../config.hpp"

#ifdef GH_ESP_BUILD
String GHmime(const String &path);

/**
 * Получить MIME тип по расширению
 * @param ext Расшиерние (без точки)
 * @param length Длинна расширения
 * @returns MIME тип файла (например, text/plain)
 */
PGM_P GH_getMimeByExt(const char *ext, size_t length);

/**
 * Получить MIME тип файла по имени.
 * @param path Имя или путь к файлу.
 * @param length Длинна параметра path
 * @param isGzip Если не nullptr, то второе расширение gz/gzip удаляется, а в *isGzip записывается true.
 * @returns MIME тип файла (например, text/plain)
 */
PGM_P GH_getMimeByPath(const char *path, size_t length, bool *isGzip = nullptr);
#endif