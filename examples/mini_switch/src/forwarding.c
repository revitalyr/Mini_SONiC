#include "common.h"
#include "forwarding.h"
#include "ethernet.h"
#include "mac_table.h"
#include "arp.h"
#include "ip.h"
#include "vlan.h"
#include "port.h"
#include "visual.h"

void forward_init(void) {
    printf("Forwarding engine initialized\n");
}

void forward_frame(packet_t *pkt, int ingress_port) {
    struct eth_hdr *eth = (struct eth_hdr*)pkt->data;
    
    printf("🔄 [FORWARD] Frame: %zu bytes, port %d\n", pkt->len, ingress_port);
    
    if (pkt->len < sizeof(*eth)) {
        printf("❌ [FORWARD] Frame too short: %zu < %zu\n", pkt->len, sizeof(*eth));
        return;
    }
    
    uint16_t ethertype = ntohs(eth->ethertype);
    printf("🔄 [FORWARD] EtherType: 0x%04x (%s)\n", ethertype, 
           ethertype == ETH_P_ARP ? "ARP" : 
           ethertype == ETH_P_IP ? "IP" : "OTHER");
    
    uint16_t vlan = 0;
    uint8_t *payload = pkt->data + sizeof(*eth);
    
    if (ethertype == ETH_P_8021Q) {
        if (pkt->len < sizeof(*eth) + 4) {
            printf("❌ [FORWARD] VLAN frame too short\n");
            return;
        }
        
        vlan_hdr_t *vh = (vlan_hdr_t*)payload;
        vlan = vlan_id(ntohs(vh->tci));
        ethertype = ntohs(*(uint16_t*)(payload + 4));
        payload += 4;
        printf("🔄 [FORWARD] VLAN ID: %d, inner EtherType: 0x%04x\n", vlan, ethertype);
    }
    
    if (!port_vlan_allowed(ingress_port, vlan)) {
        printf("❌ [FORWARD] VLAN %d not allowed on port %d\n", vlan, ingress_port);
        return;
    }
    
    mac_learn(eth->src, ingress_port, vlan);
    
    if (ethertype == ETH_P_ARP) {
        printf("🔄 [FORWARD] Handling ARP packet\n");
        handle_arp(pkt, ingress_port);
        return;
    }
    
    if (ethertype == ETH_P_IP) {
        printf("🔄 [FORWARD] Handling IP packet\n");
        handle_ip(pkt, ingress_port);
        return;
    }
    
    int dst_port = mac_lookup(eth->dst, vlan);
    
    // Determine packet type for visualization
    int is_arp_pkt = (ethertype == ETH_P_ARP);
    int vlan_id_display = (vlan > 0) ? vlan : -1;
    int known_mac = (dst_port >= 0 && dst_port != ingress_port);
    int out_port = dst_port;
    
    if (dst_port >= 0 && dst_port != ingress_port) {
        printf("🔄 [FORWARD] Forwarding to known port %d\n", dst_port);
        tx_send(dst_port, pkt->data, pkt->len);
    } else if (is_broadcast(eth->dst) || is_multicast(eth->dst) || dst_port < 0) {
        printf("🔄 [FORWARD] Flooding (unknown MAC)\n");
        tx_flood_exclude(pkt, ingress_port);
        out_port = -1; // Flood
    }
    
    // Visual output
    visual_packet(eth->src, eth->dst, ingress_port, out_port, known_mac, is_arp_pkt, vlan_id_display);
}

void tx_flood_exclude(packet_t *pkt, int exclude_port) {
    printf("🌊 [FLOOD] Excluding port %d, flooding to %zd ports\n", exclude_port, port_count - 1);
    for (int i = 0; i < port_count; i++) {
        if (i != exclude_port && ports[i].active) {
            printf("🌊 [FLOOD] -> Port %d\n", i);
            tx_send(i, pkt->data, pkt->len);
        }
    }
}

void tx_send(int port, const uint8_t *buf, size_t len) {
    if (port < 0 || port >= port_count || !ports[port].active) {
        printf("❌ [TX] Invalid port %d\n", port);
        return;
    }
    
    printf("📤 [TX] Port %d: %zu bytes\n", port, len);
    ssize_t sent = send(ports[port].sock, buf, len, 0);
    if (sent < 0) {
        perror("send");
        printf("❌ [TX] Send failed: %s\n", strerror(errno));
    } else {
        __sync_fetch_and_add((volatile PacketCount *)&global_tx_count, 1);
        printf("✅ [TX] Sent %zd bytes to port %d\n", sent, port);
    }
}
