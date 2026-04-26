#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace MiniSonic::Visualization {

/**
 * @brief Simple TCP session for a single client
 */
class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    explicit TcpSession(tcp::socket socket)
        : m_socket(std::move(socket)) {
    }

    void start() {
        std::cout << "[TcpSession] Client connected\n";
        doRead();
    }

    void send(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_write_mutex);
        asio::write(m_socket, asio::buffer(message + "\n"));
    }

    void close() {
        boost::system::error_code ec;
        m_socket.close(ec);
    }

private:
    void doRead() {
        auto self = shared_from_this();
        m_socket.async_read_some(
            asio::buffer(m_buffer),
            [this, self](boost::system::error_code ec, size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "[TcpSession] Read failed: " << ec.message() << "\n";
                    return;
                }
                // Handle incoming data if needed
                doRead();
            });
    }

private:
    tcp::socket m_socket;
    std::array<char, 8192> m_buffer;
    std::mutex m_write_mutex;
};

/**
 * @brief Simple TCP Event Server
 *
 * Streams events via plain TCP (JSON lines).
 */
class TcpEventServer {
public:
    TcpEventServer(uint16_t port = 8080)
        : m_io_context(),
          m_acceptor(m_io_context, tcp::endpoint(tcp::v4(), port)),
          m_port(port),
          m_running(false) {
    }

    ~TcpEventServer() {
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
            int packet_counter = 0;
            while (m_running.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_sessions.empty()) {
                    sendTestEvent(packet_counter++);
                }
            }
        });

        std::cout << "[TcpEventServer] Started on port " << m_port << "\n";
        std::cout << "[TcpEventServer] Test event timer started\n";
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

        std::cout << "[TcpEventServer] Stopped\n";
    }

private:
    void doAccept() {
        m_acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!m_running.load()) {
                    return;
                }

                if (ec) {
                    std::cerr << "[TcpEventServer] Accept failed: " << ec.message() << "\n";
                    return;
                }

                auto session = std::make_shared<TcpSession>(std::move(socket));
                session->start();

                {
                    std::lock_guard<std::mutex> lock(m_sessions_mutex);
                    m_sessions.push_back(session);
                }

                doAccept();
            });
    }

    void sendTestEvent(int packet_counter) {
        nlohmann::json event;
        event["type"] = "PacketGenerated";
        event["packet"] = {
            {"id", packet_counter},
            {"src_ip", "10.0.1.2"},
            {"dst_ip", "10.0.3.7"}
        };

        std::string message = event.dump();

        std::lock_guard<std::mutex> lock(m_sessions_mutex);
        for (auto& session : m_sessions) {
            session->send(message);
        }
    }

private:
    asio::io_context m_io_context;
    tcp::acceptor m_acceptor;
    uint16_t m_port;
    std::atomic<bool> m_running;
    std::thread m_server_thread;
    std::thread m_test_timer_thread;
    std::vector<std::shared_ptr<TcpSession>> m_sessions;
    std::mutex m_sessions_mutex;
};

std::unique_ptr<TcpEventServer> g_tcp_event_server;

void startTcpEventServer(uint16_t port = 8080) {
    if (!g_tcp_event_server) {
        g_tcp_event_server = std::make_unique<TcpEventServer>(port);
        g_tcp_event_server->start();
    }
}

void stopTcpEventServer() {
    if (g_tcp_event_server) {
        g_tcp_event_server->stop();
    }
}

} // namespace MiniSonic::Visualization

int main(int argc, char* argv[]) {
    using namespace MiniSonic::Visualization;

    uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::cout << "Mini_SONiC TCP Event Server\n";
    std::cout << "============================\n";
    std::cout << "Starting TCP event server on port " << port << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    startTcpEventServer(port);

    std::cout << "Server running. Press Ctrl+C to stop...\n";
    std::cin.get();

    stopTcpEventServer();

    return 0;
}
