module;

#include <cstdint>
#include <cstddef>

export module MiniSonic.Constants;

namespace MiniSonic::Constants {

// =============================================================================
// NETWORK CONSTANTS
// =============================================================================

// Ethernet protocol types
constexpr uint16_t ETH_P_ARP = 0x0806;
constexpr uint16_t ETH_P_IP = 0x0800;
constexpr uint16_t ETH_P_8021Q = 0x8100;
constexpr uint16_t ETH_P_IPV6 = 0x86DD;

// IP protocol types
constexpr uint8_t IP_PROTOCOL_ICMP = 1;
constexpr uint8_t IP_PROTOCOL_TCP = 6;
constexpr uint8_t IP_PROTOCOL_UDP = 17;

// =============================================================================
// BUFFER AND TABLE SIZES
// =============================================================================

constexpr size_t BUFFER_SIZE = 2048;
constexpr size_t MAX_PORTS = 8;
constexpr size_t MAC_TABLE_SIZE = 1024;
constexpr size_t ARP_TABLE_SIZE = 256;
constexpr size_t VLAN_TABLE_SIZE = 64;
constexpr size_t RING_SIZE = 1024;

// =============================================================================
// TIMEOUT CONSTANTS
// =============================================================================

constexpr uint32_t MAC_AGE_TIMEOUT_SEC = 300; // 5 minutes
constexpr uint32_t ARP_TIMEOUT_SEC = 300;

// =============================================================================
// STRING LITERALS
// =============================================================================

constexpr char* const DEFAULT_BIND_ADDRESS = "0.0.0.0";
constexpr char* const DEFAULT_LOOPBACK_ADDRESS = "127.0.0.1";
constexpr char* const PROTOCOL_NAME_GOSSIP = "Gossip";

// =============================================================================
// Gossip PROTOCOL CONSTANTS
// =============================================================================

constexpr uint16_t DEFAULT_GOSSIP_PORT = 7946;
constexpr uint32_t DEFAULT_GOSSIP_INTERVAL_MS = 1000;
constexpr uint32_t DEFAULT_GOSSIP_FANOUT = 3;
constexpr uint32_t DEFAULT_SUSPICION_TIMEOUT_MS = 5000;
constexpr uint32_t DEFAULT_DEAD_TIMEOUT_MS = 30000;
constexpr uint32_t DEFAULT_PING_INTERVAL_MS = 1000;
constexpr uint32_t DEFAULT_PING_TIMEOUT_MS = 500;
constexpr uint32_t DEFAULT_PING_RETRIES = 3;
constexpr size_t DEFAULT_MAX_PEERS = 100;

// =============================================================================
// PIPELINE CONSTANTS
// =============================================================================

constexpr size_t DEFAULT_BATCH_SIZE = 32;
constexpr size_t DEFAULT_SPSC_QUEUE_CAPACITY = 1024;

// =============================================================================
// CACHE AND ALIGNMENT CONSTANTS
// =============================================================================

constexpr size_t CACHE_LINE_SIZE = 64;

} // namespace MiniSonic::Constants
