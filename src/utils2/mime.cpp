#include "mime.h"
#include "macro.hpp"

#define PGMSTR_T const char*

#define PGMSTRCMP2(a, b) (pgm_read_word(a) == *(const uint16_t*)(b))
#define PGMSTRCMP3(a, b) (pgm_read_word(a) == *(const uint16_t*)(b) && pgm_read_byte(&(a)[2]) == *(const uint8_t*)(&(b)[2]))
#define PGMSTRCMP4(a, b) (pgm_read_dword(a) == *(const uint32_t*)(b))

// Взято из https://habr.com/ru/companies/pvs-studio/articles/310064/
template <class T, size_t N>
constexpr size_t countof(GHI_UNUSED const T (&array)[N]) {
  return N;
}

GHI_PGM(ext_gz, "gz" "application/gzip");
GHI_PGM(ext_js, "js" "application/javascript");
GHI_PGM_LIST(exts2, ext_gz, ext_js);

GHI_PGM(ext_avi, "avi" "video/x-msvideo");
GHI_PGM(ext_bin, "bin" "application/octet-stream");
GHI_PGM(ext_bmp, "bmp" "image/bmp");
GHI_PGM(ext_css, "css" "text/css");
GHI_PGM(ext_csv, "csv" "text/csv");
GHI_PGM(ext_gif, "gif" "image/gif");
GHI_PGM(ext_jpg, "jpg" "image/jpeg");
GHI_PGM(ext_png, "png" "image/png");
GHI_PGM(ext_svg, "svg" "image/svg+xml");
GHI_PGM(ext_txt, "txt" "text/plain");
GHI_PGM(ext_wav, "wav" "audio/wav");
GHI_PGM(ext_xml, "xml" "application/xml");
GHI_PGM_LIST(exts3, ext_avi, ext_bin, ext_bmp, ext_css, ext_csv, ext_gif, ext_jpg, ext_png, ext_svg, ext_txt, ext_wav, ext_xml);

GHI_PGM(ext_gzip, "gzip" "application/gzip");
GHI_PGM(ext_html, "html" "text/html");
GHI_PGM(ext_jpeg, "jpeg" "image/jpeg");
GHI_PGM(ext_json, "json" "application/json");
GHI_PGM_LIST(exts4, ext_gzip, ext_html, ext_jpeg, ext_json);

GHI_PGM(defaultMime, "text/html");

PGM_P gyverhub::getMimeByExt(const char *ext, size_t length, bool *isGizp) {
    if (isGizp != nullptr) *isGizp = false;

    switch (length) {
        case 2:
            for (size_t i = 0; i < countof(exts2); i++) {
                PGM_P it = (PGM_P) pgm_read_ptr(&exts2[i]);
                if (PGMSTRCMP2(it, ext)) {
                    if (i == 0 && isGizp != nullptr) *isGizp = true;
                    return it + 2;
                }
            }
            break;
        case 3:
            for (size_t i = 0; i < countof(exts3); i++) {
                PGM_P it = (PGM_P) pgm_read_ptr(&exts3[i]);
                if (PGMSTRCMP3(it, ext))
                    return it + 3;
            }
            break;
        case 4:
            for (size_t i = 0; i < countof(exts4); i++) {
                PGM_P it = (PGM_P) pgm_read_ptr(&exts4[i]);
                if (PGMSTRCMP4(it, ext)) {
                    if (i == 0 && isGizp != nullptr) *isGizp = true;
                    return it + 4;
                }
            }
            break;
        default:
            break;
    }

    return defaultMime;
}

/**
 * @param path Указатель на начало имени файла
 * @param end_p Указатель на конец имени файла
 */
static size_t getExtOf(const char *path, const char * &end_p) {
    size_t ext_len = 0;
    while (end_p != path && *end_p != '.') end_p--, ext_len++;
    if (end_p == path || ext_len == 0) return 0; // no '.' in path or '.' is last char
    end_p++;
    return ext_len;
}

PGM_P gyverhub::getMimeByPath(const char *path, size_t length, bool *isGzip) {
    const char *end_p = path + length - 1;

    // *end_p = last char
    size_t ext_len = getExtOf(path, end_p);
    // *(end_p-1) = '.'
    PGM_P mime = getMimeByExt(end_p, ext_len, isGzip);

    if (isGzip != nullptr && *isGzip) {
        end_p -= 2;
        // *(end_p+1) = '.'

        ext_len = getExtOf(path, end_p);
        if (ext_len == 0) { // no '.' in path
            *isGzip = false;
            return mime;
        }

        mime = getMimeByExt(end_p, ext_len);
    }

    return mime;
}
