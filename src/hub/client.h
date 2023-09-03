#pragma once
#include "hub/types.h"


struct GHclient {
    GHclient() {}
    GHclient(gyverhub::ConnectionType nfrom, const char* nid) {
        from = nfrom;
        if (strlen(nid) <= 8) strcpy(id, nid);
    }

    // тип соединения
    gyverhub::ConnectionType from = gyverhub::ConnectionType::UNKNOWN;

    // id клиента
    char id[9] = {'\0'};

    bool operator==(GHclient& client) {
        return client.from == from && strcmp(client.id, id) == 0;
    }
    bool operator!=(GHclient& client) {
        return !(*this == client);
    }
};