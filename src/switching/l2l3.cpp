module;

#include <memory> // For std::unique_ptr, std::shared_ptr
#include <string> // For std::string
#include <vector> // For std::vector
#include <unordered_map> // For std::unordered_map
#include <iostream> // For std::cout, std::cerr
#include <sstream> // For std::ostringstream
#include <mutex> // For std::mutex, std::lock_guard
#include <chrono> // For std::chrono
#include "core/common/types.hpp"

module MiniSonic.L2L3;

// Import Packet type from DataPlane module
using MiniSonic::DataPlane::Packet;

// Import Events module
import MiniSonic.Core.Events;

namespace MiniSonic::L2 {
// L2Service Implementation
L2Service::L2Service(SAI::SaiInterface& sai, const std::string& switch_id)
    : m_sai(sai),
      m_switch_id(switch_id),
      m_event_bus(Events::getGlobalEventBus()),
      m_fdb_mutex() { // Initialize the mutex
    std::cout << "[L2] Initializing L2 service for " << m_switch_id << "\n";
}

bool L2Service::handle(Packet& pkt) {
    // Learn source MAC
    learn(pkt.srcMac(), pkt.ingressPort());
    
    // Forward based on destination MAC
    return forwardPacket(pkt);
}

void L2Service::learn(const Types::MacAddress& mac, Types::PortId port) {
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
        // std::cout << "[L2] Learned MAC " << mac << " on port " << port << "\n";
    }
}

std::string L2Service::getStats() const {
    std::lock_guard<std::mutex> lock(m_fdb_mutex);
    
    std::ostringstream oss;
    oss << "L2 Service Statistics:\n"
         << "  FDB Size: " << m_fdb.size() << " entries\n";
    
    // Count entries per port
    std::unordered_map<Types::PortId, size_t> port_counts;
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
    
    const auto dst_mac = pkt.dstMac();
    auto it = m_fdb.find(dst_mac);
    
    if (it != m_fdb.end()) {
        // Found in FDB - forward to specific port
        Types::PortId out_port = it->second.port;
        
        // Check if port is up
        if (m_sai.getPortState(out_port)) {
            // std::cout << "[L2] Forwarding " << dst_mac << " to port " << out_port << "\n";
            return true;
        } else {
            // std::cerr << "[L2] Output port " << out_port << " is down\n";
            return false;
        }
    } else {
        // Not found - flood to all ports except ingress
        // std::cout << "[L2] Flooding packet to " << dst_mac << " (unknown destination)\n";
        return true;
    }
}

} // export namespace MiniSonic::L2

namespace MiniSonic::L3 {
LpmTrie::LpmTrie() : m_root(std::make_unique<TrieNode>()) {}
// LpmTrie Implementation
void LpmTrie::insert(Types::IpAddress network, Types::PrefixLength prefix_len, Types::IpAddress next_hop) {
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
}

void LpmTrie::remove(Types::IpAddress network, Types::PrefixLength prefix_len) {
    auto binary = ipToBinary(network);

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
    node->next_hop = 0;
}

std::optional<Types::IpAddress> LpmTrie::lookup(Types::IpAddress ip) const {
    auto binary = ipToBinary(ip);

    std::optional<Types::IpAddress> best_match;
    const TrieNode* node = m_root.get();

    for (size_t i = 0; i < binary.size() && node; ++i) {
        if (node->is_route) {
            best_match = node->next_hop; // Already uint32_t
        }

        int bit = binary[i] ? 1 : 0;
        node = node->children[bit].get();
    }

    return best_match;
}

size_t LpmTrie::size() const {
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

std::vector<bool> LpmTrie::ipToBinary(Types::IpAddress ip) {
    std::vector<bool> binary;
    binary.reserve(32);
    
    for (int i = 31; i >= 0; --i) {
        binary.push_back((ip >> i) & 1);
    }
    
    return binary;
}

// L3Service Implementation
L3Service::L3Service(SAI::SaiInterface& sai, const std::string& switch_id)
    : m_sai(sai),
      m_switch_id(switch_id), // Initialize m_switch_id
      m_event_bus(Events::getGlobalEventBus()), // Initialize m_event_bus
      m_lpm_trie(std::make_unique<LpmTrie>()) { // Initialize m_lpm_trie
    std::cout << "[L3] Initializing L3 service for " << m_switch_id << "\n";
    
    // Add default route using numeric types
    addRoute(0, 0, MiniSonic::Types::ipToUint32("192.168.1.1"));
}

bool L3Service::handle(Packet& pkt) {
    // Check if packet is for local processing
    if (isLocalIp(pkt.dstIp())) {
        return true;
    }
    
    // Route the packet
    return forwardPacket(pkt);
}

bool L3Service::addRoute(Types::IpAddress network, Types::PrefixLength prefix_len, Types::IpAddress next_hop) {
    std::lock_guard<std::mutex> lock(m_routes_mutex);
    
    m_lpm_trie->insert(network, prefix_len, next_hop);
    m_routes[network] = next_hop;
    
    return true;
}

bool L3Service::removeRoute(Types::IpAddress network, Types::PrefixLength prefix_len) {
    std::lock_guard<std::mutex> lock(m_routes_mutex);

    m_lpm_trie->remove(network, prefix_len);
    m_routes.erase(network);
    
    return true;
}

std::string L3Service::getStats() const {
    std::lock_guard<std::mutex> lock(m_routes_mutex);
    
    std::ostringstream oss;
    oss << "L3 Service Statistics:\n";
    oss << m_lpm_trie->getStats()
         << "  Configured Routes: " << m_routes.size() << "\n";
    
    return oss.str();
}

bool L3Service::isLocalIp(Types::IpAddress ip) const {
    // Simple implementation - check if IP is in local subnet
    // Example: 10.1.1.0/24 or 192.168.0.0/16
    return (ip & 0xFFFFFF00) == 0x0A010100 || (ip & 0xFFFF0000) == 0xC0A80000;
}

bool L3Service::forwardPacket(Packet& pkt) {
    auto next_hop = m_lpm_trie->lookup(pkt.dstIp());

    if (!next_hop.has_value()) {
        return false;
    }

    // std::cout << "[L3] Forwarding to next-hop " << next_hop.value() << "\n";
    return true;
}

} // namespace MiniSonic::L3
