#pragma once
#include <stdint.h>
#include "cross_platform.h" // For PACKED_STRUCT macros

START_PACKED_STRUCT
struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t ethertype;
} PACKED_STRUCT;
END_PACKED_STRUCT

void print_mac(const uint8_t *mac);
int is_broadcast(const uint8_t *mac);
int is_multicast(const uint8_t *mac);
