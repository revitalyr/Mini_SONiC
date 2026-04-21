#include <boost/system/error_code.hpp>
#include "core/app.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>

namespace MiniSonic::Core {

App::App(
    Types::Port listen_port,
    const Types::String& peer_ip,
    Types::Port peer_port
) : m_listen_port(listen_port),
    m_peer_ip(peer_ip),
    m_peer_port(peer_port) {
    
    initialize();
}

void App::initialize() {
    std::cout << "[APP] Initializing Mini SONiC with async architecture" << std::endl;
    
    // Initialize Boost.Asio IO context
    std::cout << "[APP] Creating IO context" << std::endl;
    m_io_context = std::make_unique<boost::asio::io_context>();
    
    // Initialize core components
    std::cout << "[APP] Creating SAI\n";
    // m_sai = std::make_unique<Sai::SimulatedSai>();
    
    std::cout << "[APP] Creating pipeline\n";
    // m_pipeline = std::make_unique<DataPlane::Pipeline>(*m_sai);
    
    std::cout << "[APP] Creating packet queue\n";
    // m_packet_queue = std::make_unique<Utils::SPSCQueue<DataPlane::Packet>>(1024);
    
    std::cout << "[APP] Creating pipeline thread\n";
    // m_pipeline_thread = std::make_unique<DataPlane::PipelineThread>(
    //     *m_pipeline, *m_packet_queue, 32
    // );
    
    // Initialize async TCP link
    std::cout << "[APP] Creating TCP link\n";
    // m_tcp_link = std::make_shared<Link::TcpLinkAsync>(
    //     *m_io_context,
    //     m_listen_port,
    //     m_peer_ip,
    //     m_peer_port
    // );
    
    // Initialize event loop (for local packet generation)
    std::cout << "[APP] Creating event loop\n";
    // m_event_loop = std::make_unique<EventLoop>(*m_pipeline);
    
    std::cout << "[APP] Initialized with:\n"
              << "  Listen Port: " << m_listen_port << "\n"
              << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n"
              << "  Queue Size: " << m_packet_queue->capacity() << "\n"
              << "  Batch Size: 32\n";
}

void App::setupHandler() {
    // Set packet handler for incoming TCP packets
    m_tcp_link->setHandler([this](const DataPlane::Packet& pkt) {
        // Add timestamp and queue for processing
        auto timestamped_pkt = pkt;
        timestamped_pkt.updateTimestamp();
        
        if (m_packet_queue->push(std::move(timestamped_pkt))) {
            Utils::Metrics::instance().inc("app_packets_queued");
        } else {
            Utils::Metrics::instance().inc("app_queue_full");
        }
    });
}

void App::run() {
    m_running.store(true);
    
    std::cout << "[APP] Starting Mini SONiC\n";
    
    try {
        // Start pipeline thread
        m_pipeline_thread->start();
        
        // Start async TCP link
        // m_tcp_link->start();
        
        // Start Asio IO context in separate thread
        // m_asio_thread = std::make_unique<std::thread>([io_context = m_io_context.get()]() {
        //     try {
        //         std::cout << "[APP] Starting Asio IO thread\n";
        //         io_context->run();
        //     } catch (const std::exception& e) {
        //         std::cerr << "[APP] Asio thread error: " << e.what() << "\n";
        //     }
        // });
        
        std::cout << "[APP] Skipping Asio thread for testing\n";
        
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
    if (m_tcp_link) {
        m_tcp_link->stop();
    }
    
    if (m_io_context) {
        m_io_context->stop();
    }
    
    // if (m_asio_thread && m_asio_thread->joinable()) {
    //     m_asio_thread->join();
    // }
    
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

Types::String App::getStats() const {
    std::ostringstream oss;
    oss << "=== Mini SONiC App Stats ===\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Listen Port: " << m_listen_port << "\n"
         << "  Peer: " << m_peer_ip << ":" << m_peer_port << "\n";
    
    if (m_tcp_link) {
        oss << m_tcp_link->getStats();
    }
    
    if (m_pipeline_thread) {
        oss << m_pipeline_thread->getStats();
    }
    
    if (m_packet_queue) {
        oss << "  Queue Usage: " << m_packet_queue->size() 
             << "/" << m_packet_queue->capacity() << "\n";
    }
    
    return oss.str();
}

void App::startPacketGenerator() {
    m_generator_thread = std::make_unique<std::thread>([running = &m_running]() {
        std::cout << "[APP] Starting packet generator thread\n";
        
        Types::Count packet_counter = 0;
        while (running->load()) {
            // For now, just sleep - actual packet generation would be implemented here
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Update metrics
            if (packet_counter % 100 == 0) {
                Utils::Metrics::instance().inc("app_generator_cycles");
            }
            ++packet_counter;
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
                "aa:bb:cc:dd:ee:01",
                "bb:cc:dd:ee:02",
                "10.0.0.1",
                "10.0.0.2",
                1
            );
            break;
            
        case 1: // L3 test packet
            pkt = DataPlane::Packet(
                "cc:dd:ee:ff:03",
                "dd:ee:ff:aa:04",
                "10.1.1.100",
                "10.1.1.42",
                2
            );
            break;
            
        case 2: // Broadcast packet
            pkt = DataPlane::Packet(
                "ee:ff:aa:bb:05",
                "ff:ff:ff:ff:ff",
                "10.2.2.1",
                "255.255.255.255",
                3
            );
            break;
            
        case 3: // Cross-subnet packet
            pkt = DataPlane::Packet(
                "ff:aa:bb:cc:06",
                "aa:bb:cc:dd:07",
                "192.168.100.1",
                "192.168.200.1",
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

} // namespace MiniSonic::Core
