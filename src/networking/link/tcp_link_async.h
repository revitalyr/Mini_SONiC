#pragma once

#include "dataplane/packet.h" // Packet is now in MiniSonic.L2L3, but this header is for Networking module
#include "core/utils/metrics.hpp" // Corrected include path
#include "core/common/types.hpp" // Corrected include path

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>
#include <memory>
#include <string>
#include <functional>
#include <array>
#include <atomic>

namespace MiniSonic::Link {

/**
 * @brief Asynchronous TCP link for inter-switch communication
 * 
 * High-performance async TCP implementation using Boost.Asio.
 * Supports both server (accepting connections) and client (connecting to peers) modes.
 * Uses strand for thread safety and async operations for non-blocking I/O.
 */
class TcpLinkAsync : public std::enable_shared_from_this<TcpLinkAsync> {
public:
    using PacketHandler = std::function<void(DataPlane::Packet)>;
    using ErrorCode = boost::system::error_code;
    using Tcp = boost::asio::ip::tcp;

    /**
     * @brief Construct async TCP link
     * @param io_context Boost.Asio IO context
     * @param listen_port Port to listen on (server mode)
     * @param peer_ip Peer IP to connect to (client mode)
     * @param peer_port Peer port to connect to (client mode)
     */
    TcpLinkAsync(
        boost::asio::io_context& io_context,
        Types::Port listen_port,
        const Types::String& peer_ip = "",
        Types::Port peer_port = 0
    );

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~TcpLinkAsync();

    /**
     * @brief Start the link (begin accepting/connecting)
     */
    void start();

    /**
     * @brief Stop the link gracefully
     */
    void stop();

    /**
     * @brief Send packet asynchronously
     * @param pkt Packet to send
     */
    void send(const DataPlane::Packet& pkt);

    /**
     * @brief Set packet handler callback
     * @param handler Function to call when packet is received
     */
    void setHandler(PacketHandler handler);

    /**
     * @brief Check if link is connected
     * @return true if connected to peer
     */
    [[nodiscard]] bool isConnected() const noexcept;

    /**
     * @brief Get connection statistics
     */
    [[nodiscard]] Types::String getStats() const;

private:
    /**
     * @brief Start accepting incoming connections
     */
    void doAccept();

    /**
     * @brief Start connecting to peer
     */
    void doConnect();

    /**
     * @brief Start reading from socket
     */
    void doRead();

    /**
     * @brief Handle connection established
     */
    void onConnected();

    /**
     * @brief Handle connection lost
     */
    void onDisconnected();

    /**
     * @brief Handle packet received
     */
    void onPacketReceived(const DataPlane::Packet& pkt);

    /**
     * @brief Serialize packet to string
     */
    [[nodiscard]] static Types::String serializePacket(const DataPlane::Packet& pkt);

    /**
     * @brief Deserialize string to packet
     */
    [[nodiscard]] static DataPlane::Packet deserializePacket(const Types::String& data);

    // Core components
    boost::asio::io_context& m_io_context;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::ip::tcp::endpoint m_peer_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

    // Configuration
    const Types::Port m_listen_port;
    const Types::String m_peer_ip;
    const Types::Port m_peer_port;

    // State
    Types::AtomicBool m_running{false};
    Types::AtomicBool m_connected{false};
    PacketHandler m_handler;

    // Buffer for incoming data
    std::array<char, 4096> m_buffer;
    Types::String m_receive_buffer;

    // Statistics
    Types::Atomic<Types::Count> m_packets_sent{0};
    Types::Atomic<Types::Count> m_packets_received{0};
    Types::Atomic<Types::Count> m_bytes_sent{0};
    Types::Atomic<Types::Count> m_bytes_received{0};
};

} // namespace MiniSonic::Link
