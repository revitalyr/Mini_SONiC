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
#include "core/common/types.hpp"

module MiniSonic.App;

// Import our custom modules
import MiniSonic.DataPlane;
import MiniSonic.Networking;
import MiniSonic.SAI;
import MiniSonic.L2L3;
import MiniSonic.Core.Utils;

namespace MiniSonic::App {

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
    
    initializeDataPlane();
    initializeNetworking();
    
    std::cout << "[APP] Initialized with:\n"
              << "  Listen Port: " << m_listen_port << "\n"
              << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n"
              << "  Queue Size: " << m_packet_queue->capacity() << "\n"
              << "  Batch Size: 32\n";
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
    
    // Create network link using factory
    m_network_link = Networking::NetworkProviderFactory::createTcpLink(
        m_listen_port, m_peer_ip, m_peer_port
    );
    
    // Set packet handler
    // TODO: Type mismatch - Networking::Packet vs DataPlane::Packet needs resolution
    // m_network_link->setPacketHandler([this](const DataPlane::Packet& pkt) {
    //     // Add timestamp and queue for processing
    //     auto timestamped_pkt = pkt;
    //     timestamped_pkt.updateTimestamp();
    //
    //     if (m_packet_queue->push(std::move(timestamped_pkt))) {
    //         Utils::Metrics::instance().inc("app_packets_queued");
    //     } else {
    //         Utils::Metrics::instance().inc("app_queue_full");
    //     }
    // });
    
    std::cout << "[APP] Networking components initialized\n";
    std::cout << "[APP] Boost.Asio support: " 
              << (Networking::NetworkProviderFactory::hasBoostSupport() ? "Yes" : "No") << "\n";
}

void App::run() {
    m_running.store(true);
    
    std::cout << "[APP] Starting Mini SONiC\n";
    
    try {
        // Start pipeline thread
        m_pipeline_thread->start();
        
        // Start network link
        m_network_link->start();
        
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
    // Generate packets with different patterns
    const Types::Count counter = Utils::Metrics::instance().getCounter("app_packets_generated");
    
    DataPlane::Packet pkt;
    
    switch (counter % 4) {
        case 0: // L2 test packet
            pkt = DataPlane::Packet(
                MiniSonic::Types::macToUint64("aa:bb:cc:dd:ee:01"),
                MiniSonic::Types::macToUint64("bb:cc:dd:ee:02"),
                MiniSonic::Types::ipToUint32("10.0.0.1"),
                MiniSonic::Types::ipToUint32("10.0.0.2"),
                1
            );
            break;
            
        case 1: // Local subnet packet
            pkt = DataPlane::Packet(
                MiniSonic::Types::macToUint64("cc:dd:ee:ff:03"),
                MiniSonic::Types::macToUint64("dd:ee:ff:aa:04"),
                MiniSonic::Types::ipToUint32("10.1.1.100"),
                MiniSonic::Types::ipToUint32("10.1.1.42"),
                2
            );
            break;
            
        case 2: // Broadcast packet
            pkt = DataPlane::Packet(
                MiniSonic::Types::macToUint64("ee:ff:aa:bb:05"),
                MiniSonic::Types::macToUint64("ff:ff:ff:ff:ff:ff"),
                MiniSonic::Types::ipToUint32("10.2.2.1"),
                MiniSonic::Types::ipToUint32("255.255.255.255"),
                3
            );
            break;

        case 3: // Cross-subnet packet
            pkt = DataPlane::Packet(
                MiniSonic::Types::macToUint64("ff:aa:bb:cc:06"),
                MiniSonic::Types::macToUint64("aa:bb:cc:dd:07"),
                MiniSonic::Types::ipToUint32("192.168.100.1"),
                MiniSonic::Types::ipToUint32("192.168.200.1"),
                4
            );
            break;
    }
    
    pkt.updateTimestamp();
    
    if (m_packet_queue->push(std::move(pkt))) {
        Utils::Metrics::instance().inc("app_packets_generated");
        Utils::Metrics::instance().inc("app_packets_queued");
    } else {
        Utils::Metrics::instance().inc("app_queue_full");
    }
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
