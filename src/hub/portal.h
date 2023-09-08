#pragma once 
#include "macro.hpp"

namespace gyverhub::portal {
#if GHI_MOD_ENABLED(GH_MOD_PORTAL)
    extern const uint8_t index[] PROGMEM;
    extern const size_t index_size;
    extern const uint8_t script[] PROGMEM;
    extern const size_t script_size;
    extern const uint8_t style[] PROGMEM;
    extern const size_t style_size;
#endif
}
