#pragma once
#include <stdint.h>

// Visual packet output with colors
void visual_packet(const uint8_t *src, const uint8_t *dst, 
                 int src_port, int dst_port, int known_mac,
                 int is_arp, int vlan_id);

// Print MAC and ARP tables
void visual_mac_table(uint8_t macs[][6], int ports[], int count);
void visual_arp_table(uint32_t ips[], uint8_t macs[][6], int count);
