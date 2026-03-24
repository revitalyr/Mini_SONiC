module;

// Import the module we're implementing
import MiniSonic.L2L3;

// Import dependencies
import <memory>;
import <string>;
import <vector>;
import <unordered_map>;
import <atomic>;
import <iostream>;
import <sstream>;
import <algorithm>;
import <mutex>;
import <chrono>;

export namespace MiniSonic::L2 {

// L2Service Implementation
L2Service::L2Service(SAI::SaiInterface& sai) : m_sai(sai) {
    std::cout << "[L2] Initializing L2 service\n";
}

bool L2Service::handle(Packet& pkt) {
    // Learn source MAC
    learn(pkt.srcMac(), pkt.ingressPort());
    
    // Forward based on destination MAC
    return forwardPacket(pkt);
}

void L2Service::learn(const Types::MacAddress& mac, Types::Port port) {
    std::lock_guard<std::mutex> lock(m_fdb_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = m_fdb.find(mac);
    
    if (it != m_fdb.end()) {
        // Update existing entry
        it->second.last_seen = now;
    } else {
        // Add new entry
        MacEntry entry;
        entry.port = port;
        entry.learned_at = now;
        entry.last_seen = now;
        
        m_fdb[mac] = entry;
        std::cout << "[L2] Learned MAC " << mac << " on port " << port << "\n";
    }
}

std::string L2Service::getStats() const {
    std::lock_guard<std::mutex> lock(m_fdb_mutex);
    
    std::ostringstream oss;
    oss << "L2 Service Statistics:\n"
         << "  FDB Size: " << m_fdb.size() << " entries\n";
    
    // Count entries per port
    std::unordered_map<Types::Port, size_t> port_counts;
    for (const auto& [mac, entry] : m_fdb) {
        port_counts[entry.port]++;
    }
    
    for (const auto& [port, count] : port_counts) {
        oss << "  Port " << port << ": " << count << " MACs\n";
    }
    
    return oss.str();
}

void L2Service::cleanupOldEntries() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - MAC_AGE_TIMEOUT;
    
    std::lock_guard<std::mutex> lock(m_fdb_mutex);
    
    auto it = m_fdb.begin();
    while (it != m_fdb.end()) {
        if (it->second.last_seen < cutoff) {
            it = m_fdb.erase(it);
        } else {
            ++it;
        }
    }
}

bool L2Service::forwardPacket(Packet& pkt) {
    std::lock_guard<std::mutex> lock(m_fdb_mutex);
    
    auto it = m_fdb.find(pkt.dstMac());
    
    if (it != m_fdb.end()) {
        // Found in FDB - forward to specific port
        Types::Port out_port = it->second.port;
        
        // Check if port is up
        if (m_sai.getPortState(out_port)) {
            std::cout << "[L2] Forwarding to port " << out_port << "\n";
            return true;
        } else {
            std::cerr << "[L2] Output port " << out_port << " is down\n";
            return false;
        }
    } else {
        // Not found - flood to all ports except ingress
        std::cout << "[L2] Flooding packet (unknown destination MAC)\n";
        return true;
    }
}

} // export namespace MiniSonic::L2

export namespace MiniSonic::L3 {

// LpmTrie Implementation
void LpmTrie::insert(const std::string& network, int prefix_len, const std::string& next_hop) {
    auto binary = ipToBinary(network);
    
    TrieNode* node = m_root.get();
    if (!node) {
        m_root = std::make_unique<TrieNode>();
        node = m_root.get();
    }
    
    for (int i = 0; i < prefix_len && i < static_cast<int>(binary.size()); ++i) {
        int bit = binary[i] ? 1 : 0;
        
        if (!node->children[bit]) {
            node->children[bit] = std::make_unique<TrieNode>();
        }
        node = node->children[bit].get();
    }
    
    node->next_hop = next_hop;
    node->is_route = true;
    
    std::cout << "[L3] Added route " << network << "/" << prefix_len 
              << " -> " << next_hop << "\n";
}

void LpmTrie::remove(const std::string& network, int prefix_len) {
    auto binary = ipToBinary(network);
    
    // Simple implementation - just mark as non-route
    TrieNode* node = m_root.get();
    if (!node) return;
    
    for (int i = 0; i < prefix_len && i < static_cast<int>(binary.size()); ++i) {
        int bit = binary[i] ? 1 : 0;
        
        if (!node->children[bit]) {
            return; // Route doesn't exist
        }
        node = node->children[bit].get();
    }
    
    node->is_route = false;
    node->next_hop.clear();
    
    std::cout << "[L3] Removed route " << network << "/" << prefix_len << "\n";
}

std::string LpmTrie::lookup(const std::string& ip) const {
    auto binary = ipToBinary(ip);
    
    std::string best_match;
    const TrieNode* node = m_root.get();
    
    for (size_t i = 0; i < binary.size() && node; ++i) {
        if (node->is_route && !node->next_hop.empty()) {
            best_match = node->next_hop;
        }
        
        int bit = binary[i] ? 1 : 0;
        node = node->children[bit].get();
    }
    
    return best_match;
}

size_t LpmTrie::size() const {
    // Simple implementation - count routes
    std::function<size_t(const TrieNode*)> count_routes = [&](const TrieNode* node) -> size_t {
        if (!node) return 0;
        
        size_t count = node->is_route ? 1 : 0;
        count += count_routes(node->children[0].get());
        count += count_routes(node->children[1].get());
        return count;
    };
    
    return count_routes(m_root.get());
}

void LpmTrie::clear() {
    m_root.reset();
}

std::string LpmTrie::getStats() const {
    std::ostringstream oss;
    oss << "LPM Trie Statistics:\n"
         << "  Routes: " << size() << "\n";
    return oss.str();
}

std::vector<bool> LpmTrie::ipToBinary(const std::string& ip) {
    std::vector<bool> binary;
    binary.reserve(32);
    
    std::istringstream iss(ip);
    std::string octet;
    
    while (std::getline(iss, octet, '.')) {
        int value = std::stoi(octet);
        
        for (int i = 7; i >= 0; --i) {
            binary.push_back((value >> i) & 1);
        }
    }
    
    return binary;
}

// L3Service Implementation
L3Service::L3Service(SAI::SaiInterface& sai) : m_sai(sai) {
    std::cout << "[L3] Initializing L3 service\n";
    
    // Add default route
    addRoute("0.0.0.0", 0, "192.168.1.1");
}

bool L3Service::handle(Packet& pkt) {
    // Check if packet is for local processing
    if (isLocalIp(pkt.dstIp())) {
        std::cout << "[L3] Packet for local IP " << pkt.dstIp() << "\n";
        return true;
    }
    
    // Route the packet
    return forwardPacket(pkt);
}

bool L3Service::addRoute(const std::string& network, int prefix_len, const std::string& next_hop) {
    std::lock_guard<std::mutex> lock(m_routes_mutex);
    
    m_routing_table.insert(network, prefix_len, next_hop);
    m_routes[network + "/" + std::to_string(prefix_len)] = next_hop;
    
    return true;
}

bool L3Service::removeRoute(const std::string& network, int prefix_len) {
    std::lock_guard<std::mutex> lock(m_routes_mutex);
    
    m_routing_table.remove(network, prefix_len);
    m_routes.erase(network + "/" + std::to_string(prefix_len));
    
    return true;
}

std::string L3Service::getStats() const {
    std::lock_guard<std::mutex> lock(m_routes_mutex);
    
    std::ostringstream oss;
    oss << "L3 Service Statistics:\n"
         << m_routing_table.getStats()
         << "  Configured Routes: " << m_routes.size() << "\n";
    
    return oss.str();
}

bool L3Service::isLocalIp(const std::string& ip) const {
    // Simple implementation - check if IP is in local subnet
    // In real implementation, this would check interface addresses
    return ip.find("10.1.1.") == 0 || ip.find("192.168.") == 0;
}

bool L3Service::forwardPacket(Packet& pkt) {
    std::string next_hop = m_routing_table.lookup(pkt.dstIp());
    
    if (next_hop.empty()) {
        std::cerr << "[L3] No route found for " << pkt.dstIp() << "\n";
        return false;
    }
    
    std::cout << "[L3] Forwarding to next-hop " << next_hop << "\n";
    return true;
}

} // export namespace MiniSonic::L3
