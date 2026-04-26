module;

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

export module MiniSonic.App;

// Import our custom modules
import MiniSonic.DataPlane;
import MiniSonic.Networking;
import MiniSonic.SAI;
import MiniSonic.L2L3;
import MiniSonic.Core.Utils;
import MiniSonic.Core.Types;

export namespace MiniSonic::Core {

/**
 * @brief Network host configuration.
 *
 * Represents a host in the network topology with address information and connection details.
 */
export struct Host {
    std::string id;                     ///< Host identifier
    std::string name;                   ///< Host name
    Types::MacAddress mac;              ///< MAC address
    Types::IpAddress ip;                ///< IP address
    int x, y;                          ///< Coordinates for visualization
    std::string connected_switch;      ///< Connected switch identifier
    int connected_port;                ///< Connected port number
};

/**
 * @brief Network switch configuration.
 *
 * Represents a switch in the network topology with port information.
 */
export struct Switch {
    std::string id;                                        ///< Switch identifier
    std::string name;                                      ///< Switch name
    Types::MacAddress mac;                                 ///< MAC address
    int x, y;                                              ///< Coordinates for visualization
    std::vector<std::pair<int, std::string>> ports;       ///< Port configuration (port_id -> connected_to)
};

/**
 * @brief Network link configuration.
 *
 * Represents a bidirectional link between two network nodes.
 */
export struct Link {
    std::string source;     ///< Source node identifier
    std::string target;     ///< Target node identifier
};

/**
 * @brief Complete network topology configuration.
 *
 * Contains all hosts, switches, and links defining the network topology.
 */
export struct TopologyConfig {
    std::vector<Host> hosts;       ///< List of hosts
    std::vector<Switch> switches;  ///< List of switches
    std::vector<Link> links;       ///< List of links
};

/**
 * @brief Main application class
 *
 * Orchestrates system components and provides the primary entry point.
 * Manages topology configuration, data plane initialization, and packet generation.
 */
export class App {
public:
    /**
     * @brief Construct application with specified network parameters.
     *
     * @param listen_port Port to listen on for incoming connections
     * @param peer_ip Peer IP address for distributed mode (empty for standalone)
     * @param peer_port Peer port for distributed mode (0 for standalone)
     */
    App(
        Types::PortId listen_port,
        const std::string& peer_ip,
        Types::PortId peer_port
    );
    
    ~App() = default;

    // Rule of five
    App(const App& other) = delete;
    App& operator=(const App& other) = delete;
    App(App&& other) noexcept = delete;
    App& operator=(App&& other) = delete;

    /**
     * @brief Run the application.
     *
     * Starts all components and enters main loop.
     */
    void run();

    /**
     * @brief Stop the application.
     *
     * Gracefully shuts down all components.
     */
    void stop();

    /**
     * @brief Check if application is running.
     *
     * @return true if running, false otherwise
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get application statistics.
     *
     * @return String containing application statistics
     */
    [[nodiscard]] std::string getStats() const;

    /**
     * @brief Setup signal handlers.
     *
     * Platform-specific signal handler configuration.
     */
    void setupHandler();

private:
    /**
     * @brief Initialize all components.
     */
    void initialize();

    /**
     * @brief Start packet generator thread.
     *
     * Generates test packets from topology hosts at regular intervals.
     */
    void startPacketGenerator();

    /**
     * @brief Initialize networking components.
     *
     * Only initializes networking if peer parameters are specified for distributed mode.
     */
    void initializeNetworking();

    /**
     * @brief Initialize data plane components.
     *
     * Sets up SAI, pipeline, packet queue, and pipeline thread.
     */
    void initializeDataPlane();

    /**
     * @brief Initialize topology configuration.
     *
     * Loads topology from configuration file or uses default values.
     */
    void initializeTopology();

    /**
     * @brief Find path from source to destination using BFS.
     *
     * @param src Source node identifier
     * @param dst Destination node identifier
     * @return Vector of node identifiers representing the path
     */
    std::vector<std::string> findPath(const std::string& src, const std::string& dst);

    /**
     * @brief Route packet through switches sequentially.
     *
     * Emits hop events for each node in the path and queues packet for pipeline processing at final destination.
     *
     * @param pkt Packet to route
     * @param path Vector of node identifiers representing the route
     */
    void routePacketSequentially(const DataPlane::Packet& pkt, const std::vector<std::string>& path);

    /**
     * @brief Generate test packets from topology hosts.
     *
     * Alternates between hosts to generate bidirectional traffic.
     */
    void generateTestPackets();

    // Core components
    std::unique_ptr<SAI::SaiInterface> m_sai;
    std::unique_ptr<DataPlane::Pipeline> m_pipeline;
    std::unique_ptr<DataPlane::SPSCQueue<DataPlane::Packet>> m_packet_queue;
    std::unique_ptr<DataPlane::PipelineThread> m_pipeline_thread;
    std::unique_ptr<Networking::INetworkProvider> m_network_link;

    // Configuration
    const Types::PortId m_listen_port;
    const std::string m_peer_ip;
    const Types::PortId m_peer_port;
    TopologyConfig m_topology;

    // State
    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_generator_thread;
};

/**
 * @brief Event loop for packet processing
 * 
 * Provides asynchronous packet generation and processing.
 */
export class EventLoop {
public:
    explicit EventLoop(DataPlane::Pipeline& pipeline);
    ~EventLoop();

    // Rule of five
    EventLoop(const EventLoop& other) = delete;
    EventLoop& operator=(const EventLoop& other) = delete;
    EventLoop(EventLoop&& other) noexcept = delete;
    EventLoop& operator=(EventLoop&& other) noexcept = delete;

    void run();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

private:
    /**
     * @brief Generate test packets
     */
    void generateTestPackets();

    /**
     * @brief Process packets
     */
    void processPackets();

    DataPlane::Pipeline& m_pipeline;
    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_thread;
};

} // export namespace MiniSonic::Core
