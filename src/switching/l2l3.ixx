module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ranges>
#include <span>
#include <optional>

export module MiniSonic.L2L3;

import MiniSonic.Core.Types;

// Import dependencies
import MiniSonic.SAI;
import MiniSonic.Core.Utils;
import MiniSonic.Core.Constants;
import MiniSonic.Core.Events;

export namespace MiniSonic::DataPlane {

/**
 * @brief Network packet representation
 *
 * Represents a network packet with L2 and L3 headers using modern C++23.
 */
export class Packet {
public:
    Types::MacAddress m_src_mac;
    Types::MacAddress m_dst_mac;
    Types::IpAddress m_src_ip;
    Types::IpAddress m_dst_ip;
    Types::PortId m_ingress_port;
    uint64_t m_id;

    std::chrono::high_resolution_clock::time_point timestamp;

    Packet() : m_id(0) {}

    Packet(
        Types::MacAddress src_mac,
        Types::MacAddress dst_mac,
        Types::IpAddress src_ip,
        Types::IpAddress dst_ip,
        Types::PortId ingress_port,
        uint64_t id = 0
    ) : m_src_mac(std::move(src_mac)),
        m_dst_mac(std::move(dst_mac)),
        m_src_ip(std::move(src_ip)),
        m_dst_ip(std::move(dst_ip)),
        m_ingress_port(ingress_port),
        m_id(id),
        timestamp(std::chrono::high_resolution_clock::now()) {}

    Packet(Packet&& other) noexcept = default;
    Packet& operator=(Packet&& other) noexcept = default;
    Packet(const Packet& other) = default;
    Packet& operator=(const Packet& other) = default;
    ~Packet() = default;

    [[nodiscard]] const Types::MacAddress& srcMac() const noexcept { return m_src_mac; }
    [[nodiscard]] const Types::MacAddress& dstMac() const noexcept { return m_dst_mac; }
    [[nodiscard]] const Types::IpAddress& srcIp() const noexcept { return m_src_ip; }
    [[nodiscard]] const Types::IpAddress& dstIp() const noexcept { return m_dst_ip; }
    [[nodiscard]] Types::PortId ingressPort() const noexcept { return m_ingress_port; }
    [[nodiscard]] uint64_t id() const noexcept { return m_id; }

    void setSrcMac(Types::MacAddress mac) { m_src_mac = std::move(mac); }
    void setDstMac(Types::MacAddress mac) { m_dst_mac = std::move(mac); }
    void setSrcIp(Types::IpAddress ip) { m_src_ip = std::move(ip); }
    void setDstIp(Types::IpAddress ip) { m_dst_ip = std::move(ip); }
    void setIngressPort(Types::PortId port) { m_ingress_port = port; }
    void setId(uint64_t id) { m_id = id; }

    void updateTimestamp() noexcept {
        timestamp = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] Events::PacketInfo toPacketInfo() const {
        return Events::PacketInfo{
            .id = m_id,
            .src_mac = formatMac(m_src_mac),
            .dst_mac = formatMac(m_dst_mac),
            .src_ip = formatIp(m_src_ip),
            .dst_ip = formatIp(m_dst_ip),
            .src_port = 0,
            .dst_port = 0,
            .protocol = "IPv4",
            .dscp = 0,
            .ttl = 64
        };
    }

private:
    static std::string formatMac(Types::MacAddress mac) {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                 static_cast<unsigned int>((mac >> 40) & 0xFF), static_cast<unsigned int>((mac >> 32) & 0xFF),
                 static_cast<unsigned int>((mac >> 24) & 0xFF), static_cast<unsigned int>((mac >> 16) & 0xFF),
                 static_cast<unsigned int>((mac >> 8) & 0xFF), static_cast<unsigned int>(mac & 0xFF));
        return std::string(buf);
    }

    static std::string formatIp(Types::IpAddress ip) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                 static_cast<unsigned int>((ip >> 24) & 0xFF), static_cast<unsigned int>((ip >> 16) & 0xFF),
                 static_cast<unsigned int>((ip >> 8) & 0xFF), static_cast<unsigned int>(ip & 0xFF));
        return std::string(buf);
    }
};

} // namespace MiniSonic::DataPlane

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
    explicit L2Service(::MiniSonic::SAI::SaiInterface& sai, const std::string& switch_id = "SWITCH0");
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

using std::unordered_map;
using std::mutex;

/**
 * @brief Longest Prefix Match (LPM) Trie
 *
 * Efficient data structure for IP route lookup using modern C++23.
 */
export class LpmTrie {
public:
    LpmTrie();
    ~LpmTrie() = default;

    // Route management
    void insert(Types::IpAddress network, Types::PrefixLength prefix_len, Types::IpAddress next_hop);
    void remove(Types::IpAddress network, Types::PrefixLength prefix_len);
    std::optional<Types::IpAddress> lookup(Types::IpAddress ip) const;

    // Utility methods
    size_t size() const;
    void clear();
    std::string getStats() const;

private:
    struct TrieNode {
        std::unique_ptr<TrieNode> children[2];
        Types::IpAddress next_hop = 0;
        bool is_route = false;
    };

    std::unique_ptr<TrieNode> m_root;

    static std::vector<bool> ipToBinary(Types::IpAddress ip);
};

/**
 * @brief L3 routing service
 *
 * Handles IP routing and forwarding decisions using modern C++23.
 */
export class L3Service {
public:
    explicit L3Service(::MiniSonic::SAI::SaiInterface& sai, const std::string& switch_id = "SWITCH0");
    ~L3Service() = default;

    // Public interface
    bool handle(MiniSonic::DataPlane::Packet& pkt);
    bool addRoute(Types::IpAddress network, Types::PrefixLength prefix_len, Types::IpAddress next_hop);
    bool removeRoute(Types::IpAddress network, Types::PrefixLength prefix_len);
    std::string getStats() const;

    /**
     * @brief Set switch ID for event emission
     */
    void setSwitchId(const std::string& switch_id) { m_switch_id = switch_id; }

private:
    SAI::SaiInterface& m_sai;
    std::string m_switch_id;
    Events::EventBus& m_event_bus;
    std::unique_ptr<LpmTrie> m_lpm_trie;
    Types::RoutingTable m_routes; // network -> next_hop
    mutable mutex m_routes_mutex;

    bool isLocalIp(Types::IpAddress ip) const;
    bool forwardPacket(MiniSonic::DataPlane::Packet& pkt);
};

} // export namespace MiniSonic::L3
