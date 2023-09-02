#include "files.h"
#include "macro.hpp"

#if GHC_FS == GHC_FS_SPIFFS

void gyverhub::mkdirRecursive(char *path) {
    (void) path;
}

void gyverhub::mkdirRecursive(const char *path) {
    (void) path;
}

void gyverhub::rmdirRecursive(const char *path) {
    (void) path;
}

#elif GHC_FS != GHC_FS_NONE

static void mkdir_p(char *path) {
    char* ptr = strchr(path, '/');
    while (ptr) {
        *ptr = 0;
        GHI_FS.mkdir(path);
        *ptr = '/';
        ptr = strchr(ptr + 1, '/');
    }
}

void gyverhub::mkdirRecursive(char *path) {
    if (GHI_FS.exists(path)) return;
    mkdir_p(path);
}

void gyverhub::mkdirRecursive(const char *path) {
    if (!strchr(path, '/')) return;
    if (GHI_FS.exists(path)) return;
    
    char* pathStr = strdup(path);
    if (!pathStr) return;
    
    mkdir_p(pathStr);
    free(pathStr);
}

void gyverhub::rmdirRecursive(const char *path) {
    char* pathStr = strdup(path);
    if (!pathStr) return;

    char* ptr = strrchr(pathStr, '/');
    while (ptr) {
        *ptr = 0;
        GHI_FS.rmdir(pathStr);
        ptr = strrchr(pathStr, '/');
    }
    free(pathStr);
}

#endif
