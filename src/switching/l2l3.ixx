module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ranges>
#include <span>
#include <optional>

export module MiniSonic.L2L3;

// Import dependencies
import MiniSonic.DataPlane;
import MiniSonic.SAI;

// Import Utils module for Types namespace
import MiniSonic.Core.Utils;

// Import Constants module
import MiniSonic.Core.Constants;

// Import Events module for visualization events
import MiniSonic.Core.Events;

export namespace MiniSonic::L2 {

// Using declarations for standard library types
using std::vector;
using std::unordered_map;
using std::mutex;
using std::chrono::steady_clock;
using std::chrono::seconds;
namespace chrono = std::chrono;

/**
 * @brief L2 switching service
 *
 * Handles MAC address learning and forwarding decisions.
 */
export class L2Service {
public:
    explicit L2Service(SAI::SaiInterface& sai, const std::string& switch_id = "SWITCH0");
    ~L2Service() = default;

    // Public interface
    bool handle(MiniSonic::DataPlane::Packet& pkt);
    std::string getStats() const;

    /**
     * @brief Set switch ID for event emission
     */
    void setSwitchId(const std::string& switch_id) { m_switch_id = switch_id; }

private:
    struct MacEntry {
        Types::PortId port;  // semantic alias
        // Note: `mutable` allows modification in const methods if needed for stats
        chrono::steady_clock::time_point learned_at;
        chrono::steady_clock::time_point last_seen;
    };

    SAI::SaiInterface& m_sai;
    std::string m_switch_id;
    Events::EventBus& m_event_bus;
    unordered_map<Types::MacAddress, MacEntry> m_fdb;
    mutable mutex m_fdb_mutex;

    static constexpr chrono::seconds MAC_AGE_TIMEOUT{Constants::MAC_AGE_TIMEOUT_SEC}; // 5 minutes

    void learn(const Types::MacAddress& mac, Types::PortId port);  // semantic alias
    void cleanupOldEntries();
    bool forwardPacket(MiniSonic::DataPlane::Packet& pkt);
};

} // export namespace MiniSonic::L2

export namespace MiniSonic::L3 {

/**
 * @brief Longest Prefix Match (LPM) Trie
 *
 * Efficient data structure for IP route lookup using modern C++23.
 */
export class LpmTrie {
public:
    LpmTrie() = default;
    ~LpmTrie() = default;

    // Route management
    void insert(const std::string& network, int prefix_len, const std::string& next_hop);
    void remove(const std::string& network, int prefix_len);
    std::optional<std::string> lookup(const std::string& ip) const;

    // Utility methods
    size_t size() const;
    void clear();
    std::string getStats() const;

private:
    struct TrieNode {
        std::unique_ptr<TrieNode> children[2];
        std::string next_hop;
        bool is_route = false;
    };

    std::unique_ptr<TrieNode> m_root;

    static std::vector<bool> ipToBinary(const std::string& ip);
    static std::string binaryToIp(std::span<const bool> binary, int prefix_len);
};

/**
 * @brief L3 routing service
 *
 * Handles IP routing and forwarding decisions using modern C++23.
 */
export class L3Service {
public:
    explicit L3Service(SAI::SaiInterface& sai, const std::string& switch_id = "SWITCH0");
    ~L3Service() = default;

    // Public interface
    bool handle(MiniSonic::DataPlane::Packet& pkt);
    bool addRoute(const std::string& network, int prefix_len, const std::string& next_hop);
    bool removeRoute(const std::string& network, int prefix_len);
    std::string getStats() const;

    /**
     * @brief Set switch ID for event emission
     */
    void setSwitchId(const std::string& switch_id) { m_switch_id = switch_id; }

private:
    SAI::SaiInterface& m_sai;
    std::string m_switch_id;
    Events::EventBus& m_event_bus;
    LpmTrie m_routing_table;
    unordered_map<std::string, std::string> m_routes; // network -> next_hop
    mutable mutex m_routes_mutex;

    bool isLocalIp(const std::string& ip) const;
    bool forwardPacket(MiniSonic::DataPlane::Packet& pkt);
};

} // export namespace MiniSonic::L3
