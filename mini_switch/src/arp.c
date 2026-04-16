#include "common.h"
#include "arp.h"
#include "ethernet.h"
#include "port.h"
#include "forwarding.h"
#include "visual.h"
#include "mac_table.h"
#include "cross_platform.h"
#include "net_utils.h"

static arp_entry_t arp_table[ARP_TABLE_SIZE];
static mutex_t arp_mutex;

// Helper: check if ARP entry matches given IP
static bool arp_match(const void *entry, const void *key) {
    const arp_entry_t *e = entry;
    const uint32_t *ip = key;
    if (!ip) return e->ip == 0; // Empty slot check
    return e->ip == *ip;
}

// Helper: copy ARP entry data
static void arp_copy(void *dest, const void *src) {
    arp_entry_t *d = dest;
    const arp_entry_t *s = src;
    *d = *s;
}

// Helper: clear ARP entry
static void arp_clear(void *entry) {
    arp_entry_t *e = entry;
    memset(e, 0, sizeof(*e));
}

void arp_init(void) {
    table_init(arp_table, sizeof(arp_entry_t), ARP_TABLE_SIZE, arp_clear);
    MUTEX_INIT(arp_mutex);
}

void arp_learn(uint32_t ip, const uint8_t *mac) {
    MUTEX_LOCK(arp_mutex);
    
    arp_entry_t new_entry = {
        .ip = ip,
        .last_seen = time(NULL)
    };
    memcpy(new_entry.mac, mac, 6);
    
    int idx = table_find_entry(arp_table, sizeof(arp_entry_t), ARP_TABLE_SIZE, &ip, arp_match);
    
    if (idx >= 0) {
        // Update existing
        arp_copy(&arp_table[idx], &new_entry);
    } else {
        // Find slot for new entry
        idx = table_find_oldest(arp_table, sizeof(arp_entry_t), ARP_TABLE_SIZE, arp_match);
        arp_copy(&arp_table[idx], &new_entry);
    }
    
    printf("ARP learned: ");
    print_ip(ip);
    printf(" -> ");
    print_mac(mac);
    printf("\n");
    
    MUTEX_UNLOCK(arp_mutex);
}

uint8_t* arp_lookup(uint32_t ip) {
    MUTEX_LOCK(arp_mutex);
    
    time_t now = time(NULL);
    
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].ip == ip && 
            (now - arp_table[i].last_seen) < ARP_TIMEOUT) {
            MUTEX_UNLOCK(arp_mutex);
            return arp_table[i].mac;
        }
    }
    
    MUTEX_UNLOCK(arp_mutex);
    return NULL;
}

void send_arp_request(uint32_t target_ip, int out_port) {
    uint8_t packet[64];
    struct eth_hdr *eth = (struct eth_hdr*)packet;
    struct arp_hdr *arp = (struct arp_hdr*)(packet + sizeof(*eth));
    
    // Get interface MAC address first
    struct ifreq ifr;
    strncpy(ifr.ifr_name, ports[out_port].name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}; // Default MAC
    
    if (ioctl(ports[out_port].sock, SIOCGIFHWADDR, &ifr) == 0) {
        memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);
    }
    
    // Fill Ethernet header
    memset(eth->dst, 0xff, 6); // Broadcast
    memcpy(eth->src, src_mac, 6);
    eth->ethertype = htons(ETH_P_ARP);
    
    // Fill ARP header
    arp->htype = htons(ARP_HTYPE_ETHER);
    arp->ptype = htons(ARP_PTYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OPER_REQUEST);
    
    memcpy(arp->sha, src_mac, 6);
    arp->spa = htonl(0x0a000001); // Use 10.0.0.1 as source
    memset(arp->tha, 0, 6);
    arp->tpa = target_ip;
    
    tx_send(out_port, packet, sizeof(*eth) + sizeof(*arp));
}

void send_arp_reply(const uint8_t *dst_mac, uint32_t dst_ip, 
                   const uint8_t *src_mac, uint32_t src_ip, int out_port) {
    uint8_t packet[64];
    struct eth_hdr *eth = (struct eth_hdr*)packet;
    struct arp_hdr *arp = (struct arp_hdr*)(packet + sizeof(*eth));
    
    memcpy(eth->dst, dst_mac, 6);
    memcpy(eth->src, src_mac, 6);
    eth->ethertype = htons(ETH_P_ARP);
    
    arp->htype = htons(ARP_HTYPE_ETHER);
    arp->ptype = htons(ARP_PTYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OPER_REPLY);
    
    memcpy(arp->sha, src_mac, 6);
    arp->spa = src_ip;
    memcpy(arp->tha, dst_mac, 6);
    arp->tpa = dst_ip;
    
    tx_send(out_port, packet, sizeof(*eth) + sizeof(*arp));
}

void handle_arp(packet_t *pkt, int ingress_port) {
    struct eth_hdr *eth = (struct eth_hdr*)pkt->data;
    struct arp_hdr *arp = (struct arp_hdr*)(pkt->data + sizeof(*eth));
    
    printf("🔵 [ARP] Port %d: ", ingress_port);
    if (ntohs(arp->oper) == ARP_OPER_REQUEST) {
        printf("REQUEST %s -> ", inet_ntoa(*(struct in_addr*)&arp->spa));
        printf("%s\n", inet_ntoa(*(struct in_addr*)&arp->tpa));
        
        // Learn the sender's ARP entry
        arp_learn(arp->spa, arp->sha);
        
        // Check if this is a request for another host on the same network
        uint32_t target_ip = arp->tpa;
        uint32_t network = htonl(0x0a000000); // 10.0.0.0/24
        uint32_t mask = htonl(0xffffff00);     // 255.255.255.0
        
        // Check if target IP is in the same subnet
        if ((target_ip & mask) == network) {
            // Look up if we know the MAC for this IP
            uint8_t *target_mac = arp_lookup(target_ip);
            if (target_mac != NULL) {
                printf("🔵 [ARP] Reply with known MAC\n");
                // We know the MAC, send ARP reply
                struct ifreq ifr;
                strncpy(ifr.ifr_name, ports[ingress_port].name, IFNAMSIZ - 1);
                ifr.ifr_name[IFNAMSIZ - 1] = '\0';
                
                uint8_t switch_mac[6];
                if (ioctl(ports[ingress_port].sock, SIOCGIFHWADDR, &ifr) == 0) {
                    memcpy(switch_mac, ifr.ifr_hwaddr.sa_data, 6);
                    send_arp_reply(eth->src, arp->spa, switch_mac, target_ip, ingress_port);
                }
            } else {
                printf("🔵 [ARP] Flooding request (unknown MAC)\n");
                // We don't know the MAC, flood the ARP request to other ports
                tx_flood_exclude(pkt, ingress_port);
            }
        }
    } else if (ntohs(arp->oper) == ARP_OPER_REPLY) {
        printf("REPLY %s -> ", inet_ntoa(*(struct in_addr*)&arp->spa));
        printf("%s\n", inet_ntoa(*(struct in_addr*)&arp->tpa));
        
        // Learn from ARP reply
        arp_learn(arp->spa, arp->sha);
        
        // Forward ARP reply to the correct port if we know where the target is
        uint8_t *target_mac = arp_lookup(arp->tpa);
        if (target_mac != NULL) {
            // Find which port has this MAC
            int dst_port = mac_lookup(arp->tha, 0); // VLAN 0 for now
            if (dst_port >= 0 && dst_port != ingress_port) {
                printf("🔵 [ARP] Forwarding reply to port %d\n", dst_port);
                tx_send(dst_port, pkt->data, pkt->len);
            } else {
                printf("🔵 [ARP] Flooding reply (unknown port)\n");
                tx_flood_exclude(pkt, ingress_port);
            }
        } else {
            printf("🔵 [ARP] Flooding reply (unknown MAC)\n");
            // Don't know where to send, flood
            tx_flood_exclude(pkt, ingress_port);
        }
    }
}

void arp_table_get_entries(uint32_t ips[], uint8_t macs[][6], int *count) {
    MUTEX_LOCK(arp_mutex);
    
    time_t now = time(NULL);
    *count = 0;
    
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].ip != 0 && (now - arp_table[i].last_seen) < 300) {
            ips[*count] = arp_table[i].ip;
            memcpy(macs[*count], arp_table[i].mac, 6);
            (*count)++;
        }
    }
    
    MUTEX_UNLOCK(arp_mutex);
}
