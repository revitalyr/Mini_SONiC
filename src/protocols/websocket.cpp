module;

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <utility>
#include <sstream>

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

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost::system::error_code;

// =============================================================================
// WEBSOCKET CONNECTION
// =============================================================================

WebSocketConnection::WebSocketConnection(string connection_id, asio::io_context& io_context)
    : m_connection_id(std::move(connection_id))
    , m_socket(io_context)
    , m_strand(asio::make_strand(io_context))
    , m_read_buffer(4096)
{
}

WebSocketConnection::~WebSocketConnection() {
    stop();
}

void WebSocketConnection::start() {
    m_state = ConnectionState::CONNECTED;
    doRead();
}

void WebSocketConnection::stop() {
    m_stopped.store(true);
    m_state = ConnectionState::DISCONNECTED;
    
    error_code ec;
    m_socket.close(ec);
    
    if (m_close_handler) {
        m_close_handler();
    }
}

void WebSocketConnection::write(const vector<uint8_t>& data) {
    if (m_stopped.load() || !m_socket.is_open()) {
        return;
    }
    
    asio::post(m_strand, [self = shared_from_this(), data]() {
        bool write_in_progress = !self->m_write_buffer.empty();
        self->m_write_buffer.insert(self->m_write_buffer.end(), data.begin(), data.end());
        if (!write_in_progress) {
            self->doWrite();
        }
    });
}

void WebSocketConnection::doRead() {
    auto self = shared_from_this();
    m_socket.async_read_some(
        asio::buffer(m_read_buffer),
        asio::bind_executor(m_strand,
            [this, self](const error_code& ec, size_t bytes_read) {
                if (!ec) {
                    m_bytes_received.fetch_add(bytes_read);
                    if (m_message_handler) {
                        vector<uint8_t> data(m_read_buffer.begin(), m_read_buffer.begin() + bytes_read);
                        m_message_handler(data);
                    }
                    doRead();
                } else if (ec != asio::error::operation_aborted) {
                    handleError(ec);
                }
            }));
}

void WebSocketConnection::doWrite() {
    auto self = shared_from_this();
    asio::async_write(m_socket,
        asio::buffer(m_write_buffer),
        asio::bind_executor(m_strand,
            [this, self](const error_code& ec, size_t bytes_written) {
                if (!ec) {
                    m_bytes_sent.fetch_add(bytes_written);
                    m_write_buffer.erase(m_write_buffer.begin(), m_write_buffer.begin() + bytes_written);
                    if (!m_write_buffer.empty()) {
                        doWrite();
                    }
                } else if (ec != asio::error::operation_aborted) {
                    handleError(ec);
                }
            }));
}

void WebSocketConnection::handleError(const error_code& ec) {
    if (ec != asio::error::eof && ec != asio::error::connection_reset) {
        std::cerr << "[WebSocketConnection] Error: " << ec.message() << "\n";
    }
    stop();
}

void WebSocketConnection::setMessageHandler(function<void(const vector<uint8_t>&)> handler) {
    m_message_handler = std::move(handler);
}

void WebSocketConnection::setCloseHandler(function<void()> handler) {
    m_close_handler = std::move(handler);
}

// =============================================================================
// WEBSOCKET GATEWAY
// =============================================================================

WebSocketGateway::WebSocketGateway(const Protocol::HandlerConfig& config)
    : m_config(config)
    , m_acceptor(m_io_context)
{
}

WebSocketGateway::~WebSocketGateway() {
    stop();
}

void WebSocketGateway::start() {
    if (m_running.load()) {
        return;
    }
    
    m_running.store(true);
    m_connection_count.store(0);
    
    tcp::endpoint endpoint(asio::ip::make_address(m_config.bind_address), m_config.listen_port);
    
    try {
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind(endpoint);
        m_acceptor.listen(asio::socket_base::max_listen_connections);
    } catch (const std::exception& e) {
        if (m_error_handler) {
            m_error_handler(string("Failed to setup acceptor: ") + e.what());
        }
        m_running.store(false);
        return;
    }
    
    m_work = std::make_unique<asio::io_context::work>(m_io_context);
    m_io_thread = thread([this]() { m_io_context.run(); });
    
    doAccept();
    
    std::cout << "[WebSocketGateway] Started on " << m_config.bind_address << ":" 
              << m_config.listen_port << "\n";
}

void WebSocketGateway::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    
    {
        lock_guard<mutex> lock(m_connections_mutex);
        for (auto& [id, conn] : m_connections) {
            conn->stop();
        }
        m_connections.clear();
    }
    
    error_code ec;
    m_acceptor.close(ec);
    
    if (m_work) {
        m_work.reset();
    }
    
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
    
    std::cout << "[WebSocketGateway] Stopped\n";
}

bool WebSocketGateway::isRunning() const noexcept {
    return m_running.load();
}

void WebSocketGateway::doAccept() {
    auto conn = std::make_shared<WebSocketConnection>(generateConnectionId(), m_io_context);
    
    m_acceptor.async_accept(conn->socket(),
        [this, conn](const error_code& ec) {
            handleAccept(ec, conn);
        });
}

void WebSocketGateway::handleAccept(const error_code& ec, shared_ptr<WebSocketConnection> conn) {
    if (!ec) {
        conn->setState(ConnectionState::CONNECTED);
        
        try {
            conn->setRemoteAddress(conn->socket().remote_endpoint().address().to_string() + ":" +
                                   std::to_string(conn->socket().remote_endpoint().port()));
        } catch (...) {
            conn->setRemoteAddress("unknown");
        }
        
        string conn_id = conn->connectionId();
        {
            lock_guard<mutex> lock(m_connections_mutex);
            m_connections[conn_id] = conn;
            m_connection_count.fetch_add(1);
        }
        
        conn->setMessageHandler(
            [this, conn_id](const vector<uint8_t>& data) {
                handleConnectionMessage(conn_id, data);
            });
        conn->setCloseHandler(
            [this, conn_id]() {
                handleConnectionClose(conn_id);
            });
        
        conn->start();
        
        std::cout << "[WebSocketGateway] New connection: " << conn_id 
                  << " from " << conn->remoteAddress() << "\n";
        
        if (m_running.load()) {
            doAccept();
        }
    } else if (ec != asio::error::operation_aborted) {
        std::cerr << "[WebSocketGateway] Accept error: " << ec.message() << "\n";
        if (m_error_handler) {
            m_error_handler("Accept failed: " + ec.message());
        }
        if (m_running.load()) {
            doAccept();
        }
    }
}

void WebSocketGateway::handleConnectionMessage(const string& conn_id, const vector<uint8_t>& data) {
    auto msg = deserializeMessage(data);
    if (msg.isValid() && m_message_handler) {
        m_total_messages_received.fetch_add(1);
        m_total_bytes_received.fetch_add(data.size());
        m_message_handler(msg);
    }
}

void WebSocketGateway::handleConnectionClose(const string& conn_id) {
    lock_guard<mutex> lock(m_connections_mutex);
    auto it = m_connections.find(conn_id);
    if (it != m_connections.end()) {
        it->second->setState(ConnectionState::DISCONNECTED);
        m_connections.erase(it);
        m_connection_count.fetch_sub(1);
        std::cout << "[WebSocketGateway] Connection closed: " << conn_id << "\n";
    }
}

void WebSocketGateway::send(const Protocol::Message& msg) {
    broadcast(msg);
}

void WebSocketGateway::broadcast(const Protocol::Message& msg) {
    auto data = serializeMessage(msg);
    
    lock_guard<mutex> lock(m_connections_mutex);
    for (auto& [id, conn] : m_connections) {
        if (conn->isActive()) {
            conn->write(data);
            m_total_bytes_sent.fetch_add(data.size());
            m_total_messages_sent.fetch_add(1);
        }
    }
}

void WebSocketGateway::sendToConnection(const string& connection_id, const Protocol::Message& msg) {
    auto data = serializeMessage(msg);
    
    lock_guard<mutex> lock(m_connections_mutex);
    auto it = m_connections.find(connection_id);
    if (it != m_connections.end() && it->second->isActive()) {
        it->second->write(data);
        m_total_bytes_sent.fetch_add(data.size());
        m_total_messages_sent.fetch_add(1);
    }
}

void WebSocketGateway::closeConnection(const string& connection_id) {
    lock_guard<mutex> lock(m_connections_mutex);
    auto it = m_connections.find(connection_id);
    if (it != m_connections.end()) {
        it->second->stop();
        m_connections.erase(it);
        m_connection_count.fetch_sub(1);
    }
}

size_t WebSocketGateway::activeConnections() const {
    lock_guard<mutex> lock(m_connections_mutex);
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
    std::ostringstream oss;
    oss << "WebSocket Gateway Statistics:\n"
        << "  Running: " << (m_running.load() ? "yes" : "no") << "\n"
        << "  Active Connections: " << activeConnections() << "\n"
        << "  Total Messages Sent: " << m_total_messages_sent.load() << "\n"
        << "  Total Messages Received: " << m_total_messages_received.load() << "\n"
        << "  Total Bytes Sent: " << m_total_bytes_sent.load() << "\n"
        << "  Total Bytes Received: " << m_total_bytes_received.load() << "\n";
    return oss.str();
}

vector<uint8_t> WebSocketGateway::serializeMessage(const Protocol::Message& msg) {
    vector<uint8_t> buffer;
    buffer.reserve(4 + msg.payload().size());
    
    uint32_t type = static_cast<uint32_t>(msg.header().type);
    buffer.push_back(static_cast<uint8_t>(type));
    buffer.push_back(static_cast<uint8_t>(type >> 8));
    buffer.push_back(static_cast<uint8_t>(type >> 16));
    buffer.push_back(static_cast<uint8_t>(type >> 24));
    
    const auto& payload = msg.payload();
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    
    return buffer;
}

Protocol::Message WebSocketGateway::deserializeMessage(const vector<uint8_t>& data) {
    Protocol::Message msg;
    
    if (data.size() < 4) {
        return msg;
    }
    
    uint32_t type = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    msg.setType(static_cast<Protocol::MessageType>(type));
    
    if (data.size() > 4) {
        vector<uint8_t> payload(data.begin() + 4, data.end());
        msg.payload() = std::move(payload);
    }
    
    return msg;
}

string WebSocketGateway::generateConnectionId() {
    return "ws_conn_" + std::to_string(m_connection_id_counter.fetch_add(1));
}

void WebSocketGateway::removeConnection(const string& connection_id) {
    lock_guard<mutex> lock(m_connections_mutex);
    auto it = m_connections.find(connection_id);
    if (it != m_connections.end()) {
        it->second->stop();
        m_connections.erase(it);
        m_connection_count.fetch_sub(1);
        std::cout << "[WebSocketGateway] Connection removed: " << connection_id << "\n";
    }
}

// =============================================================================
// WEBSOCKET CLIENT
// =============================================================================

WebSocketClient::WebSocketClient(const string& server_url)
    : m_server_url(server_url)
    , m_socket(m_io_context)
    , m_read_buffer(4096)
{
    parseUrl(server_url);
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::parseUrl(const string& url) {
    if (url.substr(0, 5) == "ws://") {
        size_t host_start = 5;
        size_t host_end = url.find(':', host_start);
        size_t path_start = url.find('/', host_start);
        
        if (host_end == string::npos || host_end > path_start) {
            host_end = path_start;
        }
        
        m_host = url.substr(host_start, host_end - host_start);
        
        if (host_end != string::npos && url[host_end] == ':') {
            size_t port_end = path_start == string::npos ? string::npos : path_start;
            m_port = url.substr(host_end + 1, port_end - host_end - 1);
        } else {
            m_port = "80";
        }
        
        if (path_start != string::npos) {
            m_path = url.substr(path_start);
        } else {
            m_path = "/";
        }
        return true;
    }
    return false;
}

void WebSocketClient::connect() {
    if (m_connected.load()) {
        return;
    }
    
    m_running.store(true);
    
    tcp::resolver resolver(m_io_context);
    try {
        auto endpoints = resolver.resolve(m_host, m_port);
        
        m_work = std::make_unique<asio::io_context::work>(m_io_context);
        m_io_thread = thread([this]() { m_io_context.run(); });
        
        asio::async_connect(m_socket, endpoints,
            [this](const error_code& ec, tcp::endpoint) {
                handleConnect(ec);
            });
    } catch (const std::exception& e) {
        if (m_error_handler) {
            m_error_handler(string("Resolve failed: ") + e.what());
        }
        m_running.store(false);
    }
}

void WebSocketClient::handleConnect(const error_code& ec) {
    if (!ec) {
        m_connected.store(true);
        std::cout << "[WebSocketClient] Connected to " << m_server_url << "\n";
        doRead();
    } else {
        if (m_error_handler) {
            m_error_handler("Connect failed: " + ec.message());
        }
        m_running.store(false);
    }
}

void WebSocketClient::disconnect() {
    m_running.store(false);
    m_connected.store(false);
    
    if (m_work) {
        m_work.reset();
    }
    
    error_code ec;
    m_socket.close(ec);
    
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
    
    std::cout << "[WebSocketClient] Disconnected\n";
}

bool WebSocketClient::isConnected() const noexcept {
    return m_connected.load();
}

void WebSocketClient::send(const Protocol::Message& msg) {
    if (!m_connected.load() || !m_socket.is_open()) {
        return;
    }
    
    uint32_t type = static_cast<uint32_t>(msg.header().type);
    lock_guard<mutex> lock(m_write_mutex);
    
    m_write_buffer.push_back(static_cast<uint8_t>(type));
    m_write_buffer.push_back(static_cast<uint8_t>(type >> 8));
    m_write_buffer.push_back(static_cast<uint8_t>(type >> 16));
    m_write_buffer.push_back(static_cast<uint8_t>(type >> 24));
    
    const auto& payload = msg.payload();
    m_write_buffer.insert(m_write_buffer.end(), payload.begin(), payload.end());
    
    doWrite();
}

void WebSocketClient::doWrite() {
    auto self = shared_from_this();
    asio::async_write(m_socket, asio::buffer(m_write_buffer),
        [this, self](const error_code& ec, size_t bytes_written) {
            if (!ec) {
                m_messages_sent.fetch_add(1);
                lock_guard<mutex> lock(m_write_mutex);
                m_write_buffer.erase(m_write_buffer.begin(), m_write_buffer.begin() + bytes_written);
            } else if (m_error_handler && ec != asio::error::operation_aborted) {
                m_error_handler("Write failed: " + ec.message());
                disconnect();
            }
        });
}

void WebSocketClient::doRead() {
    auto self = shared_from_this();
    m_socket.async_read_some(asio::buffer(m_read_buffer),
        [this, self](const error_code& ec, size_t bytes_read) {
            handleRead(ec, bytes_read);
        });
}

void WebSocketClient::handleRead(const error_code& ec, size_t bytes_read) {
    if (!ec) {
        m_messages_received.fetch_add(1);
        if (m_message_handler) {
            Protocol::Message msg;
            if (bytes_read >= 4) {
                uint32_t type = m_read_buffer[0] | (m_read_buffer[1] << 8) |
                               (m_read_buffer[2] << 16) | (m_read_buffer[3] << 24);
                msg.setType(static_cast<Protocol::MessageType>(type));
                if (bytes_read > 4) {
                    vector<uint8_t> payload(m_read_buffer.begin() + 4,
                                            m_read_buffer.begin() + bytes_read);
                    msg.payload() = std::move(payload);
                }
            }
            m_message_handler(msg);
        }
        if (m_running.load()) {
            doRead();
        }
    } else if (ec != asio::error::operation_aborted) {
        if (m_error_handler) {
            m_error_handler("Read failed: " + ec.message());
        }
        disconnect();
    }
}

void WebSocketClient::setMessageHandler(function<void(const Protocol::Message&)> handler) {
    m_message_handler = std::move(handler);
}

void WebSocketClient::setErrorHandler(function<void(const string&)> handler) {
    m_error_handler = std::move(handler);
}

// =============================================================================
// WEBSOCKET FACTORY
// =============================================================================

unique_ptr<WebSocketGateway> WebSocketFactory::createGateway(const Protocol::HandlerConfig& config) {
    return std::make_unique<WebSocketGateway>(config);
}

unique_ptr<WebSocketClient> WebSocketFactory::createClient(const string& server_url) {
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
