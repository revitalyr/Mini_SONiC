#pragma once
#include <stdint.h>
#include "cross_platform.h" // For PACKED_STRUCT macros

START_PACKED_STRUCT
struct ip_hdr {
#if PLATFORM_IS_BIG_ENDIAN
    uint8_t version:4;
    uint8_t ihl:4;
#else
    uint8_t ihl:4;
    uint8_t version:4;
#endif
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} PACKED_STRUCT;
END_PACKED_STRUCT

START_PACKED_STRUCT
struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t check;
    uint16_t id;
    uint16_t sequence;
} PACKED_STRUCT;
END_PACKED_STRUCT

#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

// Legacy compatibility aliases (used by ip.c)
#define IP_ICMP IP_PROTOCOL_ICMP
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0

// Compatibility: old code uses 'iphdr' and 'icmphdr'
#define iphdr ip_hdr
#define icmphdr icmp_hdr