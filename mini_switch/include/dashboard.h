#pragma once
#include <stdint.h>
#include "cross_platform.h"

// Dashboard interface for real-time visualization
void dashboard_init(void);
void dashboard_cleanup(void);
void dashboard_refresh(void);
int dashboard_should_exit(void);

// Update functions for different modules
void dashboard_update_stats(unsigned long rx_count, unsigned long tx_count, 
                        unsigned long mac_entries, unsigned long arp_entries);
void dashboard_update_mac_table(uint8_t macs[][6], int ports[], int count);
void dashboard_update_arp_table(uint32_t ips[], uint8_t macs[][6], int count);
void dashboard_add_packet(const char *packet_info);
void dashboard_update_packets(void);
void dashboard_clear_packets(void);
