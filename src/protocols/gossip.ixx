module;

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <random>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

export module MiniSonic.Gossip;

import MiniSonic.Protocol;
import MiniSonic.Utils;

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::unordered_map;
using std::set;
using std::atomic;
using std::mutex;
using std::thread;
using std::condition_variable;

export namespace MiniSonic::Gossip {

// =============================================================================
// GOSSIP PEER INFORMATION
// =============================================================================

/**
 * @brief Peer state in gossip protocol
 */
enum class PeerState {
    ALIVE,
    SUSPECTED,
    DEAD
};

/**
 * @brief Represents a peer in the gossip network
 */
struct PeerInfo {
    string peer_id;
    string address;
    Types::Port port;
    PeerState state{PeerState::ALIVE};
    uint64_t heartbeat{0};
    uint64_t last_seen{0};
    uint64_t incarnation{0};
    
    [[nodiscard]] bool isAlive() const noexcept {
        return state == PeerState::ALIVE;
    }
    
    [[nodiscard]] bool isSuspected() const noexcept {
        return state == PeerState::SUSPECTED;
    }
    
    [[nodiscard]] bool isDead() const noexcept {
        return state == PeerState::DEAD;
    }
};

// =============================================================================
// GOSSIP MESSAGE TYPES
// =============================================================================

/**
 * @brief Gossip-specific message types
 */
enum class GossipMessageType : uint32_t {
    PING = 1,
    PONG = 2,
    GOSSIP_DIGEST = 3,
    GOSSIP_ACK = 4,
    MEMBER_LIST = 5,
    JOIN = 6,
    LEAVE = 7
};

/**
 * @brief Gossip message payload
 */
struct GossipPayload {
    GossipMessageType type{GossipMessageType::PING};
    string sender_id;
    uint64_t incarnation{0};
    vector<string> peer_list;  // For member list propagation
    map<string, uint64_t> heartbeats;  // Peer heartbeat info
};

// =============================================================================
// GOSSIP CONFIGURATION
// =============================================================================

/**
 * @brief Gossip protocol configuration
 */
struct GossipConfig {
    Types::Port listen_port{7946};  // Default gossip port
    string bind_address{"0.0.0.0"};
    
    // Gossip parameters
    uint32_t gossip_interval_ms{1000};  // Gossip every 1 second
    uint32_t gossip_fanout{3};  // Send to 3 random peers
    uint32_t suspicion_timeout_ms{5000};  // 5 seconds to suspect
    uint32_t dead_timeout_ms{30000};  // 30 seconds to declare dead
    
    // Failure detection
    uint32_t ping_interval_ms{1000};
    uint32_t ping_timeout_ms{500};
    uint32_t ping_retries{3};
    
    // Membership
    size_t max_peers{100};
    bool enable_auto_discovery{true};
};

// =============================================================================
// GOSSIP PROTOCOL (PROTOCOL HANDLER IMPLEMENTATION)
// =============================================================================

/**
 * @brief P2P Gossip protocol implementing IProtocolHandler
 * 
 * Implements a lightweight gossip protocol for peer discovery,
 * membership management, and failure detection in distributed systems.
 * Simpler than libp2p but provides core gossip functionality.
 */
class GossipProtocol : public Protocol::IProtocolHandler {
public:
    explicit GossipProtocol(string peer_id, const GossipConfig& config);
    ~GossipProtocol() override;
    
    // IProtocolHandler interface
    void start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const noexcept override;
    
    void send(const Protocol::Message& msg) override;
    void broadcast(const Protocol::Message& msg) override;
    
    void setMessageHandler(MessageCallback handler) override;
    void setErrorHandler(ErrorCallback handler) override;
    
    [[nodiscard]] string protocolName() const override { return "Gossip"; }
    [[nodiscard]] string getStats() const override;
    
    // Gossip-specific methods
    void joinPeer(const string& address, Types::Port port);
    void leaveCluster();
    
    [[nodiscard]] string peerId() const noexcept { return m_peer_id; }
    [[nodiscard]] vector<PeerInfo> getPeers() const;
    [[nodiscard]] size_t peerCount() const;
    
    // Membership callbacks
    using PeerJoinCallback = function<void(const PeerInfo&)>;
    using PeerLeaveCallback = function<void(const PeerInfo&)>;
    void setPeerJoinCallback(PeerJoinCallback callback);
    void setPeerLeaveCallback(PeerLeaveCallback callback);

private:
    // Gossip loops
    void gossipLoop();
    void failureDetectionLoop();
    
    // Message handling
    void handleGossipMessage(const GossipPayload& payload, const string& source);
    void handlePing(const GossipPayload& payload, const string& source);
    void handlePong(const GossipPayload& payload, const string& source);
    void handleMemberList(const GossipPayload& payload, const string& source);
    
    // Peer management
    void addPeer(const PeerInfo& peer);
    void removePeer(const string& peer_id);
    void updatePeerHeartbeat(const string& peer_id);
    void selectRandomPeers(size_t count, vector<string>& out_peers);
    
    // Serialization
    vector<uint8_t> serializeGossipPayload(const GossipPayload& payload);
    GossipPayload deserializeGossipPayload(const vector<uint8_t>& data);
    
    // Network
    void sendToPeer(const string& peer_id, const GossipPayload& payload);
    void broadcastToRandomPeers(const GossipPayload& payload);
    
    // Configuration
    string m_peer_id;
    GossipConfig m_config;
    
    // Server state
    atomic<bool> m_running{false};
    thread m_gossip_thread;
    thread m_failure_detection_thread;
    SOCKET m_server_socket{INVALID_SOCKET};
    
    // Membership
    unordered_map<string, PeerInfo> m_peers;
    mutable mutex m_peers_mutex;
    
    // Callbacks
    MessageCallback m_message_handler;
    ErrorCallback m_error_handler;
    PeerJoinCallback m_peer_join_callback;
    PeerLeaveCallback m_peer_leave_callback;
    
    // Statistics
    atomic<uint64_t> m_messages_sent{0};
    atomic<uint64_t> m_messages_received{0};
    atomic<uint64_t> m_gossip_rounds{0};
    atomic<uint64_t> m_peer_joins{0};
    atomic<uint64_t> m_peer_leaves{0};
    
    // Local state
    atomic<uint64_t> m_incarnation{0};
    atomic<uint64_t> m_heartbeat{0};
    
    // Random number generator
    std::mt19937 m_rng;
};

// =============================================================================
// GOSSIP NODE (HIGH-LEVEL INTERFACE)
// =============================================================================

/**
 * @brief High-level gossip node for easy integration
 */
class GossipNode {
public:
    explicit GossipNode(const string& peer_id, const GossipConfig& config = GossipConfig{});
    ~GossipNode();
    
    void start();
    void stop();
    
    void join(const string& address, Types::Port port);
    void leave();
    
    void broadcast(const vector<uint8_t>& data);
    void setDataHandler(function<void(const string&, const vector<uint8_t>&)> handler);
    
    [[nodiscard]] vector<PeerInfo> members() const;
    [[nodiscard]] string nodeId() const { return m_peer_id; }
    
    [[nodiscard]] string stats() const;

private:
    string m_peer_id;
    unique_ptr<GossipProtocol> m_protocol;
    function<void(const string&, const vector<uint8_t>&)> m_data_handler;
};

// =============================================================================
// GOSSIP FACTORY
// =============================================================================

/**
 * @brief Factory for creating gossip components
 */
class GossipFactory {
public:
    static unique_ptr<GossipProtocol> createProtocol(const string& peer_id, 
                                                     const GossipConfig& config = GossipConfig{});
    
    static unique_ptr<GossipNode> createNode(const string& peer_id,
                                             const GossipConfig& config = GossipConfig{});
    
    static GossipConfig defaultConfig();
    static string generatePeerId();
};

} // export namespace MiniSonic::Gossip
