#pragma once
#include <Arduino.h>

#include "../config.hpp"
#include "../macro.hpp"
#include "datatypes.h"
#include "client.h"
#include "stats.h"

// тип билда
enum GHbuild_t {
    GH_BUILD_NONE,
    GH_BUILD_ACTION,
    GH_BUILD_COUNT,
    GH_BUILD_READ,
    GH_BUILD_UI,
    GH_BUILD_TG,
};

class GHbuild {
private:
    void parse(void *var, GHdata_t dtype);

   public:
    GHbuild(GHbuild_t btype = GH_BUILD_NONE, const char* name = nullptr, const char* value = nullptr, GHclient nclient = GHclient()) {
        type = btype;
        name = name;
        value = value;
        client = nclient;
    }

    bool parse(VSPTR cname, void* var, GHdata_t dtype, bool fstr) {
        if (type == GH_BUILD_ACTION && nameEq(cname, fstr)) {
            type = GH_BUILD_NONE;
            parse(var, dtype);
            return 1;
        }
        return 0;
    }

    bool nameEq(VSPTR cname, bool fstr) {
        if (cname) return fstr ? !strcmp_P(name, (PGM_P)cname) : !strcmp(name, (PGM_P)cname);
        else return autoNameEq();
    }
    bool autoNameEq() {
        return name[0] == '_' && name[1] == 'n' && (uint16_t)atoi(name + 2) == count;
    }

    // тип билда
    GHbuild_t type = GH_BUILD_NONE;

    // данные клиента
    GHclient client;

    uint16_t count = 0;

    // имя компонента
    const char* name = nullptr;

    // значение компонента
    const char* value = nullptr;
};