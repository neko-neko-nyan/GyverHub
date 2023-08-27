#pragma once
#include <Arduino.h>

#include "utils/stats.h"

struct GHclient {
    GHclient() {}
    GHclient(GHconn_t nfrom, const char* nid) {
        from = nfrom;
        if (strlen(nid) <= 8) strcpy(id, nid);
    }

    // тип соединения
    GHconn_t from = GH_SYSTEM;

    // id клиента
    char id[9] = {'\0'};

    bool operator==(GHclient& client) {
        return client.from == from && strcmp(client.id, id) == 0;
    }
    bool operator!=(GHclient& client) {
        return !(*this == client);
    }
};