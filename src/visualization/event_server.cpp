#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

// Import Events module
import MiniSonic.Events;

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
 * @brief WebSocket session for a single client
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
            if (json.contains("type") && json["type"] == "speed_control") {
                if (json.contains("speed") && m_speed_callback) {
                    int speed = json["speed"];
                    m_speed_callback(speed);
                    std::cout << "[Session] Speed control received: " << speed << "x\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Session] Failed to parse message: " << e.what() << "\n";
        }
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
          m_running(false),
          m_speed(1) {
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

        // Start test event timer
        m_test_timer_thread = std::thread([this]() {
            while (m_running.load()) {
                int speed = getSpeed();
                int delay = std::max(2, 10 / speed); // Adjust delay based on speed (10s base)
                std::this_thread::sleep_for(std::chrono::seconds(delay));
                if (!m_sessions.empty()) {
                    sendTestEvent();
                }
            }
        });

        // Subscribe to event bus - subscribe to all events using wildcard
        auto& event_bus = Events::getGlobalEventBus();
        event_bus.subscribe("*", [this](const nlohmann::json& json_event) {
            handleJsonEvent(json_event);
        });

        std::cout << "[EventServer] Started on port " << m_port << "\n";
        std::cout << "[EventServer] Subscribed to global event bus\n";
        std::cout << "[EventServer] Test event timer started (sends events every second when clients connected)\n";
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

                // Create a new session
                auto session = std::make_shared<Session>(std::move(socket));
                session->setSpeedCallback([this](int speed) { setSpeed(speed); });
                session->start();

                {
                    std::lock_guard<std::mutex> lock(m_sessions_mutex);
                    m_sessions.push_back(session);
                    std::cout << "[EventServer] Client connected. Total clients: " 
                              << m_sessions.size() << "\n";
                }

                // Accept next connection
                doAccept();
            });
    }

    void handleJsonEvent(const nlohmann::json& json_event) {
        if (!m_running.load()) {
            return;
        }

        // Convert JSON to string
        string json_data = json_event.dump();
        json_data += "\n"; // Add newline for framing

        // Broadcast to all connected clients
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

    void setSpeed(int speed) {
        std::lock_guard<std::mutex> lock(m_speed_mutex);
        m_speed = speed;
        std::cout << "[EventServer] Speed set to " << speed << "x\n";
    }

    int getSpeed() const {
        std::lock_guard<std::mutex> lock(m_speed_mutex);
        return m_speed;
    }

    void sendTestEvent() {
        if (!m_running.load()) {
            return;
        }

        static int packet_counter = 0;
        packet_counter++;

        // Use switches that exist in the web visualizer topology
        static const char* switch_names[] = {"TOR1", "Spine1", "Spine2", "TOR2"};
        const char* current_switch = switch_names[packet_counter % 4];
        const char* next_switch = switch_names[(packet_counter + 1) % 4];

        // Send PacketGenerated event
        nlohmann::json packet_gen;
        packet_gen["type"] = "PacketGenerated";
        packet_gen["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        packet_gen["packet"] = {
            {"id", "TEST-" + std::to_string(packet_counter)},
            {"src_ip", "192.168.1.100"},
            {"dst_ip", "192.168.1.200"},
            {"src_port", 12345},
            {"dst_port", 80},
            {"protocol", "TCP"}
        };
        handleJsonEvent(packet_gen);

        // Send PacketEnteredSwitch event (this triggers visualization)
        nlohmann::json packet_entered;
        packet_entered["type"] = "PacketEnteredSwitch";
        packet_entered["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        packet_entered["switch_id"] = current_switch;
        packet_entered["packet_id"] = "TEST-" + std::to_string(packet_counter);
        packet_entered["ingress_port"] = "eth0";
        handleJsonEvent(packet_entered);

        // Send PacketForwardDecision event
        nlohmann::json packet_forward;
        packet_forward["type"] = "PacketForwardDecision";
        packet_forward["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        packet_forward["switch_id"] = current_switch;
        packet_forward["packet_id"] = "TEST-" + std::to_string(packet_counter);
        packet_forward["egress_port"] = "eth1";
        packet_forward["next_hop"] = next_switch;
        handleJsonEvent(packet_forward);

        // Send PacketExitedSwitch event (triggers packet movement along link)
        nlohmann::json packet_exited;
        packet_exited["type"] = "PacketExitedSwitch";
        packet_exited["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        packet_exited["switch_id"] = current_switch;
        packet_exited["packet_id"] = "TEST-" + std::to_string(packet_counter);
        packet_exited["egress_port"] = "eth1";
        packet_exited["next_hop"] = next_switch;
        packet_exited["src_ip"] = "10.0.1.2";
        packet_exited["dst_ip"] = "10.0.3.7";
        packet_exited["src_port"] = 443;
        packet_exited["dst_port"] = 51832;
        packet_exited["dscp"] = 46;
        packet_exited["ttl"] = 62;
        packet_exited["state"] = "normal";
        handleJsonEvent(packet_exited);
    }

    void handleEvent(const std::shared_ptr<Events::Event>& event) {
        if (!m_running.load()) {
            return;
        }

        // Serialize event to JSON
        string json_data = event->toJson();
        json_data += "\n"; // Add newline for framing

        // Broadcast to all connected clients
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

    net::io_context m_io_context;
    tcp::acceptor m_acceptor;
    uint16_t m_port;
    atomic<bool> m_running;
    thread m_server_thread;
    thread m_test_timer_thread;
    std::vector<std::shared_ptr<Session>> m_sessions;
    mutex m_sessions_mutex;
    int m_speed;
    mutable mutex m_speed_mutex;
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
