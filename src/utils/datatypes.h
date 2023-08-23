#pragma once
#include <Arduino.h>

#include "../config.hpp"
#include "../macro.hpp"
#include "ui/color.h"
#include "flags.h"
#include "ui/pos.h"
#include "misc.h"
#include "utils2/json.h"

enum GHdata_t {
    GH_NULL,

    GH_STR,
    GH_CSTR,

    GH_BOOL,
    GH_INT8,
    GH_UINT8,
    GH_INT16,
    GH_UINT16,
    GH_INT32,
    GH_UINT32,

    GH_FLOAT,
    GH_DOUBLE,

    GH_COLOR,
    GH_FLAGS,
    GH_POS,
};

void GHtypeToStr(gyverhub::Json* s, void* val, GHdata_t type);
