#pragma once
#include <stdint.h>
#include "cross_platform.h" // For PACKED_STRUCT macros

START_PACKED_STRUCT
typedef struct {
    uint16_t tpid;
    uint16_t tci;
} vlan_hdr_t PACKED_STRUCT;
END_PACKED_STRUCT

static inline uint16_t vlan_id(uint16_t tci) {
    return tci & 0x0FFF;
}

static inline uint8_t vlan_prio(uint16_t tci) {
    return (tci >> 13) & 0x07;
}

static inline int vlan_is_tagged(uint16_t ethertype) {
    return ethertype == ETH_P_8021Q;
}

int port_vlan_allowed(int port, uint16_t vlan_id);
void vlan_add_port(int port, uint16_t vlan_id);
void vlan_init(void);
