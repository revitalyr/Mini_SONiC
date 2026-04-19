module;

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdint>
#include <cstring>

// WebSocket++ header-only library (to be added to dependencies)
// For now, we'll create a simple native implementation
#ifdef USE_WEBSOCKETPP
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#endif

export module MiniSonic.WebSocket;

import MiniSonic.Protocol;
import MiniSonic.Utils;

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::atomic;
using std::mutex;
using std::thread;
using std::condition_variable;

export namespace MiniSonic::WebSocket {

// =============================================================================
// WEBSOCKET CONNECTION
// =============================================================================

/**
 * @brief WebSocket connection state
 */
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CLOSING
};

/**
 * @brief Represents a WebSocket client connection
 */
class WebSocketConnection {
public:
    explicit WebSocketConnection(string connection_id);
    ~WebSocketConnection() = default;
    
    [[nodiscard]] string connectionId() const noexcept { return m_connection_id; }
    [[nodiscard]] ConnectionState state() const noexcept { return m_state; }
    [[nodiscard]] string remoteAddress() const noexcept { return m_remote_address; }
    
    void setState(ConnectionState state) noexcept { m_state = state; }
    void setRemoteAddress(string address) { m_remote_address = std::move(address); }
    
    [[nodiscard]] bool isActive() const noexcept {
        return m_state == ConnectionState::CONNECTED;
    }

private:
    string m_connection_id;
    ConnectionState m_state{ConnectionState::DISCONNECTED};
    string m_remote_address;
    atomic<uint64_t> m_bytes_sent{0};
    atomic<uint64_t> m_bytes_received{0};
};

// =============================================================================
// WEBSOCKET GATEWAY (PROTOCOL HANDLER IMPLEMENTATION)
// =============================================================================

/**
 * @brief Native C++ WebSocket gateway implementing IProtocolHandler
 * 
 * Provides real-time bidirectional communication using WebSocket protocol.
 * Replaces the Python WebSocket demo with a native C++ implementation.
 */
class WebSocketGateway : public Protocol::IProtocolHandler {
public:
    explicit WebSocketGateway(const Protocol::HandlerConfig& config);
    ~WebSocketGateway() override;
    
    // IProtocolHandler interface
    void start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const noexcept override;
    
    void send(const Protocol::Message& msg) override;
    void broadcast(const Protocol::Message& msg) override;
    
    void setMessageHandler(MessageCallback handler) override;
    void setErrorHandler(ErrorCallback handler) override;
    
    [[nodiscard]] string protocolName() const override { return "WebSocket"; }
    [[nodiscard]] string getStats() const override;
    
    // WebSocket-specific methods
    void sendToConnection(const string& connection_id, const Protocol::Message& msg);
    void closeConnection(const string& connection_id);
    [[nodiscard]] size_t activeConnections() const;

private:
    // Server loop
    void serverLoop();
    void acceptLoop();
    
    // Message processing
    void processIncomingMessage(const string& connection_id, const vector<uint8_t>& data);
    vector<uint8_t> serializeMessage(const Protocol::Message& msg);
    Protocol::Message deserializeMessage(const vector<uint8_t>& data);
    
    // Connection management
    string generateConnectionId();
    void removeConnection(const string& connection_id);
    
    // Configuration
    Protocol::HandlerConfig m_config;
    
    // Server state
    atomic<bool> m_running{false};
    thread m_server_thread;
    thread m_accept_thread;
    
    // Connections
    map<string, shared_ptr<WebSocketConnection>> m_connections;
    mutable mutex m_connections_mutex;
    
    // Callbacks
    MessageCallback m_message_handler;
    ErrorCallback m_error_handler;
    
    // Statistics
    atomic<uint64_t> m_total_messages_sent{0};
    atomic<uint64_t> m_total_messages_received{0};
    atomic<uint64_t> m_total_bytes_sent{0};
    atomic<uint64_t> m_total_bytes_received{0};
    atomic<uint64_t> m_connection_count{0};
    
    // Native socket implementation (simplified for portability)
    int m_server_socket{-1};
    condition_variable m_cv;
    mutex m_cv_mutex;
};

// =============================================================================
// WEBSOCKET CLIENT
// =============================================================================

/**
 * @brief WebSocket client for connecting to remote servers
 */
class WebSocketClient {
public:
    explicit WebSocketClient(string server_url);
    ~WebSocketClient();
    
    void connect();
    void disconnect();
    [[nodiscard]] bool isConnected() const noexcept;
    
    void send(const Protocol::Message& msg);
    void setMessageHandler(function<void(const Protocol::Message&)> handler);
    void setErrorHandler(function<void(const string&)> handler);

private:
    void clientLoop();
    void processIncomingData(const vector<uint8_t>& data);
    
    string m_server_url;
    atomic<bool> m_connected{false};
    atomic<bool> m_running{false};
    thread m_client_thread;
    
    function<void(const Protocol::Message&)> m_message_handler;
    function<void(const string&)> m_error_handler;
    
    atomic<uint64_t> m_messages_sent{0};
    atomic<uint64_t> m_messages_received{0};
};

// =============================================================================
// WEBSOCKET FACTORY
// =============================================================================

/**
 * @brief Factory for creating WebSocket components
 */
class WebSocketFactory {
public:
    static unique_ptr<WebSocketGateway> createGateway(const Protocol::HandlerConfig& config);
    static unique_ptr<WebSocketClient> createClient(const string& server_url);
    
    static Protocol::HandlerConfig defaultGatewayConfig();
};

} // export namespace MiniSonic::WebSocket
