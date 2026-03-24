module;

// Import standard library modules
import <memory>;
import <string>;
import <functional>;
import <array>;
import <atomic>;
import <thread>;
import <chrono>;
import <iostream>;

export module MiniSonic.Networking;

// Conditional compilation for Boost
#ifdef BOOST_FOUND
#define HAS_BOOST_ASIO
#endif

#ifdef HAS_BOOST_ASIO
// Import Boost.Asio components
import <boost/asio.hpp>;
import <boost/system/error_code.hpp>;
#endif

export namespace MiniSonic::Networking {

// Forward declarations
class Packet;
class TcpLinkAsync;

/**
 * @brief Abstract interface for network operations
 * 
 * This interface provides a clean abstraction over networking
 * functionality, allowing both Boost.Asio and fallback implementations.
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
 * Creates appropriate network provider based on available dependencies.
 */
class NetworkProviderFactory {
public:
    static std::unique_ptr<INetworkProvider> createTcpLink(
        Types::Port listen_port,
        const std::string& peer_ip,
        Types::Port peer_port
    );
    
    static bool hasBoostSupport() noexcept {
#ifdef HAS_BOOST_ASIO
        return true;
#else
        return false;
#endif
    }
};

#ifdef HAS_BOOST_ASIO

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
        Types::Port listen_port,
        const std::string& peer_ip,
        Types::Port peer_port
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
    
    Types::Port m_listen_port;
    std::string m_peer_ip;
    Types::Port m_peer_port;
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

#endif

/**
 * @brief Fallback implementation for systems without Boost
 * 
 * Provides basic networking functionality using standard library.
 */
class FallbackTcpLink : public INetworkProvider {
public:
    FallbackTcpLink(
        Types::Port listen_port,
        const std::string& peer_ip,
        Types::Port peer_port
    );
    
    ~FallbackTcpLink() override;
    
    // INetworkProvider interface
    void start() override;
    void stop() override;
    bool isConnected() const noexcept override;
    void send(const Packet& pkt) override;
    void setPacketHandler(std::function<void(const Packet&)> handler) override;
    std::string getStats() const override;

private:
    Types::Port m_listen_port;
    std::string m_peer_ip;
    Types::Port m_peer_port;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::function<void(const Packet&)> m_handler;
    
    // Basic statistics
    std::atomic<Types::Count> m_packets_sent{0};
    std::atomic<Types::Count> m_packets_received{0};
};

// Implementation of factory method
inline std::unique_ptr<INetworkProvider> NetworkProviderFactory::createTcpLink(
    Types::Port listen_port,
    const std::string& peer_ip,
    Types::Port peer_port
) {
#ifdef HAS_BOOST_ASIO
    static boost::asio::io_context io_context;
    return std::make_unique<BoostTcpLink>(io_context, listen_port, peer_ip, peer_port);
#else
    std::cout << "[NETWORK] Boost.Asio not available, using fallback implementation\n";
    return std::make_unique<FallbackTcpLink>(listen_port, peer_ip, peer_port);
#endif
}

} // export namespace MiniSonic::Networking
