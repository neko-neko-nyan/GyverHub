#include "hub/types.h"
#include "macro.hpp"

GHI_PGM(_GH_CMD0, "focus");
GHI_PGM(_GH_CMD1, "ping");
GHI_PGM(_GH_CMD2, "unfocus");
GHI_PGM(_GH_CMD3, "info");
GHI_PGM(_GH_CMD4, "fsbr");
GHI_PGM(_GH_CMD5, "format");
GHI_PGM(_GH_CMD6, "reboot");
GHI_PGM(_GH_CMD7, "data");
GHI_PGM(_GH_CMD8, "set");
GHI_PGM(_GH_CMD9, "cli");
GHI_PGM(_GH_CMD10, "delete");
GHI_PGM(_GH_CMD11, "rename");
GHI_PGM(_GH_CMD12, "fetch");
GHI_PGM(_GH_CMD13, "fetch_chunk");
GHI_PGM(_GH_CMD14, "fetch_stop");
GHI_PGM(_GH_CMD15, "upload");
GHI_PGM(_GH_CMD16, "upload_chunk");
GHI_PGM(_GH_CMD17, "ota");
GHI_PGM(_GH_CMD18, "ota_chunk");
GHI_PGM(_GH_CMD19, "ota_url");
GHI_PGM(_GH_CMD20, "read");

#define GH_CMD_LEN 20
GHI_PGM_LIST(_GH_cmd_list, _GH_CMD0, _GH_CMD1, _GH_CMD2, _GH_CMD3, _GH_CMD4, _GH_CMD5, _GH_CMD6, _GH_CMD7, _GH_CMD8, _GH_CMD9, _GH_CMD10, _GH_CMD11, _GH_CMD12, _GH_CMD13, _GH_CMD14, _GH_CMD15, _GH_CMD16, _GH_CMD17, _GH_CMD18, _GH_CMD19, _GH_CMD20);

gyverhub::Command gyverhub::parseCommand(const char* str) {
    for (int i = 0; i < GH_CMD_LEN; i++) {
#if GHI_ESP_BUILD
        if (!strcmp_P(str, _GH_cmd_list[i])) return static_cast<Command>(i);
#else
        if (!strcmp_P(str, (PGM_P)pgm_read_word(_GH_cmd_list + i))) return static_cast<Command>(i);
#endif
    }
    return Command::UNKNOWN;
}
