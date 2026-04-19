export module MiniSonic.Constants;

namespace MiniSonic::Constants {

// =============================================================================
// NETWORK CONSTANTS
// =============================================================================

// Ethernet protocol types
inline constexpr uint16_t ETH_P_ARP = 0x0806;
inline constexpr uint16_t ETH_P_IP = 0x0800;
inline constexpr uint16_t ETH_P_8021Q = 0x8100;
inline constexpr uint16_t ETH_P_IPV6 = 0x86DD;

// IP protocol types
inline constexpr uint8_t IP_PROTOCOL_ICMP = 1;
inline constexpr uint8_t IP_PROTOCOL_TCP = 6;
inline constexpr uint8_t IP_PROTOCOL_UDP = 17;

// =============================================================================
// BUFFER AND TABLE SIZES
// =============================================================================

inline constexpr size_t BUFFER_SIZE = 2048;
inline constexpr size_t MAX_PORTS = 8;
inline constexpr size_t MAC_TABLE_SIZE = 1024;
inline constexpr size_t ARP_TABLE_SIZE = 256;
inline constexpr size_t VLAN_TABLE_SIZE = 64;
inline constexpr size_t RING_SIZE = 1024;

// =============================================================================
// TIMEOUT CONSTANTS
// =============================================================================

inline constexpr uint32_t MAC_AGE_TIMEOUT_SEC = 300; // 5 minutes
inline constexpr uint32_t ARP_TIMEOUT_SEC = 300;

// =============================================================================
// STRING LITERALS
// =============================================================================

inline constexpr const char* DEFAULT_BIND_ADDRESS = "0.0.0.0";
inline constexpr const char* DEFAULT_LOOPBACK_ADDRESS = "127.0.0.1";
inline constexpr const char* PROTOCOL_NAME_GOSSIP = "Gossip";

// =============================================================================
// Gossip PROTOCOL CONSTANTS
// =============================================================================

inline constexpr uint16_t DEFAULT_GOSSIP_PORT = 7946;
inline constexpr uint32_t DEFAULT_GOSSIP_INTERVAL_MS = 1000;
inline constexpr uint32_t DEFAULT_GOSSIP_FANOUT = 3;
inline constexpr uint32_t DEFAULT_SUSPICION_TIMEOUT_MS = 5000;
inline constexpr uint32_t DEFAULT_DEAD_TIMEOUT_MS = 30000;
inline constexpr uint32_t DEFAULT_PING_INTERVAL_MS = 1000;
inline constexpr uint32_t DEFAULT_PING_TIMEOUT_MS = 500;
inline constexpr uint32_t DEFAULT_PING_RETRIES = 3;
inline constexpr size_t DEFAULT_MAX_PEERS = 100;

// =============================================================================
// PIPELINE CONSTANTS
// =============================================================================

inline constexpr size_t DEFAULT_BATCH_SIZE = 32;
inline constexpr size_t DEFAULT_SPSC_QUEUE_CAPACITY = 1024;

// =============================================================================
// CACHE AND ALIGNMENT CONSTANTS
// =============================================================================

inline constexpr size_t CACHE_LINE_SIZE = 64;

} // namespace MiniSonic::Constants
