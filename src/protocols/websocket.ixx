module;

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstdint>
#include <cstring>

export module MiniSonic.WebSocket;

import MiniSonic.Protocol;
import MiniSonic.Core.Utils;

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::atomic;
using std::mutex;
using std::thread;

export namespace MiniSonic::WebSocket {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// =============================================================================
// WEBSOCKET CONNECTION
// =============================================================================

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CLOSING
};

class WebSocketConnection : public std::enable_shared_from_this<WebSocketConnection> {
public:
    WebSocketConnection(string connection_id, asio::io_context& io_context);
    ~WebSocketConnection();
    
    [[nodiscard]] string connectionId() const noexcept { return m_connection_id; }
    [[nodiscard]] ConnectionState state() const noexcept { return m_state; }
    [[nodiscard]] string remoteAddress() const noexcept { return m_remote_address; }
    [[nodiscard]] tcp::socket& socket() noexcept { return m_socket; }
    
    void setState(ConnectionState state) noexcept { m_state = state; }
    void setRemoteAddress(const string& address) { m_remote_address = address; }
    
    [[nodiscard]] bool isActive() const noexcept {
        return m_state == ConnectionState::CONNECTED;
    }
    
    void start();
    void stop();
    void write(const vector<uint8_t>& data);
    
    void setMessageHandler(function<void(const vector<uint8_t>&)> handler);
    void setCloseHandler(function<void()> handler);

private:
    void doRead();
    void doWrite();
    void handleError(const boost::system::error_code& ec);
    
    string m_connection_id;
    ConnectionState m_state{ConnectionState::DISCONNECTED};
    string m_remote_address;
    
    tcp::socket m_socket;
    asio::strand<asio::io_context::executor_type> m_strand;
    
    vector<uint8_t> m_read_buffer;
    vector<uint8_t> m_write_buffer;
    vector<uint8_t> m_pending_write;
    
    function<void(const vector<uint8_t>&)> m_message_handler;
    function<void()> m_close_handler;
    
    atomic<bool> m_stopped{false};
    atomic<uint64_t> m_bytes_sent{0};
    atomic<uint64_t> m_bytes_received{0};
};

// =============================================================================
// WEBSOCKET GATEWAY
// =============================================================================

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
    
    [[nodiscard]] std::string protocolName() const override { return "WebSocket"; }
    [[nodiscard]] std::string getStats() const override;
    
    // WebSocket-specific methods
    void sendToConnection(const std::string& connection_id, const Protocol::Message& msg);
    void closeConnection(const std::string& connection_id);
    [[nodiscard]] size_t activeConnections() const;

private:
    void doAccept();
    void handleAccept(const boost::system::error_code& ec, shared_ptr<WebSocketConnection> conn);
    void handleConnectionMessage(const string& conn_id, const vector<uint8_t>& data);
    void handleConnectionClose(const string& conn_id);
    void removeConnection(const string& connection_id);
    string generateConnectionId();
    
    vector<uint8_t> serializeMessage(const Protocol::Message& msg);
    Protocol::Message deserializeMessage(const vector<uint8_t>& data);
    
    Protocol::HandlerConfig m_config;
    
    asio::io_context m_io_context;
    unique_ptr<asio::io_context::work> m_work;
    tcp::acceptor m_acceptor;
    
    thread m_io_thread;
    atomic<bool> m_running{false};
    
    map<string, shared_ptr<WebSocketConnection>> m_connections;
    mutable mutex m_connections_mutex;
    
    MessageCallback m_message_handler;
    ErrorCallback m_error_handler;
    
    atomic<uint64_t> m_total_messages_sent{0};
    atomic<uint64_t> m_total_messages_received{0};
    atomic<uint64_t> m_total_bytes_sent{0};
    atomic<uint64_t> m_total_bytes_received{0};
    atomic<uint64_t> m_connection_count{0};
    atomic<uint64_t> m_connection_id_counter{0};
};

// =============================================================================
// WEBSOCKET CLIENT
// =============================================================================

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    explicit WebSocketClient(const string& server_url);
    ~WebSocketClient();
    
    void connect();
    void disconnect();
    [[nodiscard]] bool isConnected() const noexcept;
    
    void send(const Protocol::Message& msg);
    void setMessageHandler(function<void(const Protocol::Message&)> handler);
    void setErrorHandler(function<void(const string&)> handler);

private:
    void doConnect();
    void handleConnect(const boost::system::error_code& ec);
    void doRead();
    void handleRead(const boost::system::error_code& ec, size_t bytes_read);
    void doWrite();
    void handleWrite(const boost::system::error_code& ec, size_t bytes_written);
    
    string m_server_url;
    string m_host;
    string m_port;
    string m_path;
    
    asio::io_context m_io_context;
    unique_ptr<asio::io_context::work> m_work;
    tcp::socket m_socket;
    thread m_io_thread;
    
    atomic<bool> m_connected{false};
    atomic<bool> m_running{false};
    
    vector<uint8_t> m_read_buffer;
    vector<uint8_t> m_write_buffer;
    mutex m_write_mutex;
    
    function<void(const Protocol::Message&)> m_message_handler;
    function<void(const string&)> m_error_handler;
    
    atomic<uint64_t> m_messages_sent{0};
    atomic<uint64_t> m_messages_received{0};
    
    bool parseUrl(const string& url);
};

// =============================================================================
// WEBSOCKET FACTORY
// =============================================================================

class WebSocketFactory {
public:
    static unique_ptr<WebSocketGateway> createGateway(const Protocol::HandlerConfig& config);
    static unique_ptr<WebSocketClient> createClient(const string& server_url);
    static Protocol::HandlerConfig defaultGatewayConfig();
};

} // export namespace MiniSonic::WebSocket
