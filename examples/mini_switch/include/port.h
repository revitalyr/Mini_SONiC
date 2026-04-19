#pragma once

#include "common.h"

typedef struct {
    InterfaceIndex ifindex;
    SocketFd sock;
    InterfaceName name;
    VlanId vlan;
    int active;
} port_t;

extern port_t ports[MAX_PORTS];
extern EntryCount port_count;

int port_init(void);
int port_add(const char *ifname);
int port_find_by_ifindex(InterfaceIndex ifindex);
int port_find_by_name(const char *ifname);
void port_print(void);
SocketFd port_get_sock(EntryCount port_idx);
