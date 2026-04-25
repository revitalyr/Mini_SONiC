module;

#include <system_error>
#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio.hpp>

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <nlohmann/json.hpp>

module MiniSonic.App;

import MiniSonic.Core.Types;

// Import our custom modules
import MiniSonic.DataPlane;
import MiniSonic.Networking;
import MiniSonic.SAI;
import MiniSonic.L2L3;
import MiniSonic.Core.Utils;
import MiniSonic.Core.Events;

namespace MiniSonic::Core {

// App Implementation
App::App(
    Types::PortId listen_port,
    const std::string& peer_ip,
    Types::PortId peer_port
) : m_listen_port(listen_port),
    m_peer_ip(peer_ip),
    m_peer_port(peer_port) {
    
    initialize();
}

void App::initialize() {
    std::cout << "[APP] Initializing Mini SONiC with modular architecture\n";
    
    // Initialize topology
    initializeTopology();
    
    // Initialize data plane
    initializeDataPlane();
    
    // Initialize networking
    initializeNetworking();
    
    // Subscribe to event bus for web visualization
    auto& event_bus = Events::getGlobalEventBus();
    event_bus.subscribe("packet", [this](const nlohmann::json& event) {
        // Forward packet events to visualization
        // This will be picked up by the event server
    });
    
    std::cout << "[APP] Initialized with:\n"
              << "  Listen Port: " << m_listen_port << "\n"
              << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n"
              << "  Queue Size: " << m_packet_queue->capacity() << "\n"
              << "  Batch Size: 32\n"
              << "  Hosts: " << m_topology.hosts.size() << "\n"
              << "  Switch Links: " << m_topology.links.size() << "\n";
}

void App::initializeTopology() {
    std::cout << "[APP] Initializing topology configuration\n";
    
    // Configure hosts H1 and H2
    m_topology.hosts = {
        {
            "H1",
            "Host 1",
            Types::macToUint64("00:11:22:33:44:55"),
            Types::ipToUint32("10.0.1.2"),
            1, // Connected to switch 1
            1  // Port 1
        },
        {
            "H2",
            "Host 2",
            Types::macToUint64("00:11:22:33:44:56"),
            Types::ipToUint32("10.0.3.7"),
            4, // Connected to switch 4
            1  // Port 1
        }
    };
    
    // Configure switch links (topology: 1<->2<->3<->4)
    m_topology.links = {
        {1, 2}, // Core <-> Aggregation
        {2, 3}, // Aggregation <-> Edge
        {3, 4}  // Edge <-> Access
    };
    
    std::cout << "[APP] Topology configured:\n";
    for (const auto& host : m_topology.hosts) {
        std::cout << "  Host: " << host.name << " (" << host.id 
                  << ") connected to switch " << host.connected_switch_id 
                  << " port " << host.connected_port << "\n";
    }
}

void App::setupHandler() {
    std::cout << "[APP] Setting up signal handlers\n";
    // Signal handler setup is platform-specific
}

void App::initializeDataPlane() {
    std::cout << "[APP] Initializing data plane components\n";
    
    // Initialize SAI
    m_sai = std::make_unique<SAI::SimulatedSai>();
    
    // Initialize pipeline
    m_pipeline = std::make_unique<DataPlane::Pipeline>(*m_sai);
    
    // Initialize packet queue
    m_packet_queue = std::make_unique<DataPlane::SPSCQueue<DataPlane::Packet>>(1024);
    
    // Initialize pipeline thread
    m_pipeline_thread = std::make_unique<DataPlane::PipelineThread>(
        *m_pipeline, *m_packet_queue, 32
    );
    
    std::cout << "[APP] Data plane components initialized\n";
}

void App::initializeNetworking() {
    std::cout << "[APP] Initializing networking components\n";
    
    // Only initialize networking if peer is specified (for distributed mode)
    if (!m_peer_ip.empty() && m_peer_port > 0) {
        m_network_link = Networking::NetworkProviderFactory::createTcpLink(
            m_listen_port, m_peer_ip, m_peer_port
        );
        std::cout << "[APP] Networking components initialized for distributed mode\n";
        std::cout << "[APP] Boost.Asio support: "
                  << (Networking::NetworkProviderFactory::hasBoostSupport() ? "Yes" : "No") << "\n";
    } else {
        std::cout << "[APP] Running in standalone mode (no networking)\n";
    }
}

void App::run() {
    m_running.store(true);

    std::cout << "[APP] Starting Mini SONiC\n";

    try {
        // Start pipeline thread
        m_pipeline_thread->start();

        // Start network link if in distributed mode
        if (m_network_link) {
            m_network_link->start();
        }

        // Start packet generator
        startPacketGenerator();

        std::cout << "[APP] All components started\n";

        // Main thread - wait for shutdown
        while (m_running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Print periodic stats
            static Types::Count stats_counter = 0;
            if (++stats_counter % 30 == 0) { // Every 30 seconds
                std::cout << "\n" << getStats() << "\n";
                std::cout << Utils::Metrics::instance().getSummary() << "\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[APP] Fatal error: " << e.what() << "\n";
        stop();
    }
}

void App::stop() {
    if (!m_running.load()) {
        return;
    }
    
    std::cout << "[APP] Shutting down...\n";
    
    m_running.store(false);
    
    // Stop components in reverse order
    if (m_network_link) {
        m_network_link->stop();
    }
    
    if (m_pipeline_thread) {
        m_pipeline_thread->stop();
    }
    
    if (m_generator_thread && m_generator_thread->joinable()) {
        m_generator_thread->join();
    }
    
    std::cout << "[APP] Shutdown complete\n";
}

bool App::isRunning() const noexcept {
    return m_running.load();
}

std::string App::getStats() const {
    std::ostringstream oss;
    oss << "=== Mini SONiC App Stats ===\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Listen Port: " << m_listen_port << "\n"
         << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n";
    
    if (m_network_link) {
        oss << m_network_link->getStats();
    }
    
    if (m_pipeline_thread) {
        oss << m_pipeline_thread->getStats();
    }
    
    if (m_packet_queue) {
        oss << "  Queue Usage: " << m_packet_queue->size() 
             << "/" << m_packet_queue->capacity() << "\n";
    }
    
    if (m_sai) {
        oss << m_sai->getStats();
    }
    
    return oss.str();
}

void App::startPacketGenerator() {
    m_generator_thread = std::make_unique<std::thread>([this]() {
        std::cout << "[APP] Starting packet generator thread\n";
        
        Types::Count packet_counter = 0;
        while (m_running.load()) {
            generateTestPackets();
            ++packet_counter;
            
            // Generate packets at ~20 pps (50ms interval)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Update metrics
            if (packet_counter % 100 == 0) {
                Utils::Metrics::instance().inc("app_generator_cycles");
            }
        }
        
        std::cout << "[APP] Packet generator stopped\n";
    });
}

void App::generateTestPackets() {
    static Types::Count packet_id = 0;
    static int direction = 0;

    const Host& src_host = m_topology.hosts[direction];
    const Host& dst_host = m_topology.hosts[1 - direction];

    DataPlane::Packet pkt(
        src_host.mac,
        dst_host.mac,
        src_host.ip,
        dst_host.ip,
        src_host.connected_port,
        ++packet_id
    );

    pkt.updateTimestamp();

    // Emit event for web visualization
    auto& event_bus = Events::getGlobalEventBus();
    nlohmann::json event;
    event["type"] = "packet_generated";
    event["src_host"] = src_host.id;
    event["dst_host"] = dst_host.id;
    event["src_switch"] = src_host.connected_switch_id;
    event["dst_switch"] = dst_host.connected_switch_id;
    event["packet_id"] = packet_id;
    event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    event_bus.publishJson(event);

    if (m_packet_queue->push(std::move(pkt))) {
        Utils::Metrics::instance().inc("app_packets_generated");
        std::cout << "[APP] Packet " << packet_id << " generated: "
                  << src_host.id << " -> " << dst_host.id << "\n";
    }

    direction = 1 - direction;
}

// EventLoop Implementation
EventLoop::EventLoop(DataPlane::Pipeline& pipeline) : m_pipeline(pipeline) {
}

EventLoop::~EventLoop() = default;

void EventLoop::run() {
    m_running.store(true);
    
    std::cout << "[EVENT_LOOP] Starting event loop\n";
    
    while (m_running.load()) {
        generateTestPackets();
        processPackets();
        
        // Brief sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[EVENT_LOOP] Event loop stopped\n";
}

void EventLoop::stop() {
    m_running.store(false);
}

bool EventLoop::isRunning() const noexcept {
    return m_running.load();
}

void EventLoop::generateTestPackets() {
    // Generate test packets for local processing
    static Types::Count counter = 0;
    
    std::string src_ip = "10.1.1." + std::to_string(100 + counter % 100);
    std::string dst_ip = "10.1.1." + std::to_string(1 + counter % 100);
    
    DataPlane::Packet pkt(
        MiniSonic::Types::macToUint64("aa:bb:cc:dd:ee:ff"),
        MiniSonic::Types::macToUint64("ff:ee:dd:cc:bb:aa"),
        MiniSonic::Types::ipToUint32(src_ip),
        MiniSonic::Types::ipToUint32(dst_ip),
        counter % 24 + 1
    );
    
    pkt.updateTimestamp();
    m_pipeline.process(pkt);
    
    ++counter;
}

void EventLoop::processPackets() {
    // This would handle any pending packets
    // For now, we just update metrics
    Utils::Metrics::instance().inc("event_loop_cycles");
}

} // namespace MiniSonic::App
