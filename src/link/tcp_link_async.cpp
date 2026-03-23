#include "link/tcp_link_async.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace MiniSonic::Link {

TcpLinkAsync::TcpLinkAsync(
    boost::asio::io_context& io_context,
    Types::Port listen_port,
    const Types::String& peer_ip,
    Types::Port peer_port
) : m_io_context(io_context),
    m_acceptor(io_context, Tcp::endpoint(Tcp::v4(), listen_port)),
    m_socket(io_context),
    m_peer_endpoint(
        peer_ip.empty() ? 
        Tcp::endpoint() : 
        Tcp::endpoint(boost::asio::ip::make_address(peer_ip), peer_port)
    ),
    m_strand(io_context),
    m_listen_port(listen_port),
    m_peer_ip(peer_ip),
    m_peer_port(peer_port) {
}

TcpLinkAsync::~TcpLinkAsync() {
    stop();
}

void TcpLinkAsync::start() {
    m_running.store(true);
    
    std::cout << "[LINK] Starting async TCP link on port " << m_listen_port;
    if (!m_peer_ip.empty()) {
        std::cout << " with peer " << m_peer_ip << ":" << m_peer_port;
    }
    std::cout << "\n";

    // Start accepting connections if we have a peer
    if (!m_peer_ip.empty()) {
        doConnect();
    }
    
    // Always start accepting for incoming connections
    doAccept();
}

void TcpLinkAsync::stop() {
    m_running.store(false);
    m_connected.store(false);
    
    boost::system::error_code ec;
    m_socket.close(ec);
    m_acceptor.close(ec);
    
    if (ec) {
        std::cerr << "[LINK] Error during shutdown: " << ec.message() << "\n";
    }
    
    std::cout << "[LINK] Stopped\n";
}

void TcpLinkAsync::send(const DataPlane::Packet& pkt) {
    if (!m_connected.load() || !m_socket.is_open()) {
        Utils::Metrics::instance().inc("link_send_dropped");
        return;
    }

    const auto data = serializePacket(pkt);
    
    // Use strand to ensure thread safety
    boost::asio::post(m_strand, [self = shared_from_this(), data]() {
        boost::asio::async_write(
            self->m_socket,
            boost::asio::buffer(data),
            [self, data](const ErrorCode& ec, std::size_t bytes_sent) {
                if (!ec) {
                    self->m_packets_sent.fetch_add(1, std::memory_order_relaxed);
                    self->m_bytes_sent.fetch_add(bytes_sent, std::memory_order_relaxed);
                    Utils::Metrics::instance().inc("link_tx");
                } else {
                    std::cerr << "[LINK] Send error: " << ec.message() << "\n";
                    Utils::Metrics::instance().inc("link_send_errors");
                    self->onDisconnected();
                }
            }
        );
    });
}

void TcpLinkAsync::setHandler(PacketHandler handler) {
    m_handler = std::move(handler);
}

bool TcpLinkAsync::isConnected() const noexcept {
    return m_connected.load() && m_socket.is_open();
}

Types::String TcpLinkAsync::getStats() const {
    std::ostringstream oss;
    oss << "TCP Link Stats:\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Connected: " << (m_connected.load() ? "Yes" : "No") << "\n"
         << "  Packets Sent: " << m_packets_sent.load() << "\n"
         << "  Packets Received: " << m_packets_received.load() << "\n"
         << "  Bytes Sent: " << m_bytes_sent.load() << "\n"
         << "  Bytes Received: " << m_bytes_received.load() << "\n"
         << "  Listen Port: " << m_listen_port << "\n"
         << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n";
    return oss.str();
}

void TcpLinkAsync::doAccept() {
    if (!m_running.load()) {
        return;
    }

    auto self = shared_from_this();
    m_acceptor.async_accept(
        [self](const ErrorCode& ec, Tcp::socket new_socket) {
            if (!ec) {
                std::cout << "[LINK] Accepted incoming connection\n";
                
                // Close current socket and use new one
                boost::system::error_code close_ec;
                self->m_socket.close(close_ec);
                self->m_socket = std::move(new_socket);
                
                self->onConnected();
                self->doRead();
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cerr << "[LINK] Accept error: " << ec.message() << "\n";
                Utils::Metrics::instance().inc("link_accept_errors");
            }
            
            // Continue accepting
            if (self->m_running.load()) {
                self->doAccept();
            }
        }
    );
}

void TcpLinkAsync::doConnect() {
    if (!m_running.load() || m_peer_ip.empty()) {
        return;
    }

    auto self = shared_from_this();
    m_socket.async_connect(
        m_peer_endpoint,
        [self](const ErrorCode& ec) {
            if (!ec) {
                std::cout << "[LINK] Connected to peer " 
                         << self->m_peer_ip << ":" << self->m_peer_port << "\n";
                self->onConnected();
                self->doRead();
            } else {
                std::cerr << "[LINK] Connect error: " << ec.message() << "\n";
                Utils::Metrics::instance().inc("link_connect_errors");
                
                // Retry connection after delay
                if (self->m_running.load()) {
                    boost::asio::post(
                        self->m_strand,
                        [self]() {
                            auto timer = std::make_shared<boost::asio::steady_timer>(
                                self->m_io_context, 
                                std::chrono::seconds(1)
                            );
                            timer->async_wait([self, timer](const ErrorCode&) {
                                self->doConnect();
                            });
                        }
                    );
                }
            }
        }
    );
}

void TcpLinkAsync::doRead() {
    if (!m_connected.load()) {
        return;
    }

    auto self = shared_from_this();
    m_socket.async_read_some(
        boost::asio::buffer(m_buffer),
        [self](const ErrorCode& ec, std::size_t bytes_read) {
            if (!ec) {
                self->m_receive_buffer.append(
                    self->m_buffer.data(), 
                    bytes_read
                );
                self->m_bytes_received.fetch_add(bytes_read, std::memory_order_relaxed);
                
                // Try to extract complete packets
                while (true) {
                    const auto newline_pos = self->m_receive_buffer.find('\n');
                    if (newline_pos == Types::String::npos) {
                        break; // Incomplete packet
                    }
                    
                    const auto packet_data = self->m_receive_buffer.substr(0, newline_pos);
                    self->m_receive_buffer.erase(0, newline_pos + 1);
                    
                    try {
                        const auto pkt = deserializePacket(packet_data);
                        self->onPacketReceived(pkt);
                    } catch (const std::exception& e) {
                        std::cerr << "[LINK] Packet deserialization error: " 
                                 << e.what() << "\n";
                        Utils::Metrics::instance().inc("link_parse_errors");
                    }
                }
                
                // Continue reading
                self->doRead();
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cerr << "[LINK] Read error: " << ec.message() << "\n";
                Utils::Metrics::instance().inc("link_read_errors");
                self->onDisconnected();
            }
        }
    );
}

void TcpLinkAsync::onConnected() {
    m_connected.store(true);
    Utils::Metrics::instance().inc("link_connections");
    Utils::Metrics::instance().set("link_connected", 1);
}

void TcpLinkAsync::onDisconnected() {
    if (m_connected.load()) {
        m_connected.store(false);
        Utils::Metrics::instance().inc("link_disconnections");
        Utils::Metrics::instance().set("link_connected", 0);
        
        std::cout << "[LINK] Disconnected\n";
        
        // Try to reconnect if we have a peer
        if (m_running.load() && !m_peer_ip.empty()) {
            boost::asio::post(
                m_strand,
                [self = shared_from_this()]() {
                    auto timer = std::make_shared<boost::asio::steady_timer>(
                        self->m_io_context, 
                        std::chrono::seconds(2)
                    );
                    timer->async_wait([self, timer](const ErrorCode&) {
                        self->doConnect();
                    });
                }
            );
        }
    }
}

void TcpLinkAsync::onPacketReceived(const DataPlane::Packet& pkt) {
    m_packets_received.fetch_add(1, std::memory_order_relaxed);
    Utils::Metrics::instance().inc("link_rx");
    
    if (m_handler) {
        m_handler(pkt);
    }
}

Types::String TcpLinkAsync::serializePacket(const DataPlane::Packet& pkt) {
    std::ostringstream oss;
    oss << pkt.srcMac() << ","
       << pkt.dstMac() << ","
       << pkt.srcIp() << ","
       << pkt.dstIp() << ","
       << pkt.ingressPort() << "\n";
    return oss.str();
}

DataPlane::Packet TcpLinkAsync::deserializePacket(const Types::String& data) {
    std::istringstream iss(data);
    Types::String src_mac, dst_mac, src_ip, dst_ip;
    Types::Port ingress_port;
    
    std::getline(iss, src_mac, ',');
    std::getline(iss, dst_mac, ',');
    std::getline(iss, src_ip, ',');
    std::getline(iss, dst_ip, ',');
    iss >> ingress_port;
    
    return DataPlane::Packet(
        src_mac, dst_mac, src_ip, dst_ip, ingress_port
    );
}

} // namespace MiniSonic::Link
