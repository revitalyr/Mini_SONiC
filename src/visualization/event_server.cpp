#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

// Import Events module
import MiniSonic.Core.Events; // Corrected module name

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using std::string;
using std::thread;
using std::atomic;
using std::mutex;
using std::unique_ptr;

namespace MiniSonic::Visualization {

/**
 * @brief Plain TCP session for Qt visualizer
 */
class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    explicit TcpSession(boost::asio::ip::tcp::socket socket)
        : m_socket(std::move(socket)) {
    }

    void start() {
        std::cout << "[TcpSession] TCP connection accepted\n";
        startReading();
    }

    void send(const string& message) {
        boost::asio::write(m_socket, boost::asio::buffer(message + "\n"));
    }

    void close() {
        boost::system::error_code ec;
        m_socket.close(ec);
    }

private:
    void startReading() {
        auto self = shared_from_this();
        m_socket.async_read_some(
            boost::asio::buffer(m_buffer),
            [this, self](boost::system::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    return;
                }
                // Parse received data
                std::string data(m_buffer.data(), bytes_transferred);
                handleMessage(data);
                startReading();
            });
    }

    void handleMessage(const string& data) {
        try {
            auto json = ::nlohmann::json::parse(data);
            if (json.contains("type") && json["type"] == "topology_query") {
                sendTopology();
            }
        } catch (...) {
            // Ignore parse errors
        }
    }

    void sendTopology() {
        nlohmann::json topology;
        topology["type"] = "topology";
        topology["nodes"] = nlohmann::json::array();
        topology["links"] = nlohmann::json::array();

        // Read topology from config file (same as app)
        std::ifstream config_file("examples/config/topology.json");
        if (!config_file.is_open()) {
            std::cerr << "[TcpSession] Failed to open topology config\n";
            return;
        }

        try {
            nlohmann::json config;
            config_file >> config;

            // Load hosts as nodes
            if (config.contains("hosts")) {
                for (const auto& h : config["hosts"]) {
                    nlohmann::json node;
                    node["id"] = h["id"];
                    node["type"] = "host";
                    node["x"] = h["x"];
                    node["y"] = h["y"];
                    topology["nodes"].push_back(node);
                }
            }

            // Load switches as nodes
            if (config.contains("switches")) {
                for (const auto& s : config["switches"]) {
                    nlohmann::json node;
                    node["id"] = s["id"];
                    node["type"] = "switch";
                    node["role"] = s.contains("role") ? s["role"] : "switch";
                    node["x"] = s["x"];
                    node["y"] = s["y"];
                    topology["nodes"].push_back(node);
                }
            }

            // Load links
            if (config.contains("links")) {
                for (const auto& l : config["links"]) {
                    nlohmann::json link;
                    link["source"] = l["source"];
                    link["target"] = l["target"];
                    topology["links"].push_back(link);
                }
            }

            std::cout << "[TcpSession] Sent topology: " << topology["nodes"].size() << " nodes, " << topology["links"].size() << " links\n";
        } catch (const std::exception& e) {
            std::cerr << "[TcpSession] Failed to parse topology config: " << e.what() << "\n";
            return;
        }

        send(topology.dump());
    }

private:
    boost::asio::ip::tcp::socket m_socket;
    std::array<char, 8192> m_buffer;
};

/**
 * @brief WebSocket session for web visualizer
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(boost::asio::ip::tcp::socket socket)
        : m_ws(std::move(socket)) {
    }

    void start() {
        // Accept the websocket handshake
        m_ws.async_accept(
            [self = shared_from_this()](boost::beast::error_code ec) {
                if (ec) {
                    std::cerr << "[Session] Accept failed: " << ec.message() << "\n";
                    return;
                }
                std::cout << "[Session] WebSocket connection accepted\n";
                self->startReading();
            });
    }

    void send(const string& message) {
        // Send a message synchronously for simplicity
        m_ws.write(boost::asio::buffer(message));
    }

    void close() {
        boost::beast::error_code ec;
        m_ws.close(boost::beast::websocket::close_code::normal, ec);
    }

    void setSpeedCallback(std::function<void(int)> callback) {
        m_speed_callback = callback;
    }

private:
    void startReading() {
        m_ws.async_read(
            m_buffer,
            [this](boost::system::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "[Session] Read failed: " << ec.message() << "\n";
                    return;
                }

                // Parse the message
                std::string data(static_cast<const char*>(m_buffer.data().data()), bytes_transferred);
                handleMessage(data);

                // Clear buffer and continue reading
                m_buffer.consume(bytes_transferred);
                startReading();
            });
    }

    void handleMessage(const string& data) {
        try {
            auto json = ::nlohmann::json::parse(data);
            if (json.contains("type")) {
                if (json["type"] == "speed_control") {
                    if (json.contains("speed") && m_speed_callback) {
                        int speed = json["speed"];
                        m_speed_callback(speed);
                        std::cout << "[Session] Speed control received: " << speed << "x\n";
                    }
                } else if (json["type"] == "topology_query") {
                    // Send topology information
                    sendTopology();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Session] Failed to parse message: " << e.what() << "\n";
        }
    }

    void sendTopology() {
        nlohmann::json topology;
        topology["type"] = "topology";
        topology["nodes"] = nlohmann::json::array();
        topology["links"] = nlohmann::json::array();

        // Read topology from config file (same as app)
        std::ifstream config_file("examples/config/topology.json");
        if (!config_file.is_open()) {
            std::cerr << "[Session] Failed to open topology config\n";
            return;
        }

        try {
            nlohmann::json config;
            config_file >> config;

            // Load hosts as nodes
            if (config.contains("hosts")) {
                for (const auto& h : config["hosts"]) {
                    nlohmann::json node;
                    node["id"] = h["id"];
                    node["type"] = "host";
                    node["x"] = h["x"];
                    node["y"] = h["y"];
                    topology["nodes"].push_back(node);
                }
            }

            // Load switches as nodes
            if (config.contains("switches")) {
                for (const auto& s : config["switches"]) {
                    nlohmann::json node;
                    node["id"] = s["id"];
                    node["type"] = "switch";
                    node["role"] = s.contains("role") ? s["role"] : "switch";
                    node["x"] = s["x"];
                    node["y"] = s["y"];
                    topology["nodes"].push_back(node);
                }
            }

            // Load links
            if (config.contains("links")) {
                for (const auto& l : config["links"]) {
                    nlohmann::json link;
                    link["source"] = l["source"];
                    link["target"] = l["target"];
                    topology["links"].push_back(link);
                }
            }

            std::cout << "[Session] Sent topology: " << topology["nodes"].size() << " nodes, " << topology["links"].size() << " links\n";
        } catch (const std::exception& e) {
            std::cerr << "[Session] Failed to parse topology config: " << e.what() << "\n";
            return;
        }

        string topology_data = topology.dump();
        topology_data += "\n";
        m_ws.write(boost::asio::buffer(topology_data));
    }

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_ws;
    boost::beast::flat_buffer m_buffer;
    std::function<void(int)> m_speed_callback;
};

/**
 * @brief Event Server that streams events via WebSocket using boost.asio
 * 
 * Subscribes to the global event bus and broadcasts events as JSON to all connected clients.
 */
class EventServer {
public:
    EventServer(uint16_t port = 8080)
        : m_io_context(),
          m_acceptor(m_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          m_port(port),
          m_running(false) {
    }

    ~EventServer() {
        stop();
    }

    void start() {
        if (m_running.load()) {
            return;
        }

        m_running.store(true);
        m_server_thread = std::thread([this]() {
            doAccept();
            m_io_context.run();
        });

        // Subscribe to event bus - subscribe to packet_hop events
        auto& event_bus = MiniSonic::Events::getGlobalEventBus();
        event_bus.subscribe("packet_hop", [this](const nlohmann::json& json_event) {
            handleJsonEvent(json_event);
        });

        std::cout << "[EventServer] Started on port " << m_port << "\n";
        std::cout << "[EventServer] Subscribed to global event bus\n";
    }

    void stop() {
        if (!m_running.load()) {
            return;
        }

        m_running.store(false);
        m_io_context.stop();

        if (m_server_thread.joinable()) {
            m_server_thread.join();
        }

        // Close all client connections
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        for (auto& session : m_sessions) {
            session->close();
        }
        m_sessions.clear();

        std::cout << "[EventServer] Stopped\n";
    }

    bool isRunning() const {
        return m_running.load();
    }

private:
    void doAccept() {
        m_acceptor.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!m_running.load()) {
                    return;
                }

                if (ec) {
                    std::cerr << "[EventServer] Accept failed: " << ec.message() << "\n";
                    return;
                }

                // Create a TCP session for Qt visualizer (plain TCP)
                auto tcp_session = std::make_shared<TcpSession>(std::move(socket));

                {
                    std::lock_guard<std::mutex> lock(m_sessions_mutex);
                    m_sessions.push_back(tcp_session);
                    std::cout << "[EventServer] TCP client connected. Total clients: "
                              << m_sessions.size() << "\n";
                }

                tcp_session->start();

                // Accept next connection
                doAccept();
            });
    }

    void handleJsonEvent(const nlohmann::json& json_event) {
        if (!m_running.load()) {
            return;
        }

        // Transform packet_hop events to visualizer's expected format
        nlohmann::json transformed_event = json_event;
        if (json_event.contains("type") && json_event["type"] == "packet_hop") {
            std::string packet_id;
            if (json_event["packet_id"].is_string()) {
                packet_id = json_event["packet_id"].get<std::string>();
            } else {
                packet_id = std::to_string(json_event["packet_id"].get<int>());
            }
            std::string current_node = json_event["current_node"];
            std::string next_node = json_event.contains("next_node") ? json_event["next_node"].get<std::string>() : "";
            int hop_index = json_event["hop_index"];
            int total_hops = json_event["total_hops"];

            if (hop_index == 0) {
                // First hop - send PacketGenerated event
                nlohmann::json packet_gen;
                packet_gen["type"] = "PacketGenerated";
                packet_gen["timestamp"] = json_event["timestamp"];
                packet_gen["packet"] = {
                    {"id", packet_id},
                    {"src_ip", "10.0.1.2"},
                    {"dst_ip", "10.0.3.7"},
                    {"src_port", 12345},
                    {"dst_port", 80},
                    {"protocol", "TCP"}
                };
                transformed_event = packet_gen;
            } else {
                // Subsequent hops - send packet events
                if (hop_index == 1) {
                    nlohmann::json entered;
                    entered["type"] = "PacketEnteredSwitch";
                    entered["timestamp"] = json_event["timestamp"];
                    entered["switch_id"] = current_node;
                    entered["packet_id"] = packet_id;
                    entered["ingress_port"] = "Eth0";
                    transformed_event = entered;
                } else if (hop_index < total_hops - 1) {
                    nlohmann::json forward;
                    forward["type"] = "PacketForwardDecision";
                    forward["timestamp"] = json_event["timestamp"];
                    forward["switch_id"] = current_node;
                    forward["packet_id"] = packet_id;
                    forward["egress_port"] = "Eth1";
                    forward["next_hop"] = next_node;
                    transformed_event = forward;

                    nlohmann::json exited;
                    exited["type"] = "PacketExitedSwitch";
                    exited["timestamp"] = json_event["timestamp"];
                    exited["switch_id"] = current_node;
                    exited["packet_id"] = packet_id;
                    exited["egress_port"] = "Eth1";
                    exited["next_hop"] = next_node;
                    exited["src_ip"] = "10.0.1.2";
                    exited["dst_ip"] = "10.0.3.7";
                    broadcastEvent(exited);
                } else {
                    nlohmann::json exited;
                    exited["type"] = "PacketExitedSwitch";
                    exited["timestamp"] = json_event["timestamp"];
                    exited["switch_id"] = current_node;
                    exited["packet_id"] = packet_id;
                    exited["egress_port"] = "Eth0";
                    exited["next_hop"] = next_node;
                    exited["src_ip"] = "10.0.1.2";
                    exited["dst_ip"] = "10.0.3.7";
                    transformed_event = exited;
                }
            }
        }

        // Convert JSON to string
        string json_data = transformed_event.dump();

        // Broadcast to all connected clients
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        if (m_sessions.empty()) {
            static int drop_counter = 0;
            if (++drop_counter % 100 == 0) {
                std::cout << "[EventServer] No clients connected, events dropped: " << drop_counter << "\n";
            }
        } else {
            static int event_counter = 0;
            if (++event_counter % 50 == 0) {
                std::cout << "[EventServer] Broadcasted " << event_counter << " events to " << m_sessions.size() << " clients\n";
            }
            for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
                try {
                    (*it)->send(json_data);
                    ++it;
                } catch (const std::exception& e) {
                    std::cerr << "[EventServer] Failed to send to client: " << e.what() << "\n";
                    (*it)->close();
                    it = m_sessions.erase(it);
                }
            }
        }
    }

    void broadcastEvent(const nlohmann::json& event) {
        string json_data = event.dump();
        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
            try {
                (*it)->send(json_data);
                ++it;
            } catch (const std::exception& e) {
                std::cerr << "[EventServer] Failed to send to client: " << e.what() << "\n";
                (*it)->close();
                it = m_sessions.erase(it);
            }
        }
    }

private:
    net::io_context m_io_context;
    tcp::acceptor m_acceptor;
    uint16_t m_port;
    atomic<bool> m_running;
    thread m_server_thread;
    std::vector<std::shared_ptr<TcpSession>> m_sessions;
    mutex m_sessions_mutex;
};

// Global event server instance
unique_ptr<EventServer> g_event_server;

void startEventServer(uint16_t port = 8080) {
    if (!g_event_server) {
        g_event_server = std::make_unique<EventServer>(port);
        g_event_server->start();
    }
}

void stopEventServer() {
    if (g_event_server) {
        g_event_server->stop();
        g_event_server.reset();
    }
}

} // namespace MiniSonic::Visualization

#ifndef VISUALIZATION_LIBRARY
int main(int argc, char* argv[]) {
    using namespace MiniSonic::Visualization;

    uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::cout << "Mini_SONiC Event Server\n";
    std::cout << "========================\n";
    std::cout << "Starting WebSocket event server on port " << port << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    startEventServer(port);

    // Keep running until interrupted
    std::cout << "[EventServer] Running. Waiting for events...\n";
    while (g_event_server && g_event_server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    stopEventServer();

    return 0;
}
#endif
