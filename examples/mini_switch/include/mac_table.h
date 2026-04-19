#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#define MAC_TIMEOUT 300

typedef struct {
    uint8_t mac[6];
    int port;
    uint16_t vlan;
    time_t last_seen;
} mac_entry_t;

void mac_table_init(void);
void mac_learn(const uint8_t *mac, int port, uint16_t vlan);
int mac_lookup(const uint8_t *mac, uint16_t vlan);
void mac_table_cleanup(void);
void mac_table_print(void);
void mac_table_print_visual(void);
void mac_table_get_entries(uint8_t macs[][6], int ports[], int *count);
