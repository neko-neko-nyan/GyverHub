#pragma once
#include "macro.hpp"
#include "utils/json.h"

void GH_showFiles(gyverhub::Json& answ, uint16_t* count = nullptr, const String& path = "/", GHI_UNUSED uint8_t levels = GHC_FS_MAX_DEPTH);
