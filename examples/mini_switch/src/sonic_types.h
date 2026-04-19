/**
 * @file sonic_types.h
 * @brief SONiC Basic Type Definitions
 * 
 * This file contains basic type definitions and constants for SONiC implementation
 * without external dependencies.
 */

#ifndef SONIC_TYPES_H
#define SONIC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// BASIC TYPES
// =============================================================================

/**
 * @brief SONiC status codes
 */
typedef enum {
    SONIC_STATUS_SUCCESS = 0,      ///< Operation successful
    SONIC_STATUS_FAILURE = -1,     ///< Operation failed
    SONIC_STATUS_INVALID = -2,     ///< Invalid parameters
    SONIC_STATUS_NOT_FOUND = -3,   ///< Entry not found
    SONIC_STATUS_EXISTS = -4,      ///< Entry already exists
    SONIC_STATUS_NO_MEMORY = -5,   ///< Out of memory
    SONIC_STATUS_TIMEOUT = -6,     ///< Operation timeout
    SONIC_STATUS_BUSY = -7         ///< Resource busy
} sonic_status_t;

/**
 * @brief Log levels
 */
typedef enum {
    LOG_ERROR = 0,     ///< Error messages
    LOG_WARN = 1,      ///< Warning messages
    LOG_INFO = 2,      ///< Information messages
    LOG_DEBUG = 3      ///< Debug messages
} sonic_log_level_t;

/**
 * @brief Simple string structure
 */
typedef struct {
    char *data;          ///< String data
    uint16_t length;     ///< String length
} sonic_string_t;

/**
 * @brief Simple list structure
 */
typedef struct sonic_list_node {
    void *data;                           ///< Node data
    struct sonic_list_node *next;            ///< Next node
} sonic_list_node_t;

typedef struct {
    sonic_list_node_t *head;               ///< List head
    sonic_list_node_t *tail;               ///< List tail
    uint16_t count;                       ///< Node count
} sonic_list_t;

/**
 * @brief Simple hash table structure
 */
typedef struct sonic_hash_entry {
    char *key;                            ///< Hash key
    void *value;                          ///< Hash value
    struct sonic_hash_entry *next;            ///< Next entry (for chaining)
} sonic_hash_entry_t;

typedef struct {
    sonic_hash_entry_t **buckets;            ///< Hash buckets
    uint16_t bucket_count;                  ///< Number of buckets
    uint16_t entry_count;                   ///< Number of entries
} sonic_hash_table_t;

// =============================================================================
// NETWORK TYPES
// =============================================================================

/**
 * @brief MAC address structure
 */
typedef struct {
    uint8_t bytes[6];      ///< MAC address bytes
} sonic_mac_t;

/**
 * @brief IPv4 address structure
 */
typedef struct {
    uint8_t bytes[4];      ///< IPv4 address bytes
} sonic_ipv4_t;

/**
 * @brief IPv6 address structure
 */
typedef struct {
    uint8_t bytes[16];     ///< IPv6 address bytes
} sonic_ipv6_t;

/**
 * @brief IP address union
 */
typedef union {
    sonic_ipv4_t v4;     ///< IPv4 address
    sonic_ipv6_t v6;     ///< IPv6 address
} sonic_ip_t;

/**
 * @brief Ethernet frame header
 */
typedef struct {
    sonic_mac_t dst_mac;    ///< Destination MAC
    sonic_mac_t src_mac;    ///< Source MAC
    uint16_t ether_type;    ///< EtherType
} sonic_ethernet_header_t;

/**
 * @brief IPv4 header
 */
typedef struct {
    uint8_t version_ihl;    ///< Version and IHL
    uint8_t tos;           ///< Type of Service
    uint16_t total_length;   ///< Total Length
    uint16_t identification; ///< Identification
    uint16_t flags_fragment; ///< Flags and Fragment
    uint8_t ttl;           ///< Time to Live
    uint8_t protocol;       ///< Protocol
    uint16_t header_checksum; ///< Header Checksum
    sonic_ipv4_t src_ip;   ///< Source IP
    sonic_ipv4_t dst_ip;   ///< Destination IP
} sonic_ipv4_header_t;

/**
 * @brief ARP packet structure
 */
typedef struct {
    uint16_t hardware_type;   ///< Hardware Type
    uint16_t protocol_type;   ///< Protocol Type
    uint8_t hardware_len;    ///< Hardware Address Length
    uint8_t protocol_len;    ///< Protocol Address Length
    uint16_t opcode;         ///< Operation Code
    sonic_mac_t src_mac;     ///< Sender MAC Address
    sonic_ip_t src_ip;       ///< Sender IP Address
    sonic_mac_t dst_mac;     ///< Target MAC Address
    sonic_ip_t dst_ip;       ///< Target IP Address
} sonic_arp_packet_t;

// =============================================================================
// TIMING AND SYNCHRONIZATION
// =============================================================================

/**
 * @brief Simple timer structure
 */
typedef struct {
    uint64_t start_time;    ///< Start timestamp
    uint64_t end_time;      ///< End timestamp
    bool running;           ///< Running state
} sonic_timer_t;

/**
 * @brief Timestamp structure
 */
typedef struct {
    uint32_t seconds;       ///< Seconds
    uint32_t nanoseconds;   ///< Nanoseconds
} sonic_timestamp_t;

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

#define SONIC_MAX_STRING_LENGTH    256     ///< Maximum string length
#define SONIC_MAX_HOSTNAME_LENGTH  64      ///< Maximum hostname length
#define SONIC_MAX_INTERFACE_NAME   32      ///< Maximum interface name
#define SONIC_MAX_VLAN_NAME        32      ///< Maximum VLAN name
#define SONIC_MAX_PORT_NAME       32      ///< Maximum port name

// Network constants
#define SONIC_MAC_BROADCAST       {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define SONIC_MAC_MULTICAST       {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00}
#define SONIC_IPV4_BROADCAST     {255, 255, 255, 255}
#define SONIC_IPV4_LOOPBACK       {127, 0, 0, 1}
#define SONIC_IPV6_LOOPBACK       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}

// Protocol constants
#define SONIC_ETHERTYPE_IPV4      0x0800  ///< IPv4 EtherType
#define SONIC_ETHERTYPE_IPV6      0x86DD  ///< IPv6 EtherType
#define SONIC_ETHERTYPE_ARP       0x0806  ///< ARP EtherType
#define SONIC_ETHERTYPE_VLAN       0x8100  ///< VLAN EtherType

#define SONIC_IP_PROTOCOL_TCP       6       ///< TCP Protocol
#define SONIC_IP_PROTOCOL_UDP       17      ///< UDP Protocol
#define SONIC_IP_PROTOCOL_ICMP      1       ///< ICMP Protocol

#define SONIC_ARP_REQUEST        1       ///< ARP Request
#define SONIC_ARP_REPLY          2       ///< ARP Reply

// =============================================================================
// UTILITY MACROS
// =============================================================================

/**
 * @brief Check if MAC address is broadcast
 */
#define SONIC_MAC_IS_BROADCAST(mac) \
    ((mac).bytes[0] == 0xFF && (mac).bytes[1] == 0xFF && \
     (mac).bytes[2] == 0xFF && (mac).bytes[3] == 0xFF && \
     (mac).bytes[4] == 0xFF && (mac).bytes[5] == 0xFF)

/**
 * @brief Check if MAC address is multicast
 */
#define SONIC_MAC_IS_MULTICAST(mac) \
    ((mac).bytes[0] == 0x01 && (mac).bytes[1] == 0x00 && \
     (mac).bytes[2] == 0x5E)

/**
 * @brief Check if MAC address is zero
 */
#define SONIC_MAC_IS_ZERO(mac) \
    ((mac).bytes[0] == 0x00 && (mac).bytes[1] == 0x00 && \
     (mac).bytes[2] == 0x00 && (mac).bytes[3] == 0x00 && \
     (mac).bytes[4] == 0x00 && (mac).bytes[5] == 0x00)

/**
 * @brief Compare two MAC addresses
 */
#define SONIC_MAC_EQUAL(mac1, mac2) \
    (memcmp((mac1).bytes, (mac2).bytes, 6) == 0)

/**
 * @brief Copy MAC address
 */
#define SONIC_MAC_COPY(dst, src) \
    memcpy((dst).bytes, (src).bytes, 6)

/**
 * @brief Check if IPv4 address is broadcast
 */
#define SONIC_IPV4_IS_BROADCAST(ip) \
    ((ip).bytes[0] == 255 && (ip).bytes[1] == 255 && \
     (ip).bytes[2] == 255 && (ip).bytes[3] == 255)

/**
 * @brief Check if IPv4 address is loopback
 */
#define SONIC_IPV4_IS_LOOPBACK(ip) \
    ((ip).bytes[0] == 127 && (ip).bytes[1] == 0 && \
     (ip).bytes[2] == 0 && (ip).bytes[3] == 1)

/**
 * @brief Compare two IPv4 addresses
 */
#define SONIC_IPV4_EQUAL(ip1, ip2) \
    (memcmp((ip1).bytes, (ip2).bytes, 4) == 0)

/**
 * @brief Copy IPv4 address
 */
#define SONIC_IPV4_COPY(dst, src) \
    memcpy((dst).bytes, (src).bytes, 4)

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

// String utilities
int sonic_string_compare(const char *str1, const char *str2);
char *sonic_string_duplicate(const char *str);
void sonic_string_free(char *str);

// List utilities
sonic_list_t *sonic_list_create(void);
void sonic_list_destroy(sonic_list_t *list);
int sonic_list_add(sonic_list_t *list, void *data);
int sonic_list_remove(sonic_list_t *list, void *data);
void *sonic_list_get(sonic_list_t *list, uint16_t index);
uint16_t sonic_list_count(sonic_list_t *list);

// Hash table utilities
sonic_hash_table_t *sonic_hash_create(uint16_t bucket_count);
void sonic_hash_destroy(sonic_hash_table_t *table);
int sonic_hash_set(sonic_hash_table_t *table, const char *key, void *value);
void *sonic_hash_get(sonic_hash_table_t *table, const char *key);
int sonic_hash_remove(sonic_hash_table_t *table, const char *key);

// Timer utilities
void sonic_timer_start(sonic_timer_t *timer);
void sonic_timer_stop(sonic_timer_t *timer);
uint64_t sonic_timer_elapsed_ms(sonic_timer_t *timer);
uint64_t sonic_timer_elapsed_us(sonic_timer_t *timer);

// Timestamp utilities
sonic_timestamp_t sonic_get_timestamp(void);
uint64_t sonic_timestamp_to_ms(sonic_timestamp_t ts);
uint64_t sonic_timestamp_to_us(sonic_timestamp_t ts);

// Network utilities
void sonic_mac_to_string(const sonic_mac_t *mac, char *str);
void sonic_mac_from_string(const char *str, sonic_mac_t *mac);
void sonic_ipv4_to_string(const sonic_ipv4_t *ip, char *str);
void sonic_ipv4_from_string(const char *str, sonic_ipv4_t *ip);
uint16_t sonic_calculate_checksum(const void *data, uint16_t length);

// Logging utilities
void sonic_log_init(sonic_log_level_t level);
void sonic_log(sonic_log_level_t level, const char *format, ...);
void sonic_log_cleanup(void);

#endif // SONIC_TYPES_H
