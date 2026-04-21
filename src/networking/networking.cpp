module;

#include <boost/system/error_code.hpp> // Must be at the absolute top for ADL on MSVC

#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
module MiniSonic.Networking;

// Import local modules
import MiniSonic.Core.Utils;

namespace MiniSonic::Networking {

#ifdef HAS_BOOST_ASIO

// BoostTcpLink Implementation
BoostTcpLink::BoostTcpLink(
    boost::asio::io_context& io_context,
    Types::PortId listen_port,
    const std::string& peer_ip,
    Types::PortId peer_port
) : m_io_context(io_context),
    m_acceptor(io_context),
    m_socket(io_context),
    m_peer_endpoint(boost::asio::ip::make_address(peer_ip), peer_port),
    m_strand(io_context.get_executor()),
    m_listen_port(listen_port),
    m_peer_ip(peer_ip),
    m_peer_port(peer_port) {
    
    std::cout << "[NETWORK] Creating Boost.Asio TCP link\n";
    std::cout << "  Listen Port: " << listen_port << "\n";
    std::cout << "  Peer: " << peer_ip << ":" << peer_port << "\n";
}

BoostTcpLink::~BoostTcpLink() {
    stop();
}

void BoostTcpLink::start() {
    if (m_running.load()) {
        return;
    }
    
    m_running.store(true);
    
    try {
        // Setup acceptor
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), m_listen_port);
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind(endpoint);
        m_acceptor.listen();
        
        std::cout << "[NETWORK] Boost.Asio TCP link started\n";
        
        // Start accepting connections
        if (!m_peer_ip.empty()) {
            doConnect();
        }
        doAccept();
        
    } catch (const std::exception& e) {
        std::cerr << "[NETWORK] Error starting Boost.Asio: " << e.what() << "\n";
        m_running.store(false);
    }
}

void BoostTcpLink::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    m_connected.store(false);
    
    try {
        m_acceptor.close();
        m_socket.close();
        std::cout << "[NETWORK] Boost.Asio TCP link stopped\n";
    } catch (const std::exception& e) {
        std::cerr << "[NETWORK] Error stopping Boost.Asio: " << e.what() << "\n";
    }
}

bool BoostTcpLink::isConnected() const noexcept {
    return m_connected.load();
}

void BoostTcpLink::send(const Packet& pkt) {
    if (!isConnected()) {
        return;
    }
    
    try {
        std::string data = serializePacket(pkt);
        
        auto self = shared_from_this();
        boost::asio::async_write(
            m_socket,
            boost::asio::buffer(data),
            boost::asio::bind_executor(
                m_strand,
                [this, self](boost::system::error_code ec, std::size_t bytes_sent) {
                    if (!ec) {
                        m_packets_sent.fetch_add(1, std::memory_order_relaxed);
                        m_bytes_sent.fetch_add(bytes_sent, std::memory_order_relaxed);
                    } else {
                        std::cerr << "[NETWORK] Send error: " << ec.message() << "\n";
                        onDisconnected();
                    }
                }
            )
        );
    } catch (const std::exception& e) {
        std::cerr << "[NETWORK] Send exception: " << e.what() << "\n";
    }
}

void BoostTcpLink::setPacketHandler(PacketHandler handler) {
    m_handler = std::move(handler);
}

std::string BoostTcpLink::getStats() const {
    std::ostringstream oss;
    oss << "Boost.Asio TCP Link Stats:\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Connected: " << (m_connected.load() ? "Yes" : "No") << "\n"
         << "  Packets Sent: " << m_packets_sent.load() << "\n"
         << "  Packets Received: " << m_packets_received.load() << "\n"
         << "  Bytes Sent: " << m_bytes_sent.load() << "\n"
         << "  Bytes Received: " << m_bytes_received.load() << "\n";
    return oss.str();
}

void BoostTcpLink::doAccept() {
    if (!m_running.load()) {
        return;
    }
    
    m_acceptor.async_accept(
        m_socket,
        [this](boost::system::error_code ec) {
            if (!ec) {
                onConnected();
                doRead();
            } else {
                std::cerr << "[NETWORK] Accept error: " << ec.message() << "\n";
                if (m_running.load()) {
                    doAccept();
                }
            }
        }
    );
}

void BoostTcpLink::doConnect() {
    m_socket.async_connect(
        m_peer_endpoint,
        [this](boost::system::error_code ec) {
            if (!ec) {
                onConnected();
                doRead();
            } else {
                std::cerr << "[NETWORK] Connect error: " << ec.message() << "\n";
                // Retry after delay
                if (m_running.load()) {
                    auto timer = std::make_shared<boost::asio::steady_timer>(m_io_context);
                    timer->expires_after(std::chrono::seconds(5));
                    timer->async_wait([this, timer](boost::system::error_code) {
                        doConnect();
                    });
                }
            }
        }
    );
}

void BoostTcpLink::doRead() {
    if (!m_running.load() || !isConnected()) {
        return;
    }
    
    m_socket.async_read_some(
        boost::asio::buffer(m_buffer),
        [this](boost::system::error_code ec, std::size_t bytes_read) {
            if (!ec) {
                m_receive_buffer.append(m_buffer.data(), bytes_read);
                
                // Process complete packets
                size_t pos = 0;
                while ((pos = m_receive_buffer.find('\n')) != std::string::npos) {
                    std::string packet_data = m_receive_buffer.substr(0, pos);
                    m_receive_buffer.erase(0, pos + 1);
                    
                    try {
                        Packet pkt = deserializePacket(packet_data);
                        onPacketReceived(pkt);
                    } catch (const std::exception& e) {
                        std::cerr << "[NETWORK] Packet deserialization error: " << e.what() << "\n";
                    }
                }
                
                doRead();
            } else {
                std::cerr << "[NETWORK] Read error: " << ec.message() << "\n";
                onDisconnected();
            }
        }
    );
}

void BoostTcpLink::onConnected() {
    m_connected.store(true);
    std::cout << "[NETWORK] Connected to peer\n";
}

void BoostTcpLink::onDisconnected() {
    if (m_connected.load()) {
        m_connected.store(false);
        std::cout << "[NETWORK] Disconnected from peer\n";
        
        // Try to reconnect if running
        if (m_running.load() && !m_peer_ip.empty()) {
            auto timer = std::make_shared<boost::asio::steady_timer>(m_io_context);
            timer->expires_after(std::chrono::seconds(5));
            timer->async_wait([this, timer](boost::system::error_code) {
                doConnect();
            });
        }
    }
}

void BoostTcpLink::onPacketReceived(const Packet& pkt) {
    m_packets_received.fetch_add(1, std::memory_order_relaxed);
    
    if (m_handler) {
        m_handler(pkt);
    }
}

std::string BoostTcpLink::serializePacket(const Packet& pkt) {
    std::ostringstream oss;
    oss << pkt.srcMac() << "|" << pkt.dstMac() << "|"
        << pkt.srcIp() << "|" << pkt.dstIp() << "|"
        << pkt.ingressPort() << "\n";
    return oss.str();
}

Packet BoostTcpLink::deserializePacket(const std::string& data) {
    std::istringstream iss(data);
    std::string src_mac, dst_mac, src_ip, dst_ip;
    Types::PortId ingress_port;

    std::getline(iss, src_mac, '|');
    std::getline(iss, dst_mac, '|');
    std::getline(iss, src_ip, '|');
    std::getline(iss, dst_ip, '|');
    iss >> ingress_port;

    return Packet(src_mac, dst_mac, src_ip, dst_ip, ingress_port);
}

#endif

// FallbackTcpLink Implementation - DISABLED (Boost is now required)
/*
FallbackTcpLink::FallbackTcpLink(
    Types::Port listen_port,
    const std::string& peer_ip,
    Types::Port peer_port
) : m_listen_port(listen_port),
    m_peer_ip(peer_ip),
    m_peer_port(peer_port) {

    std::cout << "[NETWORK] Creating fallback TCP link (no Boost.Asio)\n";
    std::cout << "  Listen Port: " << listen_port << "\n";
    std::cout << "  Peer: " << peer_ip << ":" << peer_port << "\n";
}

FallbackTcpLink::~FallbackTcpLink() {
    stop();
}

void FallbackTcpLink::start() {
    if (m_running.load()) {
        return;
    }

    m_running.store(true);
    m_connected.store(true); // Simulate connection

    std::cout << "[NETWORK] Fallback TCP link started (simulated)\n";
}

void FallbackTcpLink::stop() {
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);
    m_connected.store(false);

    std::cout << "[NETWORK] Fallback TCP link stopped\n";
}

bool FallbackTcpLink::isConnected() const noexcept {
    return m_connected.load();
}

void FallbackTcpLink::send(const Packet& pkt) {
    if (!isConnected()) {
        return;
    }

    // Simulate sending
    m_packets_sent.fetch_add(1, std::memory_order_relaxed);

    std::cout << "[NETWORK] Fallback: Sent packet (simulated)\n";
}

void FallbackTcpLink::setPacketHandler(std::function<void(const Packet&)> handler) {
    m_handler = handler;
}

std::string FallbackTcpLink::getStats() {
    std::ostringstream oss;
    oss << "FallbackTcpLink Statistics:\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Connected: " << (m_connected.load() ? "Yes" : "No") << "\n"
         << "  Packets Sent: " << m_packets_sent.load() << "\n"
         << "  Packets Received: " << m_packets_received.load() << "\n";

    return oss.str();
}
*/

} // export namespace MiniSonic::Networking
