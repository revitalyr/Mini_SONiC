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
#include <fstream>
#include <queue>
#include <boost/asio.hpp>
#include "../visualization/event_server.h"

module MiniSonic.App;

import MiniSonic.Core.Types;
import MiniSonic.Core.Constants;

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
    initializeTopology();
    initializeDataPlane();
    initializeNetworking();
    Visualization::startEventServer(8080);

    auto& event_bus = Events::getGlobalEventBus();
    event_bus.subscribe("packet", [this](const nlohmann::json& event) {});

    std::cout << Constants::LOG_PREFIX_APP << " " << Constants::INFO_INITIALIZED << ": "
              << m_topology.hosts.size() << " hosts, "
              << m_topology.links.size() << " switch links\n";
}

void App::initializeTopology() {
    std::ifstream config_file(Constants::TOPOLOGY_CONFIG_PATH);
    if (!config_file.is_open()) {
        std::cerr << Constants::LOG_PREFIX_APP << " " << Constants::ERR_TOPOLOGY_CONFIG_OPEN << "\n";
        // Fallback to hardcoded topology
        m_topology.hosts = {
            {Constants::DEFAULT_HOST_H1_ID, Constants::DEFAULT_HOST_H1_NAME,
             Types::macToUint64(Constants::DEFAULT_HOST_H1_MAC), Types::ipToUint32(Constants::DEFAULT_HOST_H1_IP),
             Constants::DEFAULT_HOST_H1_X, Constants::DEFAULT_HOST_H1_Y,
             Constants::DEFAULT_HOST_H1_SWITCH, Constants::DEFAULT_HOST_H1_PORT},
            {Constants::DEFAULT_HOST_H2_ID, Constants::DEFAULT_HOST_H2_NAME,
             Types::macToUint64(Constants::DEFAULT_HOST_H2_MAC), Types::ipToUint32(Constants::DEFAULT_HOST_H2_IP),
             Constants::DEFAULT_HOST_H2_X, Constants::DEFAULT_HOST_H2_Y,
             Constants::DEFAULT_HOST_H2_SWITCH, Constants::DEFAULT_HOST_H2_PORT}
        };
        m_topology.links = {
            {Constants::DEFAULT_HOST_H1_ID, Constants::DEFAULT_HOST_H1_SWITCH},
            {Constants::DEFAULT_HOST_H1_SWITCH, "SW2"},
            {"SW2", "SW3"},
            {"SW3", Constants::DEFAULT_HOST_H2_SWITCH},
            {Constants::DEFAULT_HOST_H2_SWITCH, Constants::DEFAULT_HOST_H2_ID}
        };
        return;
    }

    try {
        nlohmann::json config;
        config_file >> config;

        // Load hosts
        for (const auto& h : config["hosts"]) {
            Host host;
            host.id = h["id"];
            host.name = h["name"];
            host.mac = Types::macToUint64(h["mac"]);
            host.ip = Types::ipToUint32(h["ip"]);
            host.x = h["x"];
            host.y = h["y"];
            host.connected_switch = h["connected_switch"];
            host.connected_port = h["connected_port"];
            m_topology.hosts.push_back(host);
        }

        // Load switches
        for (const auto& s : config["switches"]) {
            Switch sw;
            sw.id = s["id"];
            sw.name = s["name"];
            sw.mac = Types::macToUint64(s["mac"]);
            sw.x = s["x"];
            sw.y = s["y"];
            for (const auto& p : s["ports"]) {
                sw.ports.push_back({p["id"], p["connected_to"]});
            }
            m_topology.switches.push_back(sw);
        }

        // Load links
        for (const auto& l : config["links"]) {
            Link link;
            link.source = l["source"];
            link.target = l["target"];
            m_topology.links.push_back(link);
        }

        std::cout << Constants::LOG_PREFIX_APP << " " << Constants::INFO_LOADED_TOPOLOGY << ": "
                  << m_topology.hosts.size() << " hosts, "
                  << m_topology.links.size() << " links\n";

    } catch (const std::exception& e) {
        std::cerr << Constants::LOG_PREFIX_APP << " " << Constants::ERR_TOPOLOGY_CONFIG_PARSE
                  << ": " << e.what() << "\n";
    }
}

/**
 * @brief Setup signal handlers.
 *
 * Platform-specific signal handler configuration.
 */
void App::setupHandler() {
    // Signal handler setup is platform-specific
}

/**
 * @brief Initialize data plane components.
 *
 * Sets up SAI, pipeline, packet queue, and pipeline thread.
 */
void App::initializeDataPlane() {
    m_sai = std::make_unique<SAI::SimulatedSai>();
    m_pipeline = std::make_unique<DataPlane::Pipeline>(*m_sai);
    m_packet_queue = std::make_unique<DataPlane::SPSCQueue<DataPlane::Packet>>(Constants::DEFAULT_SPSC_QUEUE_CAPACITY);
    m_pipeline_thread = std::make_unique<DataPlane::PipelineThread>(*m_pipeline, *m_packet_queue, Constants::DEFAULT_BATCH_SIZE);
}

/**
 * @brief Initialize networking components.
 *
 * Only initializes networking if peer parameters are specified for distributed mode.
 */
void App::initializeNetworking() {
    if (!m_peer_ip.empty() && m_peer_port > 0) {
        m_network_link = Networking::NetworkProviderFactory::createTcpLink(m_listen_port, m_peer_ip, m_peer_port);
    }
}

/**
 * @brief Run the application.
 *
 * Starts all components and enters main loop.
 */
void App::run() {
    m_running.store(true);

    try {
        m_pipeline_thread->start();
        if (m_network_link) m_network_link->start();
        startPacketGenerator();

        std::cout << Constants::LOG_PREFIX_APP << " " << Constants::INFO_RUNNING << "\n";

        while (m_running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << Constants::LOG_PREFIX_APP << " " << Constants::INFO_FATAL_ERROR << ": " << e.what() << "\n";
        stop();
    }
}

/**
 * @brief Stop the application.
 *
 * Gracefully shuts down all components.
 */
void App::stop() {
    if (!m_running.load()) return;

    m_running.store(false);

    if (m_network_link) m_network_link->stop();
    if (m_pipeline_thread) m_pipeline_thread->stop();
    if (m_generator_thread && m_generator_thread->joinable()) m_generator_thread->join();
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
    
    if (m_sai) {
        oss << m_sai->getStats();
    }
    
    return oss.str();
}

void App::startPacketGenerator() {
    m_generator_thread = std::make_unique<std::thread>([this]() {
        Types::Count packet_counter = 0;
        while (m_running.load()) {
            generateTestPackets();
            ++packet_counter;
            std::this_thread::sleep_for(std::chrono::milliseconds(Constants::PACKET_GENERATION_INTERVAL_MS));
            if (packet_counter % 100 == 0) {
                Utils::Metrics::instance().inc(Constants::METRIC_GENERATOR_CYCLES);
            }
        }
    });
}

void App::generateTestPackets() {
    static Types::Count packet_id = 0;
    static int direction = 0;

    if (m_topology.hosts.size() < 2) return;

    const Host& src_host = m_topology.hosts[direction];
    const Host& dst_host = m_topology.hosts[1 - direction];

    DataPlane::Packet pkt(src_host.mac, dst_host.mac, src_host.ip, dst_host.ip, src_host.connected_port, ++packet_id);
    pkt.updateTimestamp();

    // Find path and route sequentially
    auto path = findPath(src_host.id, dst_host.id);
    if (!path.empty()) {
        routePacketSequentially(pkt, path);
    }

    direction = 1 - direction;
}

std::vector<std::string> App::findPath(const std::string& src, const std::string& dst) {
    // Simple BFS to find path through topology
    std::vector<std::string> path;
    std::map<std::string, std::string> parent;
    std::queue<std::string> q;
    
    q.push(src);
    parent[src] = "";
    
    while (!q.empty()) {
        auto current = q.front();
        q.pop();
        
        if (current == dst) {
            // Reconstruct path
            while (!current.empty()) {
                path.push_back(current);
                current = parent[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        // Find neighbors
        for (const auto& link : m_topology.links) {
            if (link.source == current && parent.find(link.target) == parent.end()) {
                parent[link.target] = current;
                q.push(link.target);
            } else if (link.target == current && parent.find(link.source) == parent.end()) {
                parent[link.source] = current;
                q.push(link.source);
            }
        }
    }
    
    return path;
}

/**
 * @brief Route packet through switches sequentially.
 *
 * Emits hop events for each node in the path and queues packet for pipeline processing at final destination.
 *
 * @param pkt Packet to route
 * @param path Vector of node identifiers representing the route
 */
void App::routePacketSequentially(const DataPlane::Packet& pkt, const std::vector<std::string>& path) {
    auto& event_bus = Events::getGlobalEventBus();

    for (size_t i = 0; i < path.size(); ++i) {
        nlohmann::json event;
        event["type"] = Constants::EVENT_PACKET_HOP;
        event["packet_id"] = pkt.id();
        event["current_node"] = path[i];
        event["next_node"] = (i + 1 < path.size()) ? path[i + 1] : "";
        event["hop_index"] = i;
        event["total_hops"] = path.size();
        event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
        event_bus.publishJson(event);

        // Process packet at this hop
        if (i == path.size() - 1) {
            // Final destination - queue for pipeline processing
            auto pkt_copy = pkt;
            if (m_packet_queue->push(std::move(pkt_copy))) {
                Utils::Metrics::instance().inc(Constants::METRIC_PACKETS_GENERATED);
            }
        }
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
