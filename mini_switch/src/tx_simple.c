#include "common.h"
#include "port.h"
#include <stdio.h>

// Simple transmission functions for demo mode
int tx_send(int port, const uint8_t *data, size_t len) {
    printf("📤 [TX] Port %d: %zu bytes\n", port, len);
    // In real implementation, this would send the packet
    return 0;
}

int tx_flood_exclude(packet_t *pkt, int exclude_port) {
    printf("🌊 [FLOOD] Excluding port %d, flooding to %d ports\n", 
           exclude_port, port_count - 1);
    
    for (int i = 0; i < port_count; i++) {
        if (i != exclude_port) {
            printf("📤 [TX] Port %d: %zu bytes\n", i, pkt->len);
        }
    }
    return 0;
}
