#pragma once 
#include "macro.hpp"

namespace gyverhub::portal {
#if GHC_PORTAL == GHC_PORTAL_BUILTIN
    extern const uint8_t index[] PROGMEM;
    extern const size_t index_size;
    extern const uint8_t script[] PROGMEM;
    extern const size_t script_size;
    extern const uint8_t style[] PROGMEM;
    extern const size_t style_size;
#endif
}
