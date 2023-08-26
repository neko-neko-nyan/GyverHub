#include "datatypes.h"
#include "utils/flags.h"
#include "ui/color.h"

void GHtypeToStr(gyverhub::Json* s, void* var, GHdata_t type) {
    if (!var) {
        *s += '0';
        return;
    }
    switch (type) {
        case GH_STR:
            //*s += *(String*)var;
            s->appendEscaped(((String*)var)->c_str());
            break;
        case GH_CSTR:
            //*s += (char*)var;
            s->appendEscaped((char*) var);
            break;

        case GH_BOOL:
            *s += *(bool*)var;
            break;

        case GH_INT8:
            *s += *(int8_t*)var;
            break;

        case GH_UINT8:
            *s += *(uint8_t*)var;
            break;

        case GH_INT16:
            *s += *(int16_t*)var;
            break;
        case GH_UINT16:
            *s += *(uint16_t*)var;
            break;

        case GH_INT32:
            *s += *(int32_t*)var;
            break;
        case GH_UINT32:
            *s += *(uint32_t*)var;
            break;

        case GH_FLOAT:
            if (isnan(*(float*)var)) *s += 0;
            else *s += *(float*)var;
            break;
        case GH_DOUBLE:
            if (isnan(*(double*)var)) *s += 0;
            else *s += *(double*)var;
            break;

        case GH_COLOR:
            *s += ((gyverhub::Color*)var)->toHex();
            break;
        case GH_FLAGS:
#if GH_ESP_BUILD
            *s += ((GHflags<64>*)var)->to_ullong();
#else
            *s += (unsigned long)((GHflags<64>*)var)->to_ullong();
#endif
            break;
        case GH_POS:
            break;

        case GH_NULL:
            *s += '0';
            break;
    }
}