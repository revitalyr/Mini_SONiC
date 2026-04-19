#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos_config.h"
#include "switch_hal.h"

/* ── Switch Context Structure ───────────────────────────── */

/**
 * @brief Main switch context structure
 */
struct SwitchContext {
    /* Port management */
    SwitchPort_t ports[SWITCH_MAX_PORTS];
    uint8_t port_count;
    bool learning_enabled;
    bool forwarding_enabled;
    
    /* MAC address table */
    MacEntry_t mac_table[MAC_TABLE_SIZE];
    uint16_t mac_table_size;
    uint32_t mac_aging_time_ms;
    
    /* ARP cache */
    ArpEntry_t arp_table[ARP_TABLE_SIZE];
    uint16_t arp_table_size;
    uint32_t arp_aging_time_ms;
    
    /* Statistics */
    uint32_t total_rx_packets;
    uint32_t total_tx_packets;
    uint32_t total_rx_errors;
    uint32_t total_tx_errors;
    uint32_t learning_discards;
    uint32_t forwarding_discards;
    
    /* FreeRTOS objects */
    StaticQueue_t rx_queue_struct;
    uint8_t rx_queue_storage[PACKET_QUEUE_DEPTH * sizeof(NetPacket_t)];
    QueueHandle_t rx_queue;
    
    StaticQueue_t tx_queue_struct;
    uint8_t tx_queue_storage[PACKET_QUEUE_DEPTH * sizeof(NetPacket_t)];
    QueueHandle_t tx_queue;
    
    StaticSemaphore_t mac_mutex_struct;
    SemaphoreHandle_t mac_mutex;
    
    StaticSemaphore_t arp_mutex_struct;
    SemaphoreHandle_t arp_mutex;
    
    StaticTimer_t maintenance_timer_struct;
    TimerHandle_t maintenance_timer;
    
    /* Configuration */
    uint8_t switch_mac_addr[6U];
    uint32_t system_start_time_ms;
};

typedef struct SwitchContext SwitchContext_t;

/* ── Switch Core API ─────────────────────────────────────── */

/**
 * @brief Initialize switch core
 * @param[out] ctx Switch context pointer
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_Init(SwitchContext_t **ctx);

/**
 * @brief Deinitialize switch core
 * @param[in] ctx Switch context pointer
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_Deinit(SwitchContext_t *ctx);

/**
 * @brief Add port to switch
 * @param[in] ctx Switch context
 * @param[in] port_id Port identifier
 * @param[in] mac_addr Port MAC address
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_AddPort(SwitchContext_t *ctx, uint8_t port_id, 
                                  const uint8_t *mac_addr);

/**
 * @brief Remove port from switch
 * @param[in] ctx Switch context
 * @param[in] port_id Port identifier
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_RemovePort(SwitchContext_t *ctx, uint8_t port_id);

/**
 * @brief Enable/disable port
 * @param[in] ctx Switch context
 * @param[in] port_id Port identifier
 * @param[in] enable Enable/disable flag
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetPortState(SwitchContext_t *ctx, uint8_t port_id, 
                                     bool enable);

/**
 * @brief Configure VLAN for port
 * @param[in] ctx Switch context
 * @param[in] port_id Port identifier
 * @param[in] vlan_id VLAN identifier
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetPortVlan(SwitchContext_t *ctx, uint8_t port_id, 
                                     uint16_t vlan_id);

/**
 * @brief Process received packet
 * @param[in] ctx Switch context
 * @param[in] packet Received packet
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_ProcessPacket(SwitchContext_t *ctx, const NetPacket_t *packet);

/**
 * @brief Get MAC table entry
 * @param[in] ctx Switch context
 * @param[in] mac_addr MAC address to lookup
 * @param[out] port_id Associated port ID
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_GetMacEntry(SwitchContext_t *ctx, const uint8_t *mac_addr, 
                                     uint8_t *port_id);

/**
 * @brief Add MAC entry to table
 * @param[in] ctx Switch context
 * @param[in] mac_addr MAC address
 * @param[in] port_id Associated port ID
 * @param[in] vlan_id VLAN identifier
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_AddMacEntry(SwitchContext_t *ctx, const uint8_t *mac_addr, 
                                    uint8_t port_id, uint16_t vlan_id);

/**
 * @brief Remove MAC entry from table
 * @param[in] ctx Switch context
 * @param[in] mac_addr MAC address to remove
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_RemoveMacEntry(SwitchContext_t *ctx, const uint8_t *mac_addr);

/**
 * @brief Get ARP entry
 * @param[in] ctx Switch context
 * @param[in] ip_addr IPv4 address (network order)
 * @param[out] mac_addr Associated MAC address
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_GetArpEntry(SwitchContext_t *ctx, uint32_t ip_addr, 
                                     uint8_t *mac_addr);

/**
 * @brief Add ARP entry to cache
 * @param[in] ctx Switch context
 * @param[in] ip_addr IPv4 address (network order)
 * @param[in] mac_addr Associated MAC address
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_AddArpEntry(SwitchContext_t *ctx, uint32_t ip_addr, 
                                     const uint8_t *mac_addr);

/**
 * @brief Remove ARP entry from cache
 * @param[in] ctx Switch context
 * @param[in] ip_addr IPv4 address to remove
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_RemoveArpEntry(SwitchContext_t *ctx, uint32_t ip_addr);

/**
 * @brief Flush MAC table
 * @param[in] ctx Switch context
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_FlushMacTable(SwitchContext_t *ctx);

/**
 * @brief Flush ARP cache
 * @param[in] ctx Switch context
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_FlushArpCache(SwitchContext_t *ctx);

/**
 * @brief Get port statistics
 * @param[in] ctx Switch context
 * @param[in] port_id Port identifier
 * @param[out] rx_packets Received packet count
 * @param[out] tx_packets Transmitted packet count
 * @param[out] rx_errors Receive error count
 * @param[out] tx_errors Transmit error count
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_GetPortStats(SwitchContext_t *ctx, uint8_t port_id,
                                      uint32_t *rx_packets, uint32_t *tx_packets,
                                      uint32_t *rx_errors, uint32_t *tx_errors);

/**
 * @brief Get global switch statistics
 * @param[in] ctx Switch context
 * @param[out] total_rx_packets Total received packets
 * @param[out] total_tx_packets Total transmitted packets
 * @param[out] mac_table_entries Current MAC table entries
 * @param[out] arp_cache_entries Current ARP cache entries
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_GetGlobalStats(SwitchContext_t *ctx,
                                        uint32_t *total_rx_packets,
                                        uint32_t *total_tx_packets,
                                        uint32_t *mac_table_entries,
                                        uint32_t *arp_cache_entries);

/**
 * @brief Send ARP request
 * @param[in] ctx Switch context
 * @param[in] target_ip Target IPv4 address (network order)
 * @param[in] source_ip Source IPv4 address (network order)
 * @param[in] port_id Port to send from
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SendArpRequest(SwitchContext_t *ctx, uint32_t target_ip,
                                        uint32_t source_ip, uint8_t port_id);

/**
 * @brief Send ARP reply
 * @param[in] ctx Switch context
 * @param[in] target_mac Target MAC address
 * @param[in] target_ip Target IPv4 address (network order)
 * @param[in] source_mac Source MAC address
 * @param[in] source_ip Source IPv4 address (network order)
 * @param[in] port_id Port to send from
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SendArpReply(SwitchContext_t *ctx, const uint8_t *target_mac,
                                      uint32_t target_ip, const uint8_t *source_mac,
                                      uint32_t source_ip, uint8_t port_id);

/**
 * @brief Send ICMP echo reply
 * @param[in] ctx Switch context
 * @param[in] original_packet Original ICMP request packet
 * @param[in] port_id Port to send from
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SendIcmpReply(SwitchContext_t *ctx, 
                                        const NetPacket_t *original_packet,
                                        uint8_t port_id);

/**
 * @brief Enable/disable learning mode
 * @param[in] ctx Switch context
 * @param[in] enable Enable/disable MAC learning
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetLearningMode(SwitchContext_t *ctx, bool enable);

/**
 * @brief Enable/disable forwarding mode
 * @param[in] ctx Switch context
 * @param[in] enable Enable/disable packet forwarding
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetForwardingMode(SwitchContext_t *ctx, bool enable);

/**
 * @brief Set aging time for MAC table entries
 * @param[in] ctx Switch context
 * @param[in] aging_seconds Aging time in seconds
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetMacAgingTime(SwitchContext_t *ctx, uint32_t aging_seconds);

/**
 * @brief Set aging time for ARP cache entries
 * @param[in] ctx Switch context
 * @param[in] aging_seconds Aging time in seconds
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_SetArpAgingTime(SwitchContext_t *ctx, uint32_t aging_seconds);

/**
 * @brief Perform periodic maintenance (aging, cleanup)
 * @param[in] ctx Switch context
 * @return SwitchStatus_t Status code
 */
SwitchStatus_t SwitchCore_PeriodicMaintenance(SwitchContext_t *ctx);

/* ── Event Flags ───────────────────────────────────────── */

// Switch event flags are now defined in constants.h

/* ── Compile-time Assertions ───────────────────────────── */

#ifndef _WIN32
static_assert(sizeof(SwitchContext_t) <= 4096U, "SwitchContext_t too large");
#endif
static_assert(SWITCH_MAX_PORTS <= 255U, "Too many ports for uint8_t");
static_assert(MAC_TABLE_SIZE <= UINT16_MAX, "MAC table too large for uint16_t");
static_assert(ARP_TABLE_SIZE <= UINT16_MAX, "ARP table too large for uint16_t");
