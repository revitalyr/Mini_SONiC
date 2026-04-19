#pragma once

typedef struct {
    int ifindex;
    int sock;
    char name[16];
    uint16_t vlan;
    int active;
} port_t;

extern port_t ports[MAX_PORTS];
extern int port_count;

int port_init(void);
int port_add(const char *ifname);
int port_find_by_ifindex(int ifindex);
int port_find_by_name(const char *ifname);
void port_print(void);
int port_get_sock(int port_idx);
