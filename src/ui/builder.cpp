#include "builder.h"

void gyverhub::Builder::parse(void *var, DataType dtype) {
    const char *str = value;
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
            *((gyverhub::Flags*)var) = gyverhub::Flags(str);
            break;
        case GH_POS: {
            uint32_t xy = atol(str);
            *(gyverhub::Point*)var = gyverhub::Point(xy >> 16, xy & 0xffff, true);
        } break;

        case GH_NULL:
            break;
    }
}

void gyverhub::Builder::appendObject(void* var, DataType type) {
    if (!var) {
        *sptr += '0';
        return;
    }
    switch (type) {
        case GH_STR:
            sptr->appendEscaped(((String*)var)->c_str());
            break;

        case GH_CSTR:
            sptr->appendEscaped((char*) var);
            break;

        case GH_BOOL:
            *sptr += *(bool*)var;
            break;

        case GH_INT8:
            *sptr += *(int8_t*)var;
            break;

        case GH_UINT8:
            *sptr += *(uint8_t*)var;
            break;

        case GH_INT16:
            *sptr += *(int16_t*)var;
            break;
        case GH_UINT16:
            *sptr += *(uint16_t*)var;
            break;

        case GH_INT32:
            *sptr += *(int32_t*)var;
            break;
        case GH_UINT32:
            *sptr += *(uint32_t*)var;
            break;

        case GH_FLOAT:
            if (isnan(*(float*)var)) *sptr += 0;
            else *sptr += *(float*)var;
            break;
        case GH_DOUBLE:
            if (isnan(*(double*)var)) *sptr += 0;
            else *sptr += *(double*)var;
            break;

        case GH_COLOR:
            *sptr += ((gyverhub::Color*)var)->toHex();
            break;
        case GH_FLAGS:
            *sptr += ((gyverhub::Flags*)var)->toString();
            break;
        case GH_POS:
            break;

        case GH_NULL:
            *sptr += '0';
            break;
    }
}
