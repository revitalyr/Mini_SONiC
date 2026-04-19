#include "visual.h"
#include "dashboard.h"
#include <stdio.h>
#include <string.h>

// ANSI colors
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"

// Local MAC print function
static void print_mac(const uint8_t *mac) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void visual_packet(const uint8_t *src, const uint8_t *dst, 
                 int src_port, int dst_port, int known_mac,
                 int is_arp, int vlan_id) {
    char packet_info[256];
    char src_str[18], dst_str[18];
    
    snprintf(src_str, sizeof(src_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             src[0], src[1], src[2], src[3], src[4], src[5]);
    snprintf(dst_str, sizeof(dst_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
    
    if (is_arp) {
        printf("🔵 [ARP] %s -> %s [%d->%d]\n", src_str, dst_str, src_port, dst_port);
        snprintf(packet_info, sizeof(packet_info), 
                "🔵 ARP %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    } else if (dst_port == -1) {
        printf("🟡 [FLOOD] Port %d -> ALL | ", src_port);
        print_mac(src);
        printf(" -> ");
        print_mac(dst);
        printf("\n");
        snprintf(packet_info, sizeof(packet_info), 
                "🟡 FLOOD %s -> %s [%d]", src_str, dst_str, src_port);
    } else if (known_mac) {
        printf("🟢 [L2] Port %d -> %d | ", src_port, dst_port);
        print_mac(src);
        printf(" -> ");
        print_mac(dst);
        printf("\n");
        snprintf(packet_info, sizeof(packet_info), 
                "🟢 L2 %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    } else {
        printf("⚪ [UNK] Port %d -> %d | ", src_port, dst_port);
        print_mac(src);
        printf(" -> ");
        print_mac(dst);
        printf("\n");
        snprintf(packet_info, sizeof(packet_info), 
                "⚪ UNKNOWN %s -> %s [%d->%d]", src_str, dst_str, src_port, dst_port);
    }
    
    // Also add to dashboard if available
    dashboard_add_packet(packet_info);
}

void visual_mac_table(uint8_t macs[][6], int ports[], int count) {
    printf("\n=== MAC Table ===\n");
    for (int i = 0; i < count; i++) {
        printf("Port %d: ", ports[i]);
        print_mac(macs[i]);
        printf("\n");
    }
    printf("================\n\n");
}

void visual_arp_table(uint32_t ips[], uint8_t macs[][6], int count) {
    printf("=== ARP Table ===\n");
    if (count == 0) {
        printf("(empty)\n");
    } else {
        for (int i = 0; i < count; i++) {
            struct in_addr addr;
            addr.s_addr = ips[i];
            printf("%s -> ", inet_ntoa(addr));
            print_mac(macs[i]);
            printf("\n");
        }
    }
    printf("================\n\n");
}

void dashboard_add_packet(const char *packet_info) {
    // Stub function for simple dashboard compatibility
    (void)packet_info;
}
