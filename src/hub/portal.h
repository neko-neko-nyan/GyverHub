#pragma once 
#include "macro.hpp"

namespace gyverhub::portal {
#if GHI_MOD_ENABLED(GH_MOD_PORTAL)
    extern const uint8_t *const index_start PROGMEM;
    extern const uint8_t *const index_end PROGMEM;
    extern const uint8_t *const script_start PROGMEM;
    extern const uint8_t *const script_end PROGMEM;
    extern const uint8_t *const style_start PROGMEM;
    extern const uint8_t *const style_end PROGMEM;
#else
    constexpr const uint8_t *const index_start = nullptr;
    constexpr const uint8_t *const index_end = nullptr;
    constexpr const uint8_t *const script_start = nullptr;
    constexpr const uint8_t *const script_end = nullptr;
    constexpr const uint8_t *const style_start = nullptr;
    constexpr const uint8_t *const style_end = nullptr;
#endif
}
