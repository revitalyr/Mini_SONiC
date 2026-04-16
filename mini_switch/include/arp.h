#pragma once
#include <stdint.h>
#include "common.h"
#include "cross_platform.h" // For PACKED_STRUCT macros

START_PACKED_STRUCT
struct arp_hdr {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} PACKED_STRUCT;
END_PACKED_STRUCT

#define ARP_HTYPE_ETHER 1
#define ARP_PTYPE_IP 0x0800
#define ARP_OPER_REQUEST 1
#define ARP_OPER_REPLY 2
#define ARP_TIMEOUT 300

// ARP table entry typedef
typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    time_t last_seen;
} arp_entry_t;

// Function declarations
void arp_init(void);
void handle_arp(packet_t *pkt, int ingress_port);
void arp_learn(uint32_t ip, const uint8_t *mac);
uint8_t* arp_lookup(uint32_t ip);
void send_arp_request(uint32_t target_ip, int out_port);
void send_arp_reply(const uint8_t *dst_mac, uint32_t dst_ip, 
                   const uint8_t *src_mac, uint32_t src_ip, int out_port);