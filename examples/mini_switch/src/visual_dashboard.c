#include "visual.h"
#include "dashboard.h"
#include "mac_table.h"
#include "arp.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Dashboard thread
static pthread_t dashboard_thread;
static volatile int dashboard_running = 1;

// Buffer for packet info
static char packet_buffer[512];

void* dashboard_update_thread(void *arg) {
    while (dashboard_running) {
        dashboard_refresh();
        
        // Get MAC table data
        uint8_t macs[MAC_TABLE_SIZE][6];
        int ports[MAC_TABLE_SIZE];
        int mac_count = 0;
        
        // This would need to be implemented in mac_table.c
        // For now, we'll use the existing visual functions
        mac_table_get_entries(macs, ports, &mac_count);
        
        // Update dashboard with current data
        dashboard_update_stats(global_rx_count, global_tx_count, mac_count, 0);
        dashboard_update_mac_table(macs, ports, mac_count);
        
        // Get ARP table data
        uint32_t arp_ips[ARP_TABLE_SIZE];
        uint8_t arp_macs[ARP_TABLE_SIZE][6];
        int arp_count = 0;
        arp_table_get_entries(arp_ips, arp_macs, &arp_count);
        dashboard_update_arp_table(arp_ips, arp_macs, arp_count);
        
        dashboard_update_packets();
        
        usleep(100000); // Update every 100ms
    }
    
    return NULL;
}

void visual_packet_dashboard(const uint8_t *src, const uint8_t *dst, 
                         int src_port, int dst_port, int known_mac,
                         int is_arp, int vlan_id) {
    // Format packet info for dashboard
    char src_str[18], dst_str[18];
    snprintf(src_str, sizeof(src_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             src[0], src[1], src[2], src[3], src[4], src[5]);
    snprintf(dst_str, sizeof(dst_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
    
    if (is_arp) {
        snprintf(packet_buffer, sizeof(packet_buffer), 
                "🔵 ARP %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    } else if (dst_port == -1) {
        snprintf(packet_buffer, sizeof(packet_buffer), 
                "🟡 FLOOD %s -> %s [%d]", src_str, dst_str, src_port);
    } else if (known_mac) {
        snprintf(packet_buffer, sizeof(packet_buffer), 
                "🟢 L2 %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    } else {
        snprintf(packet_buffer, sizeof(packet_buffer), 
                "⚪ UNKNOWN %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    }
    
    dashboard_add_packet(packet_buffer);
}

void visual_mac_table_dashboard(uint8_t macs[][6], int ports[], int count) {
    // This is handled by dashboard_update_thread
}

void visual_arp_table_dashboard(uint32_t ips[], uint8_t macs[][6], int count) {
    // This is handled by dashboard_update_thread
}

// Initialize dashboard mode
void visual_init_dashboard(void) {
    dashboard_init();
    dashboard_clear_packets();
    
    // Start dashboard update thread
    if (pthread_create(&dashboard_thread, NULL, dashboard_update_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create dashboard thread\n");
        return;
    }
    
    printf("Dashboard initialized. Press 'q' to quit.\n");
}

// Cleanup dashboard mode
void visual_cleanup_dashboard(void) {
    dashboard_running = 0;
    pthread_join(dashboard_thread, NULL);
    dashboard_cleanup();
    printf("Dashboard cleaned up.\n");
}

// Check if dashboard should exit
int visual_dashboard_should_exit(void) {
    return dashboard_should_exit();
}
