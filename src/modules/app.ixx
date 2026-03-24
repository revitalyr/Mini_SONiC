module;

// Import standard library modules
import <memory>;
import <string>;
import <vector>;
import <atomic>;
import <thread>;
import <chrono>;
import <iostream>;
import <sstream>;

// Import our custom modules
import MiniSonic.DataPlane;
import MiniSonic.Networking;
import MiniSonic.SAI;
import MiniSonic.L2L3;
import MiniSonic.Utils;

export module MiniSonic.App;

export namespace MiniSonic::Core {

/**
 * @brief Main application class
 * 
 * Orchestrates all components and provides the main entry point.
 */
export class App {
public:
    /**
     * @brief Construct application with modular architecture
     * @param listen_port Port for this switch instance
     * @param peer_ip Peer switch IP (optional)
     * @param peer_port Peer switch port (optional)
     */
    App(
        Types::Port listen_port = 9000,
        const std::string& peer_ip = "",
        Types::Port peer_port = 0
    );
    
    ~App() = default;

    // Rule of five
    App(const App& other) = delete;
    App& operator=(const App& other) = delete;
    App(App&& other) noexcept = delete;
    App& operator=(App&& other) noexcept = delete;

    void run();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get application statistics
     */
    [[nodiscard]] std::string getStats() const;

private:
    /**
     * @brief Initialize all components
     */
    void initialize();

    /**
     * @brief Start packet generator thread
     */
    void startPacketGenerator();

    /**
     * @brief Generate test packets
     */
    void generateTestPackets();

    /**
     * @brief Initialize networking components
     */
    void initializeNetworking();

    /**
     * @brief Initialize data plane components
     */
    void initializeDataPlane();

    // Core components
    std::unique_ptr<SAI::SaiInterface> m_sai;
    std::unique_ptr<DataPlane::Pipeline> m_pipeline;
    std::unique_ptr<DataPlane::SPSCQueue<DataPlane::Packet>> m_packet_queue;
    std::unique_ptr<DataPlane::PipelineThread> m_pipeline_thread;
    std::unique_ptr<Networking::INetworkProvider> m_network_link;

    // Configuration
    const Types::Port m_listen_port;
    const std::string m_peer_ip;
    const Types::Port m_peer_port;

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
    ~EventLoop() = default;

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
