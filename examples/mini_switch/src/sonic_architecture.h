/**
 * @file sonic_architecture.h
 * @brief SONiC Architecture Definitions and Data Structures
 * 
 * This file defines the core architecture components of SONiC (Software for Open Networking in Cloud)
 * including Redis-based communication, SAI abstraction, and hardware interface definitions.
 * 
 * SONiC Architecture Overview:
 * - Control Plane: OrchAgent, syncd, Redis databases
 * - Data Plane: ASIC hardware via SAI interface
 * - Communication: Redis pub/sub and hash tables
 * - Abstraction: SAI (Switch Abstraction Interface)
 */

#ifndef SONIC_ARCHITECTURE_H
#define SONIC_ARCHITECTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "cross_platform.h"
#include "sonic_types.h"

// =============================================================================
// SONiC DATABASE DEFINITIONS
// =============================================================================

/**
 * @brief Redis Database Types in SONiC
 * 
 * SONiC uses multiple Redis databases for different purposes:
 * - APP_DB: Application database for configuration and state
 * - ASIC_DB: Hardware ASIC state database
 * - CONFIG_DB: Configuration database
 * - COUNTERS_DB: Performance counters
 * - STATE_DB: System state information
 */
typedef enum {
    REDIS_DB_APP = 0,        ///< Application database (FDB, ROUTES, etc.)
    REDIS_DB_ASIC = 1,       ///< ASIC hardware state database
    REDIS_DB_CONFIG = 2,      ///< Configuration database
    REDIS_DB_COUNTERS = 3,    ///< Performance counters database
    REDIS_DB_STATE = 4,       ///< System state database
    REDIS_DB_MAX = 5
} redis_database_t;

// =============================================================================
// SAI (SWITCH ABSTRACTION INTERFACE) DEFINITIONS
// =============================================================================

/**
 * @brief SAI Object Types
 * 
 * SAI provides unified abstraction for different ASIC vendors
 */
typedef enum {
    SAI_OBJECT_TYPE_SWITCH = 0,      ///< Switch object
    SAI_OBJECT_TYPE_PORT = 1,         ///< Physical port
    SAI_OBJECT_TYPE_FDB = 2,         ///< Forwarding Database (MAC table)
    SAI_OBJECT_TYPE_ROUTE = 3,        ///< Route entry
    SAI_OBJECT_TYPE_NEIGHBOR = 4,     ///< ARP/ND neighbor entry
    SAI_OBJECT_TYPE_VLAN = 5,         ///< VLAN interface
    SAI_OBJECT_TYPE_ACL = 6,          ///< Access Control List
    SAI_OBJECT_TYPE_MAX = 7
} sai_object_type_t;

/**
 * @brief SAI Attributes for different object types
 */
typedef enum {
    SAI_SWITCH_ATTR_INIT_SWITCH = 0,
    SAI_PORT_ATTR_ADMIN_STATE = 1,
    SAI_FDB_ATTR_MAC_ADDRESS = 2,
    SAI_FDB_ATTR_TYPE = 3,
    SAI_ROUTE_ATTR_DESTINATION = 4,
    SAI_ROUTE_ATTR_NEXT_HOP = 5,
    SAI_NEIGHBOR_ATTR_IP_ADDRESS = 6,
    SAI_NEIGHBOR_ATTR_MAC_ADDRESS = 7,
    SAI_VLAN_ATTR_VLAN_ID = 8,
    SAI_ACL_ATTR_PRIORITY = 9,
    SAI_ATTR_MAX = 10
} sai_attribute_t;

// =============================================================================
// L2 FORWARDING DATABASE (FDB) DEFINITIONS
// =============================================================================

/**
 * @brief FDB Entry Types
 */
typedef enum {
    FDB_ENTRY_TYPE_STATIC = 0,    ///< Static MAC entry
    FDB_ENTRY_TYPE_DYNAMIC = 1,    ///< Dynamic learned MAC entry
    FDB_ENTRY_TYPE_REMOTE = 2,     ///< Remote MAC (for distributed systems)
} fdb_entry_type_t;

/**
 * @brief FDB Entry Structure
 * 
 * Represents a MAC address entry in the forwarding database
 */
typedef struct {
    uint8_t mac_address[6];        ///< MAC address (6 bytes)
    uint16_t vlan_id;             ///< VLAN ID
    uint16_t port_id;              ///< Egress port
    fdb_entry_type_t type;         ///< Entry type (static/dynamic)
    uint32_t aging_time;           ///< Aging timer in seconds
    uint64_t last_seen;            ///< Timestamp of last packet
    bool valid;                   ///< Entry validity flag
} fdb_entry_t;

/**
 * @brief BUM Traffic Types (Broadcast, Unknown Unicast, Multicast)
 */
typedef enum {
    BUM_TYPE_BROADCAST = 0,       ///< Broadcast traffic (FF:FF:FF:FF:FF:FF)
    BUM_TYPE_UNKNOWN_UNICAST = 1,  ///< Unknown unicast (MAC not in FDB)
    BUM_TYPE_MULTICAST = 2,      ///< Multicast traffic (01:00:5E:xx:xx:xx)
    BUM_TYPE_MAX = 3
} bum_traffic_type_t;

// =============================================================================
// L3 ROUTING (FIB) DEFINITIONS
// =============================================================================

/**
 * @brief Route Entry Types
 */
typedef enum {
    ROUTE_TYPE_DIRECT = 0,        ///< Directly connected route
    ROUTE_TYPE_STATIC = 1,        ///< Static route
    ROUTE_TYPE_DYNAMIC = 2,       ///< Dynamic route (BGP/OSPF)
    ROUTE_TYPE_LOCAL = 3,         ///< Local route
    ROUTE_TYPE_MAX = 4
} route_type_t;

/**
 * @brief Route Entry Structure
 * 
 * Represents an IP route in the Forwarding Information Base
 */
typedef struct {
    uint32_t prefix;              ///< IP prefix (network address)
    uint8_t prefix_len;           ///< Prefix length (subnet mask)
    uint32_t next_hop;            ///< Next hop IP address
    uint16_t port_id;             ///< Egress port
    route_type_t type;            ///< Route type
    uint32_t metric;              ///< Route metric/cost
    uint32_t admin_distance;       ///< Administrative distance
    bool active;                  ///< Route active flag
} route_entry_t;

/**
 * @brief ECMP (Equal-Cost Multi-Path) Group
 */
typedef struct {
    uint32_t group_id;            ///< ECMP group identifier
    uint16_t num_paths;           ///< Number of equal-cost paths
    uint16_t port_ids[16];        ///< Array of egress ports
    uint32_t hash_seed;           ///< Hash seed for distribution
} ecmp_group_t;

// =============================================================================
// ARP/NEIGHBOR DISCOVERY DEFINITIONS
// =============================================================================

/**
 * @brief Neighbor Entry Types
 */
typedef enum {
    NEIGH_TYPE_ARP = 0,          ///< ARP neighbor (IPv4)
    NEIGH_TYPE_NDP = 1,          ///< NDP neighbor (IPv6)
    NEIGH_TYPE_STATIC = 2,        ///< Static neighbor entry
    NEIGH_TYPE_MAX = 3
} neighbor_type_t;

/**
 * @brief Neighbor Entry Structure
 * 
 * Represents an ARP/NDP neighbor entry
 */
typedef struct {
    uint32_t ip_address;           ///< IP address
    uint8_t mac_address[6];        ///< MAC address
    neighbor_type_t type;          ///< Neighbor type (ARP/NDP)
    uint16_t port_id;              ///< Associated port
    uint32_t aging_time;           ///< Aging timer
    bool reachable;                ///< Reachability status
    uint64_t last_request;         ///< Last ARP request timestamp
    uint64_t last_reply;           ///< Last ARP reply timestamp
} neighbor_entry_t;

// =============================================================================
// PLACEHOLDER TYPES (for compilation without external dependencies)
// =============================================================================

typedef void* redis_context_t;   ///< Redis connection context placeholder
typedef void* sai_api_t;         ///< SAI API interface placeholder
typedef int sai_status_t;        ///< SAI status code placeholder

// =============================================================================
// SONiC COMPONENT INTERFACES
// =============================================================================

/**
 * @brief OrchAgent Interface
 * 
 * OrchAgent is the main orchestration daemon in SONiC
 * It subscribes to Redis changes and translates them to SAI calls
 */
typedef struct {
    thread_t thread;              ///< OrchAgent thread
    redis_context_t *redis_ctx;    ///< Redis connection context
    sai_api_t *sai_api;          ///< SAI API interface
    bool running;                  ///< Running status
} orchagent_t;

/**
 * @brief syncd Interface
 * 
 * syncd synchronizes hardware state with Redis databases
 * It handles bidirectional sync between ASIC and software
 */
typedef struct {
    thread_t thread;              ///< syncd thread
    redis_context_t *asic_db_ctx;   ///< ASIC DB connection
    sai_api_t *sai_api;          ///< SAI API interface
    bool running;                  ///< Running status
} syncd_t;

/**
 * @brief FDB Orchestrator (FdbOrch)
 * 
 * Manages MAC learning and FDB operations
 */
typedef struct {
    orchagent_t *parent;           ///< Parent OrchAgent
    redis_context_t *app_db_ctx;   ///< APP_DB connection
    fdb_entry_t fdb_table[4096];  ///< FDB table (4K entries)
    uint16_t fdb_size;             ///< Current FDB size
    mutex_t fdb_lock;       ///< FDB access mutex
} fdb_orch_t;

/**
 * @brief Route Orchestrator (RouteOrch)
 * 
 * Manages IP routing and FIB operations
 */
typedef struct {
    orchagent_t *parent;           ///< Parent OrchAgent
    redis_context_t *app_db_ctx;   ///< APP_DB connection
    route_entry_t route_table[1024]; ///< Route table (1K entries)
    uint16_t route_size;           ///< Current route table size
    mutex_t route_lock;     ///< Route table mutex
} route_orch_t;

// =============================================================================
// PACKET PROCESSING PIPELINE
// =============================================================================

/**
 * @brief Packet Processing Stages in SONiC
 */
typedef enum {
    PACKET_STAGE_INGRESS = 0,       ///< Packet ingress (input)
    PACKET_STAGE_PARSING = 1,       ///< Header parsing
    PACKET_STAGE_LOOKUP = 2,         ///< FDB/Route lookup
    PACKET_STAGE_DECISION = 3,       ///< Forwarding decision
    PACKET_STAGE_EGRESS = 4,         ///< Packet egress (output)
    PACKET_STAGE_MAX = 5
} packet_stage_t;

/**
 * @brief Packet Processing Context
 */
typedef struct {
    uint8_t *data;                ///< Packet data
    uint16_t length;               ///< Packet length
    uint16_t ingress_port;         ///< Input port
    uint16_t egress_port;          ///< Output port
    packet_stage_t current_stage;   ///< Current processing stage
    fdb_entry_t *fdb_hit;         ///< FDB lookup result
    route_entry_t *route_hit;       ///< Route lookup result
    bool processed;                ///< Processing complete flag
} packet_context_t;

// =============================================================================
// PERFORMANCE COUNTERS
// =============================================================================

/**
 * @brief SONiC Performance Counters
 */
typedef struct {
    uint64_t packets_received;       ///< Total packets received
    uint64_t packets_sent;           ///< Total packets sent
    uint64_t packets_dropped;       ///< Total packets dropped
    uint64_t bytes_received;        ///< Total bytes received
    uint64_t bytes_sent;            ///< Total bytes sent
    uint64_t fdb_hits;             ///< FDB lookup hits
    uint64_t fdb_misses;           ///< FDB lookup misses
    uint64_t route_hits;           ///< Route lookup hits
    uint64_t route_misses;         ///< Route lookup misses
    uint64_t arp_requests;          ///< ARP requests sent
    uint64_t arp_replies;           ///< ARP replies received
    uint64_t bum_packets;           ///< BUM (Broadcast, Unknown Unicast, Multicast) packets
} sonic_counters_t;

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

/**
 * @brief SONiC Configuration Constants
 */
#define SONIC_FDB_MAX_ENTRIES        4096    ///< Maximum FDB entries
#define SONIC_ROUTE_MAX_ENTRIES      1024    ///< Maximum route entries
#define SONIC_NEIGHBOR_MAX_ENTRIES  256     ///< Maximum neighbor entries
#define SONIC_FDB_AGING_TIME       600      ///< FDB aging time (seconds)
#define SONIC_NEIGHBOR_AGING_TIME  300      ///< Neighbor aging time (seconds)
#define SONIC_MAX_PORTS             128      ///< Maximum number of ports
#define SONIC_MAX_VLANS            4094     ///< Maximum VLANs

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

// SAI Interface Functions
sai_status_t sai_create_route_entry(const route_entry_t *route);
sai_status_t sai_create_fdb_entry(const fdb_entry_t *fdb);
sai_status_t sai_create_neighbor_entry(const neighbor_entry_t *neighbor);
sai_status_t sai_delete_route_entry(uint32_t prefix, uint8_t prefix_len);
sai_status_t sai_delete_fdb_entry(const uint8_t *mac_address);
sai_status_t sai_delete_neighbor_entry(uint32_t ip_address);

// OrchAgent Functions
int orchagent_init(orchagent_t *orch);
int orchagent_run(orchagent_t *orch);
int orchagent_stop(orchagent_t *orch);
int orchagent_subscribe_to_db(orchagent_t *orch, redis_database_t db);

// syncd Functions
int syncd_init(syncd_t *sync);
int syncd_run(syncd_t *sync);
int syncd_stop(syncd_t *sync);
int syncd_sync_asic_to_redis(syncd_t *sync);

// FDB Orchestrator Functions
int fdb_orch_init(fdb_orch_t *fdb_orch);
int fdb_orch_add_entry(fdb_orch_t *fdb_orch, const fdb_entry_t *entry);
int fdb_orch_delete_entry(fdb_orch_t *fdb_orch, const uint8_t *mac_address);
int fdb_orch_lookup_entry(fdb_orch_t *fdb_orch, const uint8_t *mac_address, fdb_entry_t *result);

// Route Orchestrator Functions
int route_orch_init(route_orch_t *route_orch);
int route_orch_add_route(route_orch_t *route_orch, const route_entry_t *route);
int route_orch_delete_route(route_orch_t *route_orch, uint32_t prefix, uint8_t prefix_len);
int route_orch_lookup_route(route_orch_t *route_orch, uint32_t ip_address, route_entry_t *result);

// Packet Processing Functions
int packet_process_ingress(packet_context_t *ctx);
int packet_process_parsing(packet_context_t *ctx);
int packet_process_lookup(packet_context_t *ctx);
int packet_process_decision(packet_context_t *ctx);
int packet_process_egress(packet_context_t *ctx);

// Utility Functions
void sonic_log(sonic_log_level_t level, const char *format, ...);
uint32_t sonic_crc32(const uint8_t *data, uint16_t length);
uint16_t sonic_calculate_checksum(const void *data, uint16_t length);

#endif // SONIC_ARCHITECTURE_H
