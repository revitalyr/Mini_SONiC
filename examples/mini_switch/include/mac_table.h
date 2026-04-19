#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#define MAC_TIMEOUT 300

typedef struct {
    MacAddress mac;
    PortId port;
    VlanId vlan;
    Timestamp last_seen;
} mac_entry_t;

void mac_table_init(void);
void mac_learn(const MacAddress mac, PortId port, VlanId vlan);
int mac_lookup(const MacAddress mac, VlanId vlan);
void mac_table_cleanup(void);
void mac_table_print(void);
void mac_table_print_visual(void);
void mac_table_get_entries(MacAddress macs[], PortId ports[], EntryCount *count);
