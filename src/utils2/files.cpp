#include "files.h"
#include "GyverHub.h"

#if defined(ESP32)

static void mkdir_p(char *path) {
    char* ptr = strchr(path, '/');
    while (ptr) {
        *ptr = 0;
        GH_FS.mkdir(path);
        *ptr = '/';
        ptr = strchr(ptr + 1, '/');
    }
}

void gyverhub::mkdirRecursive(char *path) {
    if (GH_FS.exists(path)) return;
    mkdir_p(path);
}

void gyverhub::mkdirRecursive(const char *path) {
    if (!strchr(path, '/')) return;
    if (GH_FS.exists(path)) return;
    
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
        GH_FS.rmdir(pathStr);
        ptr = strrchr(pathStr, '/');
    }
    free(pathStr);
}

#elif defined(ESP8266)

void gyverhub::mkdirRecursive(char *path) {}

void gyverhub::mkdirRecursive(const char *path) {}

void gyverhub::rmdirRecursive(const char *path) {}


#endif
