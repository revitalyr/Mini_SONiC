#include "gossip.ixx"

export namespace MiniSonic::Gossip {

// =============================================================================
// GOSSIP PROTOCOL IMPLEMENTATION
// =============================================================================

GossipProtocol::GossipProtocol(string peer_id, const GossipConfig& config)
    : m_peer_id(std::move(peer_id))
    , m_config(config)
    , m_rng(std::random_device{}()) {}

GossipProtocol::~GossipProtocol() {
    stop();
}

void GossipProtocol::start() {
    if (m_running.load()) {
        return;
    }
    
    m_running.store(true);
    m_incarnation.store(0);
    m_heartbeat.store(0);
    
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
    
    // Create server socket
    m_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server_socket == INVALID_SOCKET) {
        if (m_error_handler) {
            m_error_handler("Failed to create gossip server socket");
        }
        return;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind to address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_config.listen_port);
    
    if (bind(m_server_socket, reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr)) == SOCKET_ERROR) {
        if (m_error_handler) {
            m_error_handler("Failed to bind gossip server socket");
        }
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
        return;
    }
    
    // Listen
    if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR) {
        if (m_error_handler) {
            m_error_handler("Failed to listen on gossip server socket");
        }
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
        return;
    }
    
    // Start threads
    m_gossip_thread = thread(&GossipProtocol::gossipLoop, this);
    m_failure_detection_thread = thread(&GossipProtocol::failureDetectionLoop, this);
    
    std::cout << "[GossipProtocol] Started peer " << m_peer_id 
              << " on port " << m_config.listen_port << "\n";
}

void GossipProtocol::stop() {
    if (!m_running.load()) {
        return;
    }
    
    m_running.store(false);
    
    if (m_gossip_thread.joinable()) {
        m_gossip_thread.join();
    }
    
    if (m_failure_detection_thread.joinable()) {
        m_failure_detection_thread.join();
    }
    
    // Close server socket
    if (m_server_socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(m_server_socket);
#else
        close(m_server_socket);
#endif
        m_server_socket = INVALID_SOCKET;
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    std::cout << "[GossipProtocol] Stopped peer " << m_peer_id << "\n";
}

bool GossipProtocol::isRunning() const noexcept {
    return m_running.load();
}

void GossipProtocol::send(const Protocol::Message& msg) {
    // Convert to gossip payload and broadcast
    GossipPayload payload;
    payload.type = GossipMessageType::GOSSIP_DIGEST;
    payload.sender_id = m_peer_id;
    payload.incarnation = m_incarnation.load();
    
    broadcastToRandomPeers(payload);
    m_messages_sent.fetch_add(1);
}

void GossipProtocol::broadcast(const Protocol::Message& msg) {
    send(msg);
}

void GossipProtocol::setMessageHandler(MessageCallback handler) {
    m_message_handler = std::move(handler);
}

void GossipProtocol::setErrorHandler(ErrorCallback handler) {
    m_error_handler = std::move(handler);
}

string GossipProtocol::getStats() const {
    lock_guard<mutex> lock(m_peers_mutex);
    return "Gossip Protocol Statistics:\n"
           "  Running: " + string(m_running.load() ? "yes" : "no") + "\n"
           "  Peer ID: " + m_peer_id + "\n"
           "  Peer Count: " + std::to_string(m_peers.size()) + "\n"
           "  Messages Sent: " + std::to_string(m_messages_sent.load()) + "\n"
           "  Messages Received: " + std::to_string(m_messages_received.load()) + "\n"
           "  Gossip Rounds: " + std::to_string(m_gossip_rounds.load()) + "\n"
           "  Peer Joins: " + std::to_string(m_peer_joins.load()) + "\n"
           "  Peer Leaves: " + std::to_string(m_peer_leaves.load()) + "\n";
}

void GossipProtocol::joinPeer(const string& address, Types::Port port) {
    PeerInfo peer;
    peer.peer_id = address + ":" + std::to_string(port);
    peer.address = address;
    peer.port = port;
    peer.state = PeerState::ALIVE;
    peer.heartbeat = 0;
    peer.last_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    peer.incarnation = 0;
    
    addPeer(peer);
    
    // Send join message
    GossipPayload payload;
    payload.type = GossipMessageType::JOIN;
    payload.sender_id = m_peer_id;
    sendToPeer(peer.peer_id, payload);
    
    std::cout << "[GossipProtocol] Joined peer: " << peer.peer_id << "\n";
}

void GossipProtocol::leaveCluster() {
    GossipPayload payload;
    payload.type = GossipMessageType::LEAVE;
    payload.sender_id = m_peer_id;
    
    broadcastToRandomPeers(payload);
    
    lock_guard<mutex> lock(m_peers_mutex);
    m_peers.clear();
    
    std::cout << "[GossipProtocol] Left cluster\n";
}

vector<PeerInfo> GossipProtocol::getPeers() const {
    lock_guard<mutex> lock(m_peers_mutex);
    vector<PeerInfo> peers;
    peers.reserve(m_peers.size());
    
    for (const auto& [id, peer] : m_peers) {
        peers.push_back(peer);
    }
    
    return peers;
}

size_t GossipProtocol::peerCount() const {
    lock_guard<mutex> lock(m_peers_mutex);
    return m_peers.size();
}

void GossipProtocol::setPeerJoinCallback(PeerJoinCallback callback) {
    m_peer_join_callback = std::move(callback);
}

void GossipProtocol::setPeerLeaveCallback(PeerLeaveCallback callback) {
    m_peer_leave_callback = std::move(callback);
}

void GossipProtocol::gossipLoop() {
    while (m_running.load()) {
        m_heartbeat.fetch_add(1);
        m_gossip_rounds.fetch_add(1);
        
        // Create gossip digest
        GossipPayload payload;
        payload.type = GossipMessageType::GOSSIP_DIGEST;
        payload.sender_id = m_peer_id;
        payload.incarnation = m_incarnation.load();
        
        // Add peer heartbeat info
        lock_guard<mutex> lock(m_peers_mutex);
        for (const auto& [id, peer] : m_peers) {
            payload.heartbeats[id] = peer.heartbeat;
        }
        
        // Broadcast to random peers
        broadcastToRandomPeers(payload);
        m_messages_sent.fetch_add(1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.gossip_interval_ms));
    }
}

void GossipProtocol::failureDetectionLoop() {
    while (m_running.load()) {
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        lock_guard<mutex> lock(m_peers_mutex);
        
        for (auto it = m_peers.begin(); it != m_peers.end(); ) {
            auto& [id, peer] = *it;
            uint64_t time_since_seen = now - peer.last_seen;
            
            if (peer.state == PeerState::ALIVE && 
                time_since_seen > m_config.suspicion_timeout_ms) {
                peer.state = PeerState::SUSPECTED;
                std::cout << "[GossipProtocol] Peer suspected: " << id << "\n";
                ++it;
            } else if (peer.state == PeerState::SUSPECTED && 
                       time_since_seen > m_config.dead_timeout_ms) {
                peer.state = PeerState::DEAD;
                std::cout << "[GossipProtocol] Peer declared dead: " << id << "\n";
                
                if (m_peer_leave_callback) {
                    m_peer_leave_callback(peer);
                }
                
                m_peer_leaves.fetch_add(1);
                it = m_peers.erase(it);
            } else {
                ++it;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.ping_interval_ms));
    }
}

void GossipProtocol::handleGossipMessage(const GossipPayload& payload, const string& source) {
    m_messages_received.fetch_add(1);
    
    switch (payload.type) {
        case GossipMessageType::PING:
            handlePing(payload, source);
            break;
        case GossipMessageType::PONG:
            handlePong(payload, source);
            break;
        case GossipMessageType::GOSSIP_DIGEST:
            handleMemberList(payload, source);
            break;
        case GossipMessageType::MEMBER_LIST:
            handleMemberList(payload, source);
            break;
        case GossipMessageType::JOIN:
            // Handle new peer joining
            std::cout << "[GossipProtocol] Peer join request from: " << payload.sender_id << "\n";
            break;
        case GossipMessageType::LEAVE:
            removePeer(payload.sender_id);
            break;
        default:
            break;
    }
}

void GossipProtocol::handlePing(const GossipPayload& payload, const string& source) {
    GossipPayload pong;
    pong.type = GossipMessageType::PONG;
    pong.sender_id = m_peer_id;
    pong.incarnation = m_incarnation.load();
    
    sendToPeer(source, pong);
}

void GossipProtocol::handlePong(const GossipPayload& payload, const string& source) {
    updatePeerHeartbeat(source);
}

void GossipProtocol::handleMemberList(const GossipPayload& payload, const string& source) {
    // Update peer heartbeats from digest
    lock_guard<mutex> lock(m_peers_mutex);
    
    for (const auto& [peer_id, heartbeat] : payload.heartbeats) {
        auto it = m_peers.find(peer_id);
        if (it != m_peers.end()) {
            if (heartbeat > it->second.heartbeat) {
                it->second.heartbeat = heartbeat;
                it->second.last_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                it->second.state = PeerState::ALIVE;
            }
        }
    }
    
    updatePeerHeartbeat(source);
}

void GossipProtocol::addPeer(const PeerInfo& peer) {
    lock_guard<mutex> lock(m_peers_mutex);
    
    if (m_peers.size() >= m_config.max_peers) {
        return;
    }
    
    auto it = m_peers.find(peer.peer_id);
    if (it == m_peers.end()) {
        m_peers[peer.peer_id] = peer;
        m_peer_joins.fetch_add(1);
        
        if (m_peer_join_callback) {
            m_peer_join_callback(peer);
        }
        
        std::cout << "[GossipProtocol] Added peer: " << peer.peer_id << "\n";
    } else {
        // Update existing peer
        it->second.last_seen = peer.last_seen;
        it->second.heartbeat = peer.heartbeat;
    }
}

void GossipProtocol::removePeer(const string& peer_id) {
    lock_guard<mutex> lock(m_peers_mutex);
    auto it = m_peers.find(peer_id);
    if (it != m_peers.end()) {
        if (m_peer_leave_callback) {
            m_peer_leave_callback(it->second);
        }
        m_peers.erase(it);
        m_peer_leaves.fetch_add(1);
        std::cout << "[GossipProtocol] Removed peer: " << peer_id << "\n";
    }
}

void GossipProtocol::updatePeerHeartbeat(const string& peer_id) {
    lock_guard<mutex> lock(m_peers_mutex);
    auto it = m_peers.find(peer_id);
    if (it != m_peers.end()) {
        it->second.heartbeat = m_heartbeat.load();
        it->second.last_seen = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        it->second.state = PeerState::ALIVE;
    }
}

void GossipProtocol::selectRandomPeers(size_t count, vector<string>& out_peers) {
    lock_guard<mutex> lock(m_peers_mutex);
    
    if (m_peers.empty()) {
        return;
    }
    
    vector<string> peer_ids;
    peer_ids.reserve(m_peers.size());
    for (const auto& [id, peer] : m_peers) {
        if (peer.isAlive()) {
            peer_ids.push_back(id);
        }
    }
    
    // Shuffle and select
    std::shuffle(peer_ids.begin(), peer_ids.end(), m_rng);
    
    size_t select_count = std::min(count, peer_ids.size());
    out_peers.assign(peer_ids.begin(), peer_ids.begin() + select_count);
}

vector<uint8_t> GossipProtocol::serializeGossipPayload(const GossipPayload& payload) {
    vector<uint8_t> buffer;
    
    // Type
    uint32_t type = static_cast<uint32_t>(payload.type);
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&type),
                  reinterpret_cast<uint8_t*>(&type) + sizeof(type));
    
    // Sender ID length and data
    uint32_t sender_len = static_cast<uint32_t>(payload.sender_id.size());
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&sender_len),
                  reinterpret_cast<uint8_t*>(&sender_len) + sizeof(sender_len));
    buffer.insert(buffer.end(), payload.sender_id.begin(), payload.sender_id.end());
    
    // Incarnation
    uint64_t incarnation = payload.incarnation;
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&incarnation),
                  reinterpret_cast<uint8_t*>(&incarnation) + sizeof(incarnation));
    
    return buffer;
}

GossipPayload GossipProtocol::deserializeGossipPayload(const vector<uint8_t>& data) {
    GossipPayload payload;
    
    if (data.size() < sizeof(uint32_t)) {
        return payload;
    }
    
    size_t offset = 0;
    
    // Type
    uint32_t type;
    std::memcpy(&type, data.data() + offset, sizeof(type));
    payload.type = static_cast<GossipMessageType>(type);
    offset += sizeof(type);
    
    if (data.size() < offset + sizeof(uint32_t)) {
        return payload;
    }
    
    // Sender ID length
    uint32_t sender_len;
    std::memcpy(&sender_len, data.data() + offset, sizeof(sender_len));
    offset += sizeof(sender_len);
    
    if (data.size() < offset + sender_len) {
        return payload;
    }
    
    // Sender ID
    payload.sender_id.assign(data.begin() + offset, data.begin() + offset + sender_len);
    offset += sender_len;
    
    if (data.size() < offset + sizeof(uint64_t)) {
        return payload;
    }
    
    // Incarnation
    uint64_t incarnation;
    std::memcpy(&incarnation, data.data() + offset, sizeof(incarnation));
    payload.incarnation = incarnation;
    
    return payload;
}

void GossipProtocol::sendToPeer(const string& peer_id, const GossipPayload& payload) {
    auto data = serializeGossipPayload(payload);
    
    lock_guard<mutex> lock(m_peers_mutex);
    auto it = m_peers.find(peer_id);
    if (it != m_peers.end()) {
        // In real implementation, send via TCP socket
        m_messages_sent.fetch_add(1);
    }
}

void GossipProtocol::broadcastToRandomPeers(const GossipPayload& payload) {
    vector<string> peers;
    selectRandomPeers(m_config.gossip_fanout, peers);
    
    for (const auto& peer_id : peers) {
        sendToPeer(peer_id, payload);
    }
}

// =============================================================================
// GOSSIP NODE IMPLEMENTATION
// =============================================================================

GossipNode::GossipNode(const string& peer_id, const GossipConfig& config)
    : m_peer_id(peer_id)
    , m_protocol(GossipFactory::createProtocol(peer_id, config)) {}

GossipNode::~GossipNode() {
    stop();
}

void GossipNode::start() {
    m_protocol->start();
}

void GossipNode::stop() {
    m_protocol->stop();
}

void GossipNode::join(const string& address, Types::Port port) {
    m_protocol->joinPeer(address, port);
}

void GossipNode::leave() {
    m_protocol->leaveCluster();
}

void GossipNode::broadcast(const vector<uint8_t>& data) {
    Protocol::Message msg(Protocol::MessageType::DATA, data);
    m_protocol->broadcast(msg);
}

void GossipNode::setDataHandler(function<void(const string&, const vector<uint8_t>&)> handler) {
    m_data_handler = std::move(handler);
    
    m_protocol->setMessageHandler([this](const Protocol::Message& msg) {
        if (m_data_handler && msg.header().type == Protocol::MessageType::MSG_DATA) {
            m_data_handler(m_protocol->peerId(), msg.payload());
        }
    });
}

vector<PeerInfo> GossipNode::members() const {
    return m_protocol->getPeers();
}

string GossipNode::stats() const {
    return m_protocol->getStats();
}

// =============================================================================
// GOSSIP FACTORY IMPLEMENTATION
// =============================================================================

unique_ptr<GossipProtocol> GossipFactory::createProtocol(const string& peer_id,
                                                         const GossipConfig& config) {
    return std::make_unique<GossipProtocol>(peer_id, config);
}

unique_ptr<GossipNode> GossipFactory::createNode(const string& peer_id,
                                                 const GossipConfig& config) {
    return std::make_unique<GossipNode>(peer_id, config);
}

GossipConfig GossipFactory::defaultConfig() {
    return GossipConfig{};
}

string GossipFactory::generatePeerId() {
    static atomic<uint64_t> counter{0};
    return "peer_" + std::to_string(counter.fetch_add(1));
}

} // namespace MiniSonic::Gossip
