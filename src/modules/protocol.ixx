module;

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <variant>
#include <cstdint>

export module MiniSonic.Protocol;

import MiniSonic.Utils;

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::atomic;
using std::mutex;
using std::variant;
using std::byte;

export namespace MiniSonic::Protocol {

// =============================================================================
// PROTOCOL MESSAGE TYPES
// =============================================================================

/**
 * @brief Protocol message types
 */
enum class MessageType : uint32_t {
    UNKNOWN = 0,
    DATA = 1,
    CONTROL = 2,
    DISCOVERY = 3,
    GOSSIP = 4,
    HEARTBEAT = 5,
    ERROR = 6
};

/**
 * @brief Protocol message header
 */
struct MessageHeader {
    MessageType type{MessageType::UNKNOWN};
    uint32_t length{0};
    uint32_t sequence{0};
    uint64_t timestamp{0};
    
    [[nodiscard]] bool isValid() const noexcept {
        return type != MessageType::UNKNOWN && length > 0;
    }
};

/**
 * @brief Protocol message with header and payload
 */
class Message {
public:
    Message() = default;
    
    explicit Message(MessageType type, vector<byte> payload)
        : m_header{type, static_cast<uint32_t>(payload.size()), 0, 0}
        , m_payload(std::move(payload)) {}
    
    [[nodiscard]] const MessageHeader& header() const noexcept { return m_header; }
    [[nodiscard]] const vector<byte>& payload() const noexcept { return m_payload; }
    [[nodiscard]] vector<byte>& payload() noexcept { return m_payload; }
    
    void setType(MessageType type) noexcept { m_header.type = type; }
    void setSequence(uint32_t seq) noexcept { m_header.sequence = seq; }
    void setTimestamp(uint64_t ts) noexcept { m_header.timestamp = ts; }
    
    [[nodiscard]] bool isValid() const noexcept { return m_header.isValid(); }

private:
    MessageHeader m_header;
    vector<byte> m_payload;
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
    Types::Port listen_port{0};
    string bind_address{"0.0.0.0"};
    size_t max_connections{100};
    size_t buffer_size{4096};
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
    using ReceiveCallback = function<void(vector<byte>)>;
    using ErrorCallback = function<void(const string&)>;
    
    virtual ~ITransport() = default;
    
    virtual void connect(const string& address, Types::Port port) = 0;
    virtual void listen(Types::Port port) = 0;
    virtual void disconnect() = 0;
    
    virtual void send(const vector<byte>& data) = 0;
    
    virtual void setReceiveHandler(ReceiveCallback handler) = 0;
    virtual void setErrorHandler(ErrorCallback handler) = 0;
    
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;
    [[nodiscard]] virtual string transportInfo() const = 0;
};

} // export namespace MiniSonic::Protocol
