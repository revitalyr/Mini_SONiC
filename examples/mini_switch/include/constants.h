#pragma once

#include <stdint.h>

// =============================================================================
// NETWORK CONSTANTS
// =============================================================================

// Ethernet protocol types
#define ETH_P_ARP 0x0806
#define ETH_P_IP 0x0800
#define ETH_P_8021Q 0x8100
#define ETH_P_IPV6 0x86DD
#define ETH_P_ALL 3

// IP protocol types
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP 6
#define IP_PROTOCOL_UDP 17

// ARP constants
#define ARP_HTYPE_ETHER 1
#define ARP_PTYPE_IP 0x0800
#define ARP_OPER_REQUEST 1
#define ARP_OPER_REPLY 2
#define ARP_TIMEOUT 300

// ICMP types
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0

// =============================================================================
// BUFFER AND TABLE SIZES
// =============================================================================

#define BUFFER_SIZE 2048
#define MAX_PORTS 8
#define MAC_TABLE_SIZE 1024
#define ARP_TABLE_SIZE 256
#define VLAN_TABLE_SIZE 64
#define RING_SIZE 1024
#define MAC_TIMEOUT 300

// =============================================================================
// SWITCH EVENT FLAGS
// =============================================================================

#define SWITCH_EVENT_PACKET_RECEIVED    (1U << 0)
#define SWITCH_EVENT_PACKET_SENT        (1U << 1)
#define SWITCH_EVENT_LINK_UP            (1U << 2)
#define SWITCH_EVENT_LINK_DOWN          (1U << 3)
#define SWITCH_EVENT_ERROR              (1U << 4)
#define SWITCH_EVENT_MAINTENANCE        (1U << 5)

// =============================================================================
// STRING LITERALS
// =============================================================================

#define DEFAULT_BIND_ADDRESS "0.0.0.0"
#define DEFAULT_LOOPBACK_ADDRESS "127.0.0.1"
