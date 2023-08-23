#include "build.h"

void GHbuild::parse(void *var, GHdata_t dtype) {
    const char *str = this->value;
    if (!var) return;
    switch (dtype) {
        case GH_STR:
            *(String*)var = str;
            break;
        case GH_CSTR:
            strcpy((char*)var, str);
            break;

        case GH_BOOL:
            *(bool*)var = (str[0] == '1');
            break;

        case GH_INT8:
            *(int8_t*)var = atoi(str);
            break;

        case GH_UINT8:
            *(uint8_t*)var = atoi(str);
            break;

        case GH_INT16:
            *(int16_t*)var = atoi(str);
            break;
        case GH_UINT16:
            *(uint16_t*)var = atoi(str);
            break;

        case GH_INT32:
            *(int32_t*)var = atol(str);
            break;
        case GH_UINT32:
            *(uint32_t*)var = atol(str);
            break;

        case GH_FLOAT:
            *(float*)var = atof(str);
            break;
        case GH_DOUBLE:
            *(double*)var = atof(str);
            break;

        case GH_COLOR:
            *(gyverhub::Color*)var = gyverhub::Color::fromHex(atol(str));
            break;
        case GH_FLAGS:
            *((GHflags<64>*)var) = GHflags<64>(atoi(str));
            break;
        case GH_POS: {
            uint32_t xy = atol(str);
            ((GHpos*)var)->_changed = true;
            ((GHpos*)var)->x = xy >> 16;
            ((GHpos*)var)->y = xy & 0xffff;
        } break;

        case GH_NULL:
            break;
    }
}
