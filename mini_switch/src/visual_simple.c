#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

// Forward declarations
void visual_packet_simple(const uint8_t *src, const uint8_t *dst, 
                       int src_port, int dst_port, int known_mac,
                       int is_arp, int vlan_id);
void visual_mac_table_simple(uint8_t macs[][6], int ports[], int count);
void visual_arp_table_simple(uint32_t ips[], uint8_t macs[][6], int count);

// Simple visual functions without dashboard dependencies
static void print_mac(const uint8_t *mac) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           (unsigned int)mac[0], (unsigned int)mac[1], (unsigned int)mac[2], 
           (unsigned int)mac[3], (unsigned int)mac[4], (unsigned int)mac[5]);
}

void visual_packet_simple(const uint8_t *src, const uint8_t *dst, 
                       int src_port, int dst_port, int known_mac,
                       int is_arp, int vlan_id) {
    char src_str[18], dst_str[18];
    
    snprintf(src_str, sizeof(src_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)src[0], (unsigned int)src[1], (unsigned int)src[2], 
             (unsigned int)src[3], (unsigned int)src[4], (unsigned int)src[5]);
    snprintf(dst_str, sizeof(dst_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)dst[0], (unsigned int)dst[1], (unsigned int)dst[2], 
             (unsigned int)dst[3], (unsigned int)dst[4], (unsigned int)dst[5]);
    
    if (is_arp) {
        printf("🔵 ARP %s -> %s [%d->%d]\n", src_str, dst_str, src_port, dst_port);
    } else if (dst_port == -1) {
        printf("🟡 FLOOD %s -> %s [%d]\n", src_str, dst_str, src_port);
    } else if (known_mac) {
        printf("🟢 L2 %s -> %s [%d->%d]\n", src_str, dst_str, src_port, dst_port);
    } else {
        printf("⚪ UNKNOWN %s -> %s [%d->%d]\n", src_str, dst_str, src_port, dst_port);
    }
}

void visual_mac_table_simple(uint8_t macs[][6], int ports[], int count) {
    printf("\n=== MAC Table ===\n");
    for (int i = 0; i < count; i++) {
        printf("Port %d: ", ports[i]);
        print_mac(macs[i]);
        printf("\n");
    }
    printf("================\n\n");
}

void visual_mac_table(uint8_t macs[][6], int ports[], int count) {
    visual_mac_table_simple(macs, ports, count);
}

void visual_arp_table_simple(uint32_t ips[], uint8_t macs[][6], int count) {
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
