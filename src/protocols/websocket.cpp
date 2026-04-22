module;

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <utility>

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

module MiniSonic.WebSocket;

import MiniSonic.Core.Utils;
import MiniSonic.Protocol;

using std::string;
using std::vector;
using std::function;
using std::lock_guard;
using std::mutex;
using std::atomic;

namespace MiniSonic::WebSocket {

// =============================================================================
// WEBSOCKET CONNECTION IMPLEMENTATION
// =============================================================================

WebSocketConnection::WebSocketConnection(std::string connection_id)
    : m_connection_id(std::move(connection_id)) {}

// =============================================================================
// WEBSOCKET GATEWAY IMPLEMENTATION
// =============================================================================

WebSocketGateway::WebSocketGateway(const Protocol::HandlerConfig& config)
    : m_config(config) {}

WebSocketGateway::~WebSocketGateway() {
    stop();
}

void WebSocketGateway::start() {
    if (m_running.load()) {
        return;
    }
    
    m_running.store(true);
    m_connection_count.store(0);
    
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
    
    // Create server socket
    m_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server_socket == INVALID_SOCKET) {
        if (m_error_handler) {
            m_error_handler("Failed to create server socket");
        }
        return;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, 
               reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind to address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_config.listen_port);
    
    if (bind(m_server_socket, reinterpret_cast<sockaddr*>(&server_addr), 
             sizeof(server_addr)) == SOCKET_ERROR) {
        if (m_error_handler) {
            m_error_handler("Failed to bind server socket");
        }
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
        return;
    }
    
    // Listen
    if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR) {
        if (m_error_handler) {
            m_error_handler("Failed to listen on server socket");
        }
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
        return;
    }
    
    // Start threads
    m_server_thread = std::thread(&WebSocketGateway::serverLoop, this);
    m_accept_thread = std::thread(&WebSocketGateway::acceptLoop, this);
    
    std::cout << "[WebSocketGateway] Started on port " << m_config.listen_port << "\n";
}

void WebSocketGateway::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    m_cv.notify_all();
    
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
    
    if (m_accept_thread.joinable()) {
        m_accept_thread.join();
    }
    
    // Close all connections
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    m_connections.clear();
    
    // Close server socket
    if (m_server_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "[WebSocketGateway] Stopped\n";
}

bool WebSocketGateway::isRunning() const noexcept {
    return m_running.load();
}

void WebSocketGateway::send(const Protocol::Message& msg) {
    // Broadcast to all connections
    broadcast(msg);
}

void WebSocketGateway::broadcast(const Protocol::Message& msg) {
    auto data = serializeMessage(msg);
    
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    for (auto& [id, conn] : m_connections) {
        if (conn->isActive()) {
            // In real implementation, send via WebSocket protocol
            m_total_bytes_sent.fetch_add(data.size());
            m_total_messages_sent.fetch_add(1);
        }
    }
}

void WebSocketGateway::sendToConnection(const string& connection_id, const Protocol::Message& msg) {
    auto data = serializeMessage(msg);
    
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    auto it = m_connections.find(connection_id);
    if (it != m_connections.end() && it->second->isActive()) {
        m_total_bytes_sent.fetch_add(data.size());
        m_total_messages_sent.fetch_add(1);
    }
}

void WebSocketGateway::closeConnection(const string& connection_id) {
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    removeConnection(connection_id);
}

size_t WebSocketGateway::activeConnections() const {
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    size_t count = 0;
    for (const auto& [id, conn] : m_connections) {
        if (conn->isActive()) {
            count++;
        }
    }
    return count;
}

void WebSocketGateway::setMessageHandler(MessageCallback handler) {
    m_message_handler = std::move(handler);
}

void WebSocketGateway::setErrorHandler(ErrorCallback handler) {
    m_error_handler = std::move(handler);
}

std::string WebSocketGateway::getStats() const {
    return "WebSocket Gateway Statistics:\n"
           "  Running: " + std::string(m_running.load() ? "yes" : "no") + "\n"
           "  Active Connections: " + std::to_string(activeConnections()) + "\n"
           "  Total Messages Sent: " + std::to_string(m_total_messages_sent.load()) + "\n"
           "  Total Messages Received: " + std::to_string(m_total_messages_received.load()) + "\n"
           "  Total Bytes Sent: " + std::to_string(m_total_bytes_sent.load()) + "\n"
           "  Total Bytes Received: " + std::to_string(m_total_bytes_received.load()) + "\n";
}

void WebSocketGateway::serverLoop() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(100));
    }
}

void WebSocketGateway::acceptLoop() {
    while (m_running.load()) {
        sockaddr_in client_addr{};
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        
        SOCKET client_socket = accept(m_server_socket,
                                      reinterpret_cast<sockaddr*>(&client_addr), 
                                      &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (m_running.load()) {
                // Accept failed but server is still running
                continue;
            }
            break;
        }
        
        // Create new connection
        string conn_id = generateConnectionId();
        auto conn = std::make_shared<WebSocketConnection>(conn_id);
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        conn->setRemoteAddress(string(client_ip) + ":" + 
                               std::to_string(ntohs(client_addr.sin_port)));
        conn->setState(ConnectionState::CONNECTED);
        
        {
            std::lock_guard<std::mutex> lock(m_connections_mutex);
            m_connections[conn_id] = conn;
            m_connection_count.fetch_add(1);
        }
        
        std::cout << "[WebSocketGateway] New connection: " << conn_id 
                  << " from " << conn->remoteAddress() << "\n";
        
        // In real implementation, start per-connection thread
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }
}

void WebSocketGateway::processIncomingMessage(const string& connection_id, 
                                              const vector<uint8_t>& data) {
    auto msg = deserializeMessage(data);
    if (msg.isValid() && m_message_handler) {
        m_total_messages_received.fetch_add(1);
        m_total_bytes_received.fetch_add(data.size());
        m_message_handler(msg);
    }
}

std::vector<uint8_t> WebSocketGateway::serializeMessage(const Protocol::Message& msg) {
    // Simplified serialization - in real implementation use proper WebSocket framing
    vector<uint8_t> buffer;
    
    // Add message type
    uint32_t type = static_cast<uint32_t>(msg.header().type);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&type),
                  reinterpret_cast<uint8_t*>(&type) + sizeof(type));
    
    // Add payload
    const auto& payload = msg.payload();
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    
    return buffer;
}

Protocol::Message WebSocketGateway::deserializeMessage(const std::vector<uint8_t>& data) {
    Protocol::Message msg;
    
    if (data.size() < sizeof(uint32_t)) {
        return msg;
    }
    
    uint32_t type;
    std::memcpy(&type, data.data(), sizeof(type));
    msg.setType(static_cast<Protocol::MessageType>(type));
    
    if (data.size() > sizeof(uint32_t)) {
        std::vector<uint8_t> payload(data.begin() + sizeof(uint32_t), data.end());
        msg.payload() = std::move(payload);
    }
    
    return msg;
}

std::string WebSocketGateway::generateConnectionId() {
    static atomic<uint64_t> counter{0};
    return "ws_conn_" + std::to_string(counter.fetch_add(1));
}

void WebSocketGateway::removeConnection(const string& connection_id) {
    auto it = m_connections.find(connection_id); // Use member mutex
    if (it != m_connections.end()) {
        it->second->setState(ConnectionState::DISCONNECTED);
        m_connections.erase(it);
        m_connection_count.fetch_sub(1);
        std::cout << "[WebSocketGateway] Connection removed: " << connection_id << "\n";
    }
}

// =============================================================================
// WEBSOCKET CLIENT IMPLEMENTATION
// =============================================================================

WebSocketClient::WebSocketClient(std::string server_url)
    : m_server_url(std::move(server_url)) {}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::connect() {
    // Simplified connection logic
    m_connected.store(true);
    m_running.store(true);
    m_client_thread = std::thread(&WebSocketClient::clientLoop, this);
    std::cout << "[WebSocketClient] Connected to " << m_server_url << "\n";
}

void WebSocketClient::disconnect() {
    m_running.store(false);
    m_connected.store(false);
    
    if (m_client_thread.joinable()) {
        m_client_thread.join();
    }
    
    std::cout << "[WebSocketClient] Disconnected\n";
}

bool WebSocketClient::isConnected() const noexcept {
    return m_connected.load();
}

void WebSocketClient::send(const Protocol::Message& msg) {
    // Simplified send - in real implementation use WebSocket protocol
    m_messages_sent.fetch_add(1);
}

void WebSocketClient::setMessageHandler(function<void(const Protocol::Message&)> handler) {
    m_message_handler = std::move(handler);
}

void WebSocketClient::setErrorHandler(function<void(const string&)> handler) {
    m_error_handler = std::move(handler);
}

void WebSocketClient::clientLoop() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void WebSocketClient::processIncomingData(const vector<uint8_t>& data) {
    // Process incoming WebSocket data
    m_messages_received.fetch_add(1);
}

// =============================================================================
// WEBSOCKET FACTORY IMPLEMENTATION
// =============================================================================

std::unique_ptr<WebSocketGateway> WebSocketFactory::createGateway(const Protocol::HandlerConfig& config) {
    return std::make_unique<WebSocketGateway>(config);
}

std::unique_ptr<WebSocketClient> WebSocketFactory::createClient(const std::string& server_url) {
    return std::make_unique<WebSocketClient>(server_url);
}

Protocol::HandlerConfig WebSocketFactory::defaultGatewayConfig() {
    Protocol::HandlerConfig config;
    config.listen_port = 8080;
    config.bind_address = "0.0.0.0";
    config.max_connections = 100;
    config.buffer_size = 4096;
    return config;
}

} // namespace MiniSonic::WebSocket
