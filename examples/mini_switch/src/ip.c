#include "common.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "forwarding.h"
#include "mac_table.h"

uint16_t ip_checksum(void *data, int len) {
    uint16_t *buf = (uint16_t*)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    if (len == 1) {
        sum += *(uint8_t*)buf;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    
    return (uint16_t)(~sum);
}

uint16_t icmp_checksum(void *data, int len) {
    return ip_checksum(data, len);
}

int is_local_ip(uint32_t ip) {
    return (ip & htonl(0xFFFFFF00)) == htonl(0x0A000000);
}

void send_icmp_reply(packet_t *orig_pkt, int ingress_port) {
    struct eth_hdr *orig_eth = (struct eth_hdr*)orig_pkt->data;
    struct iphdr *orig_ip = (struct iphdr*)(orig_pkt->data + sizeof(*orig_eth));
    struct icmphdr *orig_icmp = (struct icmphdr*)(orig_pkt->data + sizeof(*orig_eth) + orig_ip->ihl * 4);
    
    uint8_t reply_packet[BUFFER_SIZE];
    struct eth_hdr *reply_eth = (struct eth_hdr*)reply_packet;
    struct iphdr *reply_ip = (struct iphdr*)(reply_packet + sizeof(*reply_eth));
    struct icmphdr *reply_icmp = (struct icmphdr*)(reply_packet + sizeof(*reply_eth) + sizeof(*reply_ip));
    
    memcpy(reply_eth->dst, orig_eth->src, 6);
    memcpy(reply_eth->src, orig_eth->dst, 6);
    reply_eth->ethertype = orig_eth->ethertype;
    
    reply_ip->version = 4;
    reply_ip->ihl = 5;
    reply_ip->tos = 0;
    reply_ip->len = htons(sizeof(*reply_ip) + sizeof(*reply_icmp));
    reply_ip->id = 0;
    reply_ip->frag_off = 0;
    reply_ip->ttl = 64;
    reply_ip->protocol = IP_ICMP;
    reply_ip->check = 0;
    reply_ip->saddr = orig_ip->daddr;
    reply_ip->daddr = orig_ip->saddr;
    reply_ip->check = ip_checksum(reply_ip, sizeof(*reply_ip));
    
    reply_icmp->type = ICMP_ECHO_REPLY;
    reply_icmp->code = 0;
    reply_icmp->check = 0;
    reply_icmp->id = orig_icmp->id;
    reply_icmp->sequence = orig_icmp->sequence;
    reply_icmp->check = icmp_checksum(reply_icmp, sizeof(*reply_icmp));
    
    tx_send(ingress_port, reply_packet, sizeof(*reply_eth) + sizeof(*reply_ip) + sizeof(*reply_icmp));
}

void handle_ip(packet_t *pkt, int ingress_port) {
    struct eth_hdr *eth = (struct eth_hdr*)pkt->data;
    struct iphdr *ip = (struct iphdr*)(pkt->data + sizeof(*eth));
    
    if (pkt->len < sizeof(*eth) + ip->ihl * 4) {
        return;
    }
    
    if (is_local_ip(ip->daddr)) {
        struct icmphdr *icmp = (struct icmphdr*)(pkt->data + sizeof(*eth) + ip->ihl * 4);
        
        if (ip->protocol == IP_ICMP && icmp->type == ICMP_ECHO_REQUEST) {
            send_icmp_reply(pkt, ingress_port);
        }
        return;
    }
    
    uint8_t *dst_mac = arp_lookup(ip->daddr);
    if (!dst_mac) {
        send_arp_request(ip->daddr, ingress_port);
        return;
    }
    
    uint8_t modified_packet[BUFFER_SIZE];
    memcpy(modified_packet, pkt->data, pkt->len);
    
    struct eth_hdr *mod_eth = (struct eth_hdr*)modified_packet;
    memcpy(mod_eth->dst, dst_mac, 6);
    memcpy(mod_eth->src, eth->dst, 6);
    
    int dst_port = mac_lookup(dst_mac, 0);
    if (dst_port >= 0 && dst_port != ingress_port) {
        tx_send(dst_port, modified_packet, pkt->len);
    }
}
