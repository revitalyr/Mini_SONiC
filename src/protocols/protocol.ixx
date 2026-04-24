module;

#include <memory> // For std::unique_ptr, std::shared_ptr
#include <string> // For std::string
#include <functional> // For std::function
#include <vector> // For std::vector
#include <map> // For std::map
#include <atomic> // For std::atomic
#include <mutex> // For std::mutex
#include <variant> // For std::variant
#include <cstdint> // For uint32_t, uint64_t
#include <ranges> // For C++20 ranges
#include <span> // For C++20 std::span
#include <optional> // For std::optional

export module MiniSonic.Protocol;

import MiniSonic.Core.Utils;
import MiniSonic.Core.Constants;

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::atomic;
using std::mutex;
using std::variant;
using Byte = uint8_t;

export namespace MiniSonic::Protocol {

// =============================================================================
// PROTOCOL MESSAGE TYPES
// =============================================================================

/**
 * @brief Protocol message types
 */
enum class MessageType : uint32_t {
    MSG_UNKNOWN = 0,
    MSG_DATA = 1,
    MSG_CONTROL = 2,
    MSG_DISCOVERY = 3,
    MSG_GOSSIP = 4,
    MSG_HEARTBEAT = 5,
    MSG_ERROR = 6
};

/**
 * @brief Protocol message header
 */
struct MessageHeader {
    MessageType type{MessageType::MSG_UNKNOWN};
    uint32_t length{0};
    Types::SequenceNumber sequence{0};  // semantic alias
    Types::Timestamp timestamp{0};       // semantic alias

    [[nodiscard]] bool isValid() const noexcept {
        return type != MessageType::MSG_UNKNOWN && length > 0;
    }
};

/**
 * @brief Protocol message with header and payload using modern C++23
 */
class Message {
public:
    Message() = default;

    explicit Message(MessageType type, vector<uint8_t> payload)
        : m_header{type, static_cast<uint32_t>(payload.size()), 0, 0}
        , m_payload(std::move(payload)) {}

    [[nodiscard]] const MessageHeader& header() const noexcept { return m_header; }
    [[nodiscard]] const vector<uint8_t>& payload() const noexcept { return m_payload; }
    [[nodiscard]] vector<uint8_t>& payload() noexcept { return m_payload; }

    // C++23: Get payload as span for efficient read-only access
    [[nodiscard]] std::span<const uint8_t> payloadSpan() const noexcept {
        return std::span<const uint8_t>(m_payload);
    }

    // C++23: Get payload as span for efficient read-write access
    [[nodiscard]] std::span<uint8_t> payloadSpan() noexcept {
        return std::span<uint8_t>(m_payload);
    }

    void setType(MessageType type) noexcept { m_header.type = type; }
    void setSequence(uint32_t seq) noexcept { m_header.sequence = seq; }
    void setTimestamp(uint64_t ts) noexcept { m_header.timestamp = ts; }

    [[nodiscard]] bool isValid() const noexcept { return m_header.isValid(); }

private:
    MessageHeader m_header;
    vector<uint8_t> m_payload;
};

// =============================================================================
// PROTOCOL HANDLER INTERFACE
// =============================================================================

/**
 * @brief Abstract protocol handler interface
 * 
 * All protocol implementations (WebSocket, Gossip, etc.) must implement
 * this interface to be pluggable into the protocol stack.
 */
class IProtocolHandler {
public:
    using MessageCallback = function<void(const Message&)>;
    using ErrorCallback = function<void(const string&)>;
    
    virtual ~IProtocolHandler() = default;
    
    // Lifecycle
    virtual void start() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isRunning() const noexcept = 0;
    
    // Messaging
    virtual void send(const Message& msg) = 0;
    virtual void broadcast(const Message& msg) = 0;
    
    // Callbacks
    virtual void setMessageHandler(MessageCallback handler) = 0;
    virtual void setErrorHandler(ErrorCallback handler) = 0;
    
    // Metadata
    [[nodiscard]] virtual string protocolName() const = 0;
    [[nodiscard]] virtual string getStats() const = 0;
};

/**
 * @brief Protocol handler configuration
 */
struct HandlerConfig {
    Types::PortId listen_port{0};  // semantic alias
    string bind_address{Constants::DEFAULT_BIND_ADDRESS};
    size_t max_connections{100};
    Types::BufferLength buffer_size{4096};  // semantic alias
    bool enable_compression{false};
    uint32_t timeout_ms{5000};
};

// =============================================================================
// PROTOCOL STACK
// =============================================================================

/**
 * @brief Protocol stack manager
 * 
 * Manages multiple protocol handlers and provides unified interface.
 */
class ProtocolStack {
public:
    ProtocolStack() = default;
    ~ProtocolStack();
    
    // Disable copy
    ProtocolStack(const ProtocolStack&) = delete;
    ProtocolStack& operator=(const ProtocolStack&) = delete;
    
    /**
     * @brief Register a protocol handler
     */
    void registerHandler(const string& name, unique_ptr<IProtocolHandler> handler);
    
    /**
     * @brief Unregister a protocol handler
     */
    void unregisterHandler(const string& name);
    
    /**
     * @brief Get a registered handler by name
     */
    [[nodiscard]] IProtocolHandler* getHandler(const string& name) const;
    
    /**
     * @brief Start all registered handlers
     */
    void startAll();
    
    /**
     * @brief Stop all registered handlers
     */
    void stopAll();
    
    /**
     * @brief Send message through specific protocol
     */
    void send(const string& protocol_name, const Message& msg);
    
    /**
     * @brief Broadcast message through all protocols
     */
    void broadcast(const Message& msg);
    
    /**
     * @brief Set global message handler
     */
    void setGlobalMessageHandler(function<void(const string&, const Message&)> handler);
    
    /**
     * @brief Get statistics from all handlers
     */
    [[nodiscard]] string getAllStats() const;

private:
    map<string, unique_ptr<IProtocolHandler>> m_handlers;
    mutable mutex m_mutex;
    function<void(const string&, const Message&)> m_global_handler;
};

// =============================================================================
// TRANSPORT ABSTRACTION
// =============================================================================

/**
 * @brief Transport layer abstraction
 */
class ITransport {
public:
    using ReceiveCallback = function<void(vector<uint8_t>)>;
    using ErrorCallback = function<void(const string&)>;

    virtual ~ITransport() = default;

    virtual void connect(const string& address, Types::PortId port) = 0;
    virtual void listen(Types::PortId port) = 0;
    virtual void disconnect() = 0;

    virtual void send(const vector<uint8_t>& data) = 0;

    virtual void setReceiveHandler(ReceiveCallback handler) = 0;
    virtual void setErrorHandler(ErrorCallback handler) = 0;

    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    [[nodiscard]] virtual string transportInfo() const = 0;
};

} // export namespace MiniSonic::Protocol
