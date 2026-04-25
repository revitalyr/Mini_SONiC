#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

// MAC_TIMEOUT is now defined in constants.h

typedef struct {
    MacAddress mac;
    PortId port;
    VlanId vlan;
    Timestamp last_seen;
} mac_entry_t;

void mac_table_init(void);
void mac_learn(const uint8_t *mac, int port, uint16_t vlan);
int mac_lookup(const MacAddress mac, VlanId vlan);
void mac_table_cleanup(void);
void mac_table_print(void);
void mac_table_print_visual(void);
void mac_table_get_entries(uint8_t macs[][6], int ports[], int *count);
