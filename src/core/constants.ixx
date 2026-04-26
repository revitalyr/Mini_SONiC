module;

#include <cstdint>
#include <string>
#include <cstddef>

export module MiniSonic.Core.Constants;

namespace MiniSonic::Constants {

// =============================================================================
// NETWORK CONSTANTS
// =============================================================================

// Ethernet protocol types
export constexpr uint16_t ETHERTYPE_ARP = 0x0806;
export constexpr uint16_t ETHERTYPE_IP = 0x0800;
export constexpr uint16_t ETHERTYPE_8021Q = 0x8100;
export constexpr uint16_t ETHERTYPE_IPV6 = 0x86DD;

// IP protocol types
export constexpr uint8_t IP_PROTOCOL_ICMP = 1;
export constexpr uint8_t IP_PROTOCOL_TCP = 6;
export constexpr uint8_t IP_PROTOCOL_UDP = 17;

// =============================================================================
// BUFFER AND TABLE SIZES
// =============================================================================

export constexpr size_t BUFFER_SIZE = 2048;
export constexpr size_t MAX_PORTS = 8;
export constexpr size_t MAC_TABLE_SIZE = 1024;
export constexpr size_t ARP_TABLE_SIZE = 256;
export constexpr size_t VLAN_TABLE_SIZE = 64;
export constexpr size_t RING_SIZE = 1024;

// =============================================================================
// TIMEOUT CONSTANTS
// =============================================================================

export constexpr uint32_t MAC_AGE_TIMEOUT_SEC = 300; // 5 minutes
export constexpr uint32_t ARP_TIMEOUT_SEC = 300;

// =============================================================================
// STRING LITERALS
// =============================================================================

export constexpr const char* const DEFAULT_BIND_ADDRESS = "0.0.0.0";
export constexpr const char* const DEFAULT_LOOPBACK_ADDRESS = "127.0.0.1";
export constexpr const char* const PROTOCOL_NAME_GOSSIP = "Gossip";

// =============================================================================
// Gossip PROTOCOL CONSTANTS
// =============================================================================

export constexpr uint16_t DEFAULT_GOSSIP_PORT = 7946;
export constexpr uint32_t DEFAULT_GOSSIP_INTERVAL_MS = 1000;
export constexpr uint32_t DEFAULT_GOSSIP_FANOUT = 3;
export constexpr uint32_t DEFAULT_SUSPICION_TIMEOUT_MS = 5000;
export constexpr uint32_t DEFAULT_DEAD_TIMEOUT_MS = 30000;
export constexpr uint32_t DEFAULT_PING_INTERVAL_MS = 1000;
export constexpr uint32_t DEFAULT_PING_TIMEOUT_MS = 500;
export constexpr uint32_t DEFAULT_PING_RETRIES = 3;
export constexpr size_t DEFAULT_MAX_PEERS = 100;

// =============================================================================
// PIPELINE CONSTANTS
// =============================================================================

export constexpr size_t DEFAULT_BATCH_SIZE = 32;
export constexpr size_t DEFAULT_SPSC_QUEUE_CAPACITY = 1024;

// =============================================================================
// CACHE AND ALIGNMENT CONSTANTS
// =============================================================================

export constexpr size_t CACHE_LINE_SIZE = 64;

// =============================================================================
// APPLICATION CONSTANTS
// =============================================================================

export constexpr const char* const TOPOLOGY_CONFIG_PATH = "examples/config/topology.json";

export constexpr const char* const DEFAULT_HOST_H1_ID = "H1";
export constexpr const char* const DEFAULT_HOST_H1_NAME = "Host 1";
export constexpr const char* const DEFAULT_HOST_H1_MAC = "00:11:22:33:44:55";
export constexpr const char* const DEFAULT_HOST_H1_IP = "10.0.1.2";
export constexpr int DEFAULT_HOST_H1_X = 100;
export constexpr int DEFAULT_HOST_H1_Y = 300;
export constexpr const char* const DEFAULT_HOST_H1_SWITCH = "SW1";
export constexpr int DEFAULT_HOST_H1_PORT = 1;

export constexpr const char* const DEFAULT_HOST_H2_ID = "H2";
export constexpr const char* const DEFAULT_HOST_H2_NAME = "Host 2";
export constexpr const char* const DEFAULT_HOST_H2_MAC = "00:11:22:33:44:56";
export constexpr const char* const DEFAULT_HOST_H2_IP = "10.0.3.7";
export constexpr int DEFAULT_HOST_H2_X = 700;
export constexpr int DEFAULT_HOST_H2_Y = 300;
export constexpr const char* const DEFAULT_HOST_H2_SWITCH = "SW4";
export constexpr int DEFAULT_HOST_H2_PORT = 1;

export constexpr const char* const EVENT_PACKET_HOP = "packet_hop";
export constexpr const char* const EVENT_PACKET_GENERATED = "packet_generated";

export constexpr const char* const LOG_PREFIX_APP = "[APP]";
export constexpr const char* const LOG_PREFIX_SAI = "[SAI]";
export constexpr const char* const LOG_PREFIX_L2 = "[L2]";
export constexpr const char* const LOG_PREFIX_L3 = "[L3]";

export constexpr const char* const ERR_TOPOLOGY_CONFIG_OPEN = "Failed to open topology config, using defaults";
export constexpr const char* const ERR_TOPOLOGY_CONFIG_PARSE = "Failed to parse topology config";

export constexpr const char* const INFO_INITIALIZED = "Initialized";
export constexpr const char* const INFO_RUNNING = "Running";
export constexpr const char* const INFO_LOADED_TOPOLOGY = "Loaded topology";
export constexpr const char* const INFO_FATAL_ERROR = "Fatal error";

export constexpr const char* const SAI_SWITCH_NAME = "SWITCH0";
export constexpr int SAI_DEFAULT_PORTS = 24;

export constexpr int PACKET_GENERATION_INTERVAL_MS = 50;

export constexpr const char* const METRIC_PACKETS_GENERATED = "app_packets_generated";
export constexpr const char* const METRIC_GENERATOR_CYCLES = "app_generator_cycles";

} // namespace MiniSonic::Constants
