#ifndef VISUALIZER_CORE_STD_H
#define VISUALIZER_CORE_STD_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <cstdint>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;

// Semantic type aliases
using PortNo = std::uint16_t;
using SpeedMultiplier = int;
using PacketId = std::string;
using NodeId = std::string;
using HostId = std::string;
using SwitchId = std::string;
using IpString = std::string;

/**
 * @brief Standard C++ visualizer core implementation.
 *
 * Network communication via Boost.Asio. Event delivery via std::function callbacks.
 * No Qt dependencies. Thread-safe for single io_context thread.
 */
class VisualizerCoreStd : public std::enable_shared_from_this<VisualizerCoreStd> {
public:
    /**
     * @brief Constructs visualizer core with io_context.
     * @param io_context Boost.Asio io_context for async operations.
     */
    explicit VisualizerCoreStd(boost::asio::io_context& io_context);

    /**
     * @brief Connects to event server.
     * @param host Server hostname or IP address.
     * @param port Server port number.
     */
    void connect(const std::string& host, PortNo port);

    /** @brief Disconnects from server. Closes socket. */
    void disconnect();

    /**
     * @brief Sends speed control command.
     * @param speed Animation speed multiplier.
     */
    void sendSpeedControl(SpeedMultiplier speed);

    /** @brief Requests topology from server. */
    void requestTopology();

    /** @brief Connection status callback. Parameters: connected, status message. */
    std::function<void(bool connected, const std::string& status)> onConnectionStatus;

    /** @brief Log message callback. Parameters: event type, details. */
    std::function<void(const std::string& type, const std::string& details)> onLogMessage;

    /** @brief Topology loaded callback. Parameter: topology JSON. */
    std::function<void(const json& topology)> onTopologyLoaded;

    /** @brief Packet animation request callback. Parameters: packet ID, from node, to node, details. */
    std::function<void(const std::string& pId, const std::string& fromId, const std::string& toId, const json& details)> requestPacketAnimation;

    /** @brief Node activation callback. Parameters: node ID, is switch. */
    std::function<void(const std::string& nodeId, bool isSwitch)> nodeActivated;

    /** @brief Link activation callback. Parameters: source ID, target ID. */
    std::function<void(const std::string& sourceId, const std::string& targetId)> linkActivated;

    /** @brief Port state change callback. Parameters: switch ID, port, state. */
    std::function<void(const std::string& swId, const std::string& port, const std::string& state)> portStateChanged;

private:
    void do_read();
    void handle_line(const std::string& line);
    
    // Protocol Handlers
    void handleTopology(const json& obj);
    void handlePacketGenerated(const json& obj);
    void handlePacketEnteredSwitch(const json& obj);
    void handlePacketExitedSwitch(const json& obj);

    void write(const json& obj);
    std::string get_packet_id(const json& obj, const std::string& key);

    tcp::socket m_socket;
    boost::asio::streambuf m_buffer;
    
    std::map<std::string, std::string> m_hostIpMap;        // IP -> HostID
    std::map<std::string, std::string> m_packetLastNode;   // PacketID -> Last Node ID
    std::map<std::string, std::pair<std::string, std::string>> m_packetInfo; // PacketID -> {srcIp, dstIp}
};

#endif // VISUALIZER_CORE_STD_H