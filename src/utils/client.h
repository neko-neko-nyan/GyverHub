#pragma once
#include <Arduino.h>

#include "stats.h"

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

    // id как String
    String idString() {
        return id;
    }

    bool eq(GHclient& client) {
        return (client.from == from && !strcmp(client.id, id));
    }
    bool operator==(GHclient& client) {
        return eq(client);
    }
    bool operator!=(GHclient& client) {
        return !eq(client);
    }
};