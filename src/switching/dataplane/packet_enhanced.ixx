/**
 * @file packet_enhanced.ixx
 * @brief Enhanced Packet structure with tracing support and full metadata
 *
 * Uses C++ standard library for:
 * - std::string for packet identification
 * - std::optional for nullable fields
 *
 * Fixes identified issues:
 * - Transitioned from Boost.Wrappers to std library types
 * - Extended metadata (VLAN, DSCP, protocol, etc.)
 * - Pipeline stage tracking
 */

module;

#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <atomic>
#include <optional>
#include <utility>

export module MiniSonic.DataPlane.PacketEnhanced;

export namespace MiniSonic::DataPlane {

/**
 * @brief Protocol types for packet classification
 */
export enum class ProtocolType : uint8_t {
    UNKNOWN = 0,
    ARP = 1,
    ICMP = 2,
    TCP = 3,
    UDP = 4,
    BGP = 5,
    LLDP = 6,
    IPV4 = 7,
    IPV6 = 8
};

/**
 * @brief Pipeline stage identifiers for tracing
 */
export enum class PipelineStage : uint8_t {
    INGRESS = 0,
    PARSER = 1,
    L2_LOOKUP = 2,
    L3_LOOKUP = 3,
    ACL = 4,
    QUEUEING = 5,
    REWRITE = 6,
    EGRESS = 7,
    DROPPED = 8,
    COMPLETED = 9
};

/**
 * @brief Enhanced packet with full metadata and tracing support
 * 
 * Uses Boost.UUID via MiniSonic.Boost.Wrappers for unique packet identification.
 * Tracks packet through all pipeline stages.
 */
export struct EnhancedPacket {
    // === Core Identification ===
    std::string packet_id;                   ///< Unique packet identifier
    uint64_t sequence_number;                ///< Global sequence number

    // === L2 Information ===
    uint64_t m_src_mac;                     ///< MAC address (using primitive type)
    uint64_t m_dst_mac;                     ///< MAC address (using primitive type)
    std::optional<uint16_t> m_vlan_id;       ///< VLAN tag (if present)

    // === L3 Information ===
    uint32_t m_src_ip;                      ///< IP address (using primitive type)
    uint32_t m_dst_ip;                      ///< IP address (using primitive type)
    ProtocolType m_protocol;
    uint8_t m_tos;                           ///< Type of Service / DSCP
    uint8_t m_ttl;
    std::optional<uint16_t> m_src_port;      ///< L4 source port (if applicable)
    std::optional<uint16_t> m_dst_port;      ///< L4 destination port (if applicable)

    // === Pipeline Metadata ===
    uint16_t m_ingress_port;                ///< Port ID (using primitive type)
    std::optional<uint16_t> m_egress_port;   ///< Port ID (using primitive type)
    
    // === Tracing Information ===
    PipelineStage current_stage;
    std::vector<std::pair<PipelineStage, std::chrono::steady_clock::time_point>> stage_history;
    
    // === Timestamping ===
    std::chrono::steady_clock::time_point ingress_timestamp;
    std::chrono::steady_clock::time_point egress_timestamp;
    
    // === Processing State ===
    bool is_dropped;
    std::string drop_reason;
    uint32_t hash;                           ///< ECMP/LAG hash
    
    // === Default Constructor ===
    EnhancedPacket()
        : packet_id(generatePacketId())
        , sequence_number(0)
        , m_protocol(ProtocolType::UNKNOWN)
        , m_tos(0)
        , m_ttl(64)
        , m_ingress_port(0)
        , current_stage(PipelineStage::INGRESS)
        , is_dropped(false)
        , hash(0) {
        ingress_timestamp = std::chrono::steady_clock::now();
    }
    
    // === Parameterized Constructor ===
    EnhancedPacket(
        uint64_t src_mac,
        uint64_t dst_mac,
        uint32_t src_ip,
        uint32_t dst_ip,
        uint16_t ingress_port,
        ProtocolType protocol = ProtocolType::UNKNOWN
    ) : packet_id(generatePacketId())
        , sequence_number(0)
        , m_src_mac(std::move(src_mac))
        , m_dst_mac(std::move(dst_mac))
        , m_src_ip(std::move(src_ip))
        , m_dst_ip(std::move(dst_ip))
        , m_protocol(protocol)
        , m_tos(0)
        , m_ttl(64)
        , m_ingress_port(ingress_port)
        , current_stage(PipelineStage::INGRESS)
        , is_dropped(false)
        , hash(0) {
        ingress_timestamp = std::chrono::steady_clock::now();
        recordStage(PipelineStage::INGRESS);
    }
    
    // === Move Constructor ===
    EnhancedPacket(EnhancedPacket&& other) noexcept = default;
    
    // === Move Assignment ===
    EnhancedPacket& operator=(EnhancedPacket&& other) noexcept = default;
    
    // === Copy Constructor ===
    EnhancedPacket(const EnhancedPacket& other) = default;
    
    // === Copy Assignment ===
    EnhancedPacket& operator=(const EnhancedPacket& other) = default;
    
    // === Destructor ===
    ~EnhancedPacket() = default;
    
    // === Getters ===
    [[nodiscard]] uint64_t srcMac() const noexcept { return m_src_mac; }
    [[nodiscard]] uint64_t dstMac() const noexcept { return m_dst_mac; }
    [[nodiscard]] uint32_t srcIp() const noexcept { return m_src_ip; }
    [[nodiscard]] uint32_t dstIp() const noexcept { return m_dst_ip; }
    [[nodiscard]] uint16_t ingressPort() const noexcept { return m_ingress_port; }
    [[nodiscard]] std::optional<uint16_t> egressPort() const noexcept { return m_egress_port; }
    [[nodiscard]] ProtocolType protocol() const noexcept { return m_protocol; }
    [[nodiscard]] uint8_t ttl() const noexcept { return m_ttl; }
    [[nodiscard]] uint8_t dscp() const noexcept { return m_tos >> 2; }
    [[nodiscard]] std::optional<uint16_t> vlanId() const noexcept { return m_vlan_id; }
    [[nodiscard]] std::optional<uint16_t> srcPort() const noexcept { return m_src_port; }
    [[nodiscard]] std::optional<uint16_t> dstPort() const noexcept { return m_dst_port; }

    // === Setters ===
    void setSrcMac(uint64_t mac) { m_src_mac = mac; }
    void setDstMac(uint64_t mac) { m_dst_mac = mac; }
    void setSrcIp(uint32_t ip) { m_src_ip = ip; }
    void setDstIp(uint32_t ip) { m_dst_ip = ip; }
    void setIngressPort(uint16_t port) { m_ingress_port = port; }
    void setEgressPort(uint16_t port) { m_egress_port = port; }
    void setProtocol(ProtocolType proto) { m_protocol = proto; }
    void setTtl(uint8_t ttl) { m_ttl = ttl; }
    void setDscp(uint8_t dscp) { m_tos = (dscp << 2); }
    void setVlanId(uint16_t vlan) { m_vlan_id = vlan; }
    void setSrcPort(uint16_t port) { m_src_port = port; }
    void setDstPort(uint16_t port) { m_dst_port = port; }
    
    // === Pipeline Stage Tracking ===
    void recordStage(PipelineStage stage) {
        current_stage = stage;
        stage_history.emplace_back(stage, std::chrono::steady_clock::now());
    }
    
    [[nodiscard]] PipelineStage getCurrentStage() const noexcept {
        return current_stage;
    }
    
    [[nodiscard]] std::chrono::microseconds getLatencyUsec() const {
        auto end = is_dropped || current_stage == PipelineStage::COMPLETED 
            ? egress_timestamp 
            : std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            end - ingress_timestamp);
    }
    
    // === Drop Handling ===
    void markDropped(std::string reason) {
        is_dropped = true;
        drop_reason = std::move(reason);
        egress_timestamp = std::chrono::steady_clock::now();
        recordStage(PipelineStage::DROPPED);
    }
    
    void markCompleted() {
        egress_timestamp = std::chrono::steady_clock::now();
        recordStage(PipelineStage::COMPLETED);
    }
    
    // === Serialization for Debug ===
    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << "Packet[" << packet_id << "] "
            << "seq=" << sequence_number
            << " mac=" << std::hex << m_src_mac << "->" << m_dst_mac
            << " ip=" << std::dec << m_src_ip << "->" << m_dst_ip
            << " port=" << m_ingress_port
            << " stage=" << static_cast<int>(current_stage)
            << " latency=" << getLatencyUsec().count() << "us";
        if (is_dropped) {
            oss << " [DROPPED: " << drop_reason << "]";
        }
        return oss.str();
    }

private:
    static std::string generatePacketId() {
        static std::atomic<uint64_t> s_id_counter{0};
        return "pkt-" + std::to_string(s_id_counter.fetch_add(1, std::memory_order_relaxed));
    }
};

/**
 * @brief Global packet sequence counter using Boost.Atomic
 */
export class PacketSequenceCounter {
public:
    static PacketSequenceCounter& instance() {
        static PacketSequenceCounter instance;
        return instance;
    }
    
    uint64_t next() {
        return ++m_counter;
    }
    
    uint64_t current() const {
        return m_counter.load();
    }
    
private:
    PacketSequenceCounter() = default;
    std::atomic<uint64_t> m_counter{0};
};

/**
 * @brief Helper to assign sequence number to packet
 */
export inline void assignSequenceNumber(EnhancedPacket& pkt) {
    pkt.sequence_number = PacketSequenceCounter::instance().next();
}

} // namespace MiniSonic::DataPlane
