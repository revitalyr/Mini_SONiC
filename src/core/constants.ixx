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

} // namespace MiniSonic::Constants
