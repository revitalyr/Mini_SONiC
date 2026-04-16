/**
 * @file fdb_orch.c
 * @brief SONiC FDB (Forwarding Database) Orchestrator Implementation
 * 
 * FDB Orchestrator manages MAC learning and forwarding database operations.
 * It subscribes to FDB_TABLE changes in APP_DB and translates them to SAI calls.
 * 
 * Key Responsibilities:
 * - MAC address learning and aging
 * - FDB entry management (add/delete/update)
 * - BUM traffic handling (Broadcast, Unknown Unicast, Multicast)
 * - Hardware FDB synchronization via syncd
 */

#include "sonic_types.h"
#include "sonic_architecture.h"
#include "cross_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// FDB DATA STRUCTURES
// =============================================================================

/**
 * @brief FDB entry with additional metadata
 */
typedef struct fdb_entry_s {
    sonic_mac_t mac_address;        ///< MAC address
    uint16_t vlan_id;             ///< VLAN ID
    uint16_t port_id;              ///< Egress port
    fdb_entry_type_t type;         ///< Entry type
    uint32_t aging_time;           ///< Aging timer in seconds
    uint64_t last_seen;            ///< Last packet timestamp
    bool valid;                   ///< Entry validity
    struct fdb_entry_s *next;      ///< Hash table next pointer
} fdb_entry_s;

/**
 * @brief FDB orchestrator context
 */
struct fdb_orch_context {
    sonic_hash_table_t *fdb_table;  ///< FDB hash table
    uint16_t fdb_size;             ///< Current FDB size
    uint16_t max_entries;           ///< Maximum entries
    uint32_t aging_interval;        ///< Aging check interval (seconds)
    uint64_t packets_processed;       ///< Total packets processed
    uint64_t fdb_hits;             ///< FDB lookup hits
    uint64_t fdb_misses;           ///< FDB lookup misses
    uint64_t bum_packets;           ///< BUM packets processed
};

static struct fdb_orch_context g_fdb_orch;

// =============================================================================
// FDB HASH FUNCTIONS
// =============================================================================

/**
 * @brief Hash function for MAC addresses
 */
static uint32_t fdb_hash_mac(const sonic_mac_t *mac) {
    uint32_t hash = 0;
    for (int i = 0; i < 6; i++) {
        hash = hash * 31 + mac->bytes[i];
    }
    return hash % g_fdb_orch.fdb_table->bucket_count;
}

/**
 * @brief Find FDB entry by MAC address
 */
static fdb_entry_s *fdb_find_entry(const sonic_mac_t *mac) {
    uint32_t hash = fdb_hash_mac(mac);
    sonic_hash_entry_t *entry = g_fdb_orch.fdb_table->buckets[hash];
    
    while (entry) {
        fdb_entry_s *fdb_entry = (fdb_entry_s*)entry->value;
        if (SONIC_MAC_EQUAL(fdb_entry->mac_address, *mac)) {
            return fdb_entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief Add FDB entry to hash table
 */
static int fdb_add_entry(fdb_entry_s *entry) {
    uint32_t hash = fdb_hash_mac(&entry->mac_address);
    
    // Check if entry already exists
    fdb_entry_s *existing = fdb_find_entry(&entry->mac_address);
    if (existing) {
        // Update existing entry
        existing->port_id = entry->port_id;
        existing->vlan_id = entry->vlan_id;
        existing->last_seen = entry->last_seen;
        existing->aging_time = entry->aging_time;
        return SONIC_STATUS_SUCCESS;
    }
    
    // Add new entry
    if (g_fdb_orch.fdb_size >= g_fdb_orch.max_entries) {
        sonic_log(LOG_WARN, "FDB table full, cannot add entry");
        return SONIC_STATUS_NO_MEMORY;
    }
    
    sonic_hash_set(g_fdb_orch.fdb_table, 
                  (const char*)&entry->mac_address, entry);
    g_fdb_orch.fdb_size++;
    
    sonic_log(LOG_INFO, "Added FDB entry: MAC=%02X:%02X:%02X:%02X:%02X:%02X, "
               "VLAN=%d, PORT=%d", 
               entry->mac_address.bytes[0], entry->mac_address.bytes[1],
               entry->mac_address.bytes[2], entry->mac_address.bytes[3],
               entry->mac_address.bytes[4], entry->mac_address.bytes[5],
               entry->vlan_id, entry->port_id);
    
    return SONIC_STATUS_SUCCESS;
}

/**
 * @brief Remove FDB entry from hash table
 */
static int fdb_remove_entry(const sonic_mac_t *mac) {
    fdb_entry_s *entry = fdb_find_entry(mac);
    if (!entry) {
        return SONIC_STATUS_NOT_FOUND;
    }
    
    uint32_t hash = fdb_hash_mac(mac);
    sonic_hash_remove(g_fdb_orch.fdb_table, (const char*)mac);
    g_fdb_orch.fdb_size--;
    
    sonic_log(LOG_INFO, "Removed FDB entry: MAC=%02X:%02X:%02X:%02X:%02X:%02X",
               mac->bytes[0], mac->bytes[1], mac->bytes[2],
               mac->bytes[3], mac->bytes[4], mac->bytes[5]);
    
    free(entry);
    return SONIC_STATUS_SUCCESS;
}

// =============================================================================
// BUM TRAFFIC HANDLING
// =============================================================================

/**
 * @brief Determine BUM traffic type
 */
static bum_traffic_type_t classify_bum_traffic(const sonic_mac_t *dst_mac) {
    if (SONIC_MAC_IS_BROADCAST(*dst_mac)) {
        return BUM_TYPE_BROADCAST;
    } else if (SONIC_MAC_IS_MULTICAST(*dst_mac)) {
        return BUM_TYPE_MULTICAST;
    } else if (fdb_find_entry(dst_mac) == NULL) {
        return BUM_TYPE_UNKNOWN_UNICAST;
    }
    
    return BUM_TYPE_MAX; // Not BUM traffic
}

/**
 * @brief Handle BUM traffic (flooding)
 */
static int handle_bum_traffic(const sonic_mac_t *dst_mac, uint16_t vlan_id, 
                          uint16_t ingress_port) {
    bum_traffic_type_t bum_type = classify_bum_traffic(dst_mac);
    
    if (bum_type == BUM_TYPE_MAX) {
        return SONIC_STATUS_SUCCESS; // Not BUM traffic
    }
    
    g_fdb_orch.bum_packets++;
    
    // Flood to all ports in the VLAN except ingress port
    sonic_log(LOG_INFO, "BUM traffic detected: type=%d, VLAN=%d, ingress_port=%d",
               bum_type, vlan_id, ingress_port);
    
    // In real SONiC, this would translate to SAI flood operation
    // For demo, we just log the operation
    switch (bum_type) {
        case BUM_TYPE_BROADCAST:
            sonic_log(LOG_DEBUG, "Broadcast flooding to all VLAN %d ports", vlan_id);
            break;
        case BUM_TYPE_UNKNOWN_UNICAST:
            sonic_log(LOG_DEBUG, "Unknown unicast flooding to all VLAN %d ports", vlan_id);
            break;
        case BUM_TYPE_MULTICAST:
            sonic_log(LOG_DEBUG, "Multicast flooding to VLAN %d ports", vlan_id);
            break;
        default:
            break;
    }
    
    return SONIC_STATUS_SUCCESS;
}

// =============================================================================
// AGING AND MAINTENANCE
// =============================================================================

/**
 * @brief Aging thread function
 */
static THREAD_RETURN_TYPE aging_thread(void *arg) {
    struct fdb_orch_context *ctx = (struct fdb_orch_context*)arg;
    
    sonic_log(LOG_INFO, "FDB aging thread started (interval: %d seconds)", 
               ctx->aging_interval);
    
    while (true) {
        SLEEP_S(ctx->aging_interval);
        
        // Check all entries for aging
        uint64_t current_time = sonic_get_timestamp().seconds;
        uint32_t expired_count = 0;
        
        for (uint16_t i = 0; i < ctx->fdb_table->bucket_count; i++) {
            sonic_hash_entry_t *entry = ctx->fdb_table->buckets[i];
            while (entry) {
                fdb_entry_s *fdb_entry = (fdb_entry_s*)entry->value;
                if (fdb_entry->valid && fdb_entry->type == FDB_ENTRY_TYPE_DYNAMIC) {
                    if ((current_time - fdb_entry->last_seen) > fdb_entry->aging_time) {
                        sonic_log(LOG_DEBUG, "Aging out FDB entry: MAC=%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                                   fdb_entry->mac_address.bytes[0], fdb_entry->mac_address.bytes[1],
                                   fdb_entry->mac_address.bytes[2], fdb_entry->mac_address.bytes[3],
                                   fdb_entry->mac_address.bytes[4], fdb_entry->mac_address.bytes[5]);
                        
                        fdb_remove_entry(&fdb_entry->mac_address);
                        expired_count++;
                        break; // Re-iterate from beginning
                    }
                }
                entry = entry->next;
            }
        }
        
        if (expired_count > 0) {
            sonic_log(LOG_INFO, "Aged out %d FDB entries", expired_count);
        }
    }
    
    return (THREAD_RETURN_TYPE)0;
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

/**
 * @brief Initialize FDB orchestrator
 */
int fdb_orch_init(fdb_orch_t *fdb_orch) {
    // Initialize context
    g_fdb_orch.fdb_table = sonic_hash_create(4096);
    if (!g_fdb_orch.fdb_table) {
        sonic_log(LOG_ERROR, "Failed to create FDB hash table");
        return SONIC_STATUS_NO_MEMORY;
    }
    
    g_fdb_orch.fdb_size = 0;
    g_fdb_orch.max_entries = SONIC_FDB_MAX_ENTRIES;
    g_fdb_orch.aging_interval = SONIC_FDB_AGING_TIME;
    g_fdb_orch.packets_processed = 0;
    g_fdb_orch.fdb_hits = 0;
    g_fdb_orch.fdb_misses = 0;
    g_fdb_orch.bum_packets = 0;
    
    // Start aging thread
    thread_t aging_thread_handle;
    if (!THREAD_CREATE(aging_thread_handle, aging_thread, &g_fdb_orch)) {
        sonic_log(LOG_ERROR, "Failed to create FDB aging thread");
        return SONIC_STATUS_FAILURE;
    }
    
    sonic_log(LOG_INFO, "FDB orchestrator initialized successfully");
    return SONIC_STATUS_SUCCESS;
}

/**
 * @brief Process incoming packet for MAC learning
 */
int fdb_orch_process_packet(fdb_orch_t *fdb_orch, const uint8_t *packet_data, 
                           uint16_t packet_length, uint16_t ingress_port) {
    if (packet_length < sizeof(sonic_ethernet_header_t)) {
        return SONIC_STATUS_INVALID;
    }
    
    sonic_ethernet_header_t *eth_hdr = (sonic_ethernet_header_t*)packet_data;
    uint64_t current_time = sonic_get_timestamp().seconds;
    
    g_fdb_orch.packets_processed++;
    
    // Learn source MAC address
    if (!SONIC_MAC_IS_ZERO(eth_hdr->src_mac) && 
        !SONIC_MAC_IS_BROADCAST(eth_hdr->src_mac) &&
        !SONIC_MAC_IS_MULTICAST(eth_hdr->src_mac)) {
        
        fdb_entry_s new_entry = {
            .mac_address = eth_hdr->src_mac,
            .vlan_id = 1, // Default VLAN
            .port_id = ingress_port,
            .type = FDB_ENTRY_TYPE_DYNAMIC,
            .aging_time = SONIC_FDB_AGING_TIME,
            .last_seen = current_time,
            .valid = true,
            .next = NULL
        };
        
        fdb_add_entry(&new_entry);
    }
    
    // Handle destination MAC (lookup or BUM)
    fdb_entry_s *dst_entry = fdb_find_entry(&eth_hdr->dst_mac);
    if (dst_entry) {
        g_fdb_orch.fdb_hits++;
        sonic_log(LOG_DEBUG, "FDB hit: MAC=%02X:%02X:%02X:%02X:%02X:%02X:%02X -> PORT=%d",
                   eth_hdr->dst_mac.bytes[0], eth_hdr->dst_mac.bytes[1],
                   eth_hdr->dst_mac.bytes[2], eth_hdr->dst_mac.bytes[3],
                   eth_hdr->dst_mac.bytes[4], eth_hdr->dst_mac.bytes[5],
                   dst_entry->port_id);
    } else {
        g_fdb_orch.fdb_misses++;
        handle_bum_traffic(&eth_hdr->dst_mac, 1, ingress_port);
    }
    
    return SONIC_STATUS_SUCCESS;
}

/**
 * @brief Add static FDB entry
 */
int fdb_orch_add_static_entry(fdb_orch_t *fdb_orch, const sonic_mac_t *mac, 
                             uint16_t vlan_id, uint16_t port_id) {
    fdb_entry_s new_entry = {
        .mac_address = *mac,
        .vlan_id = vlan_id,
        .port_id = port_id,
        .type = FDB_ENTRY_TYPE_STATIC,
        .aging_time = 0, // Static entries don't age
        .last_seen = sonic_get_timestamp().seconds,
        .valid = true,
        .next = NULL
    };
    
    return fdb_add_entry(&new_entry);
}

/**
 * @brief Delete FDB entry
 */
int fdb_orch_delete_entry(fdb_orch_t *fdb_orch, const uint8_t *mac_address) {
    sonic_mac_t mac;
    memcpy(mac.bytes, mac_address, 6);
    return fdb_remove_entry(&mac);
}

/**
 * @brief Get FDB statistics
 */
void fdb_orch_get_counters(fdb_orch_t *fdb_orch, sonic_counters_t *counters) {
    counters->fdb_hits = g_fdb_orch.fdb_hits;
    counters->fdb_misses = g_fdb_orch.fdb_misses;
    counters->bum_packets = g_fdb_orch.bum_packets;
}

/**
 * @brief Cleanup FDB orchestrator
 */
void fdb_orch_cleanup(fdb_orch_t *fdb_orch) {
    if (g_fdb_orch.fdb_table) {
        sonic_hash_destroy(g_fdb_orch.fdb_table);
    }
    
    sonic_log(LOG_INFO, "FDB orchestrator cleaned up");
}
