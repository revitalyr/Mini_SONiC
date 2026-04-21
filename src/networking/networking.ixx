module;

// MSVC: use traditional includes in global module fragment
#include <boost/system/error_code.hpp>
#include <memory>
#include <string>
#include <functional>
#include <array>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdint>
#include <vector>
#include <map>

// Boost.Asio includes
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

export module MiniSonic.Networking;

// Import Utils module for Types namespace
import MiniSonic.Core.Utils;

// Re-export common standard library types
using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::thread;
using std::atomic;
using std::chrono::milliseconds;

export namespace MiniSonic::Networking {

// Forward declarations
class Packet;
class TcpLinkAsync;

/**
 * @brief Abstract interface for network operations
 * 
 * This interface provides a clean abstraction over networking
 * functionality using Boost.Asio.
 */
class INetworkProvider {
public:
    virtual ~INetworkProvider() = default;
    
    // Pure virtual methods for network operations
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const noexcept = 0;
    virtual void send(const Packet& pkt) = 0;
    virtual void setPacketHandler(std::function<void(const Packet&)> handler) = 0;
    virtual std::string getStats() const = 0;
};

/**
 * @brief Factory for creating network providers
 * 
 * Creates network providers using Boost.Asio.
 */
class NetworkProviderFactory {
public:
    static std::unique_ptr<INetworkProvider> createTcpLink(
        Types::PortId listen_port,
        const std::string& peer_ip,
        Types::PortId peer_port
    );
    
    static bool hasBoostSupport() noexcept {
        return true; // Boost is now required
    }
};

/**
 * @brief Boost.Asio implementation of network provider
 * 
 * Provides high-performance async networking using Boost.Asio.
 */
class BoostTcpLink : public INetworkProvider {
public:
    using PacketHandler = std::function<void(const Packet&)>;
    
    BoostTcpLink(
        boost::asio::io_context& io_context,
        Types::PortId listen_port,
        const std::string& peer_ip,
        Types::PortId peer_port
    );
    
    ~BoostTcpLink() override;
    
    // INetworkProvider interface
    void start() override;
    void stop() override;
    bool isConnected() const noexcept override;
    void send(const Packet& pkt) override;
    void setPacketHandler(PacketHandler handler) override;
    std::string getStats() const override;

private:
    // Boost.Asio specific implementation
    void doAccept();
    void doConnect();
    void doRead();
    void onConnected();
    void onDisconnected();
    void onPacketReceived(const Packet& pkt);
    
    static std::string serializePacket(const Packet& pkt);
    static Packet deserializePacket(const std::string& data);
    
    boost::asio::io_context& m_io_context;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::endpoint m_peer_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

    Types::PortId m_listen_port;
    std::string m_peer_ip;
    Types::PortId m_peer_port;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    PacketHandler m_handler;
    
    std::array<char, 4096> m_buffer;
    std::string m_receive_buffer;
    
    // Statistics
    std::atomic<Types::Count> m_packets_sent{0};
    std::atomic<Types::Count> m_packets_received{0};
    std::atomic<Types::Count> m_bytes_sent{0};
    std::atomic<Types::Count> m_bytes_received{0};
};

// Implementation of factory method
inline std::unique_ptr<INetworkProvider> NetworkProviderFactory::createTcpLink(
    Types::PortId listen_port,
    const std::string& peer_ip,
    Types::PortId peer_port
) {
    static boost::asio::io_context io_context;
    return std::make_unique<BoostTcpLink>(io_context, listen_port, peer_ip, peer_port);
}

} // export namespace MiniSonic::Networking
