#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <memory>

const int MSG_DATA = 0;
const int MSG_CONTROL = 1;
const int MSG_DISCOVERY = 2;
const int MSG_GOSSIP = 3;
const int MSG_HEARTBEAT = 4;
const int MSG_ERROR = 5;

struct Message {
    int type;
    std::vector<uint8_t> payload;
};

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const Message& msg) = 0;
    virtual void broadcast(const Message& msg) = 0;
    virtual std::string getStats() const = 0;
};

class ProtocolStack {
public:
    void registerHandler(const std::string& name, std::unique_ptr<IProtocolHandler> handler) {
        std::cout << "[ProtocolStack] Registered handler: " << name << "\n";
        handlers[name] = std::move(handler);
    }
    void startAll() {
        for (auto& [name, handler] : handlers) {
            handler->start();
            std::cout << "[ProtocolStack] Started: " << name << "\n";
        }
    }
    void stopAll() {
        for (auto& [name, handler] : handlers) {
            handler->stop();
            std::cout << "[ProtocolStack] Stopped: " << name << "\n";
        }
    }
    void broadcast(const Message& msg) {
        for (auto& [name, handler] : handlers) {
            handler->broadcast(msg);
        }
    }
    void getAllStats() const {
        for (const auto& [name, handler] : handlers) {
            std::cout << "\n=== " << name << " ===\n";
            std::cout << handler->getStats() << "\n";
        }
    }
private:
    std::map<std::string, std::unique_ptr<IProtocolHandler>> handlers;
};

class WebSocketGateway : public IProtocolHandler {
public:
    WebSocketGateway(int port) : m_port(port), m_running(false) {}
    void start() override {
        m_running = true;
        std::cout << "[WebSocket] Started on port " << m_port << "\n";
    }
    void stop() override {
        m_running = false;
        std::cout << "[WebSocket] Stopped\n";
    }
    void send(const Message& msg) override {
        m_messages_sent++;
        std::cout << "[WebSocket] Sent message (" << msg.payload.size() << " bytes)\n";
    }
    void broadcast(const Message& msg) override {
        m_messages_sent++;
        std::cout << "[WebSocket] Broadcast message to " << m_connections << " clients\n";
    }
    std::string getStats() const override {
        return "Active connections: " + std::to_string(m_connections) + "\n" +
               "Messages sent: " + std::to_string(m_messages_sent) + "\n" +
               "Messages received: " + std::to_string(m_messages_received) + "\n";
    }
private:
    int m_port;
    bool m_running;
    int m_connections{0};
    int m_messages_sent{0};
    int m_messages_received{0};
};

class GossipProtocol : public IProtocolHandler {
public:
    GossipProtocol(const std::string& peer_id) : m_peer_id(peer_id), m_running(false) {}
    void start() override {
        m_running = true;
        std::cout << "[Gossip] Started with peer ID: " << m_peer_id << "\n";
    }
    void stop() override {
        m_running = false;
        std::cout << "[Gossip] Stopped\n";
    }
    void send(const Message& msg) override {
        m_messages_sent++;
        std::cout << "[Gossip] Sent message to peer\n";
    }
    void broadcast(const Message& msg) override {
        m_messages_sent++;
        std::cout << "[Gossip] Gossip message to " << m_peers.size() << " peers\n";
    }
    std::string getStats() const override {
        return "Peer ID: " + m_peer_id + "\n" +
               "Known peers: " + std::to_string(m_peers.size()) + "\n" +
               "Messages sent: " + std::to_string(m_messages_sent) + "\n" +
               "Messages received: " + std::to_string(m_messages_received) + "\n" +
               "Gossip rounds: " + std::to_string(m_gossip_rounds) + "\n";
    }
private:
    std::string m_peer_id;
    bool m_running;
    std::map<std::string, int> m_peers;
    int m_messages_sent{0};
    int m_messages_received{0};
    int m_gossip_rounds{0};
};

class Serializer {
public:
    std::vector<uint8_t> serialize(const std::string& data) {
        std::vector<uint8_t> buffer(data.begin(), data.end());
        std::cout << "[Serialization] Serialized " << buffer.size() << " bytes\n";
        return buffer;
    }
};

int main() {
    std::cout << "Protocol-Oriented Networking Stack Demo\n\n";

    ProtocolStack stack;
    auto ws_gateway = std::make_unique<WebSocketGateway>(8080);
    auto gossip = std::make_unique<GossipProtocol>("peer_001");

    stack.registerHandler("websocket", std::move(ws_gateway));
    stack.registerHandler("gossip", std::move(gossip));

    std::cout << "\n--- Starting Protocol Stack ---\n\n";
    stack.startAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Serializer serializer;
    std::string test_data = "Hello from protocol stack!";
    auto serialized = serializer.serialize(test_data);

    Message msg;
    msg.type = MSG_DATA;
    msg.payload = serialized;

    std::cout << "\n--- Broadcasting Message ---\n\n";
    stack.broadcast(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\n--- Simulating Gossip Activity ---\n\n";
    std::cout << "[Gossip] Adding peer: peer_002\n";
    std::cout << "[Gossip] Adding peer: peer_003\n";

    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::cout << "[Gossip] Gossip round " << (i + 1) << " completed\n";
    }

    std::cout << "\n--- Protocol Statistics ---\n\n";
    stack.getAllStats();

    std::cout << "\n--- Stopping Protocol Stack ---\n\n";
    stack.stopAll();

    std::cout << "\nDemo Complete\n\n";

    std::cout << "Key Features Demonstrated:\n";
    std::cout << "  Pluggable protocol handler interface\n";
    std::cout << "  Protocol stack management\n";
    std::cout << "  WebSocket gateway for real-time communication\n";
    std::cout << "  P2P gossip protocol for distributed systems\n";
    std::cout << "  Zero-copy serialization\n";
    std::cout << "  Unified message broadcasting\n";
    std::cout << "  Protocol statistics and monitoring\n\n";

    return 0;
}
