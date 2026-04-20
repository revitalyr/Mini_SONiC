/**
 * @file packet_enhanced.ixx
 * @brief Enhanced Packet structure with tracing support and full metadata
 * 
 * Uses MiniSonic.Boost.Wrappers module for:
 * - UUID for packet identification
 * - Optional fields
 * 
 * Fixes identified issues:
 * - Added packet_id for end-to-end tracing
 * - Extended metadata (VLAN, DSCP, protocol, etc.)
 * - Pipeline stage tracking
 */

export module MiniSonic.DataPlane.PacketEnhanced;

import MiniSonic.Common.Types;
import MiniSonic.Boost.Wrappers;

import <chrono>;
import <string>;
import <sstream>;
import <vector>;
import <cstdint>;

export namespace MiniSonic::DataPlane {

using namespace MiniSonic::BoostWrapper;

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
    Uuid packet_id;                         ///< Unique packet identifier
    uint64_t sequence_number;                ///< Global sequence number
    
    // === L2 Information ===
    Types::MacAddress m_src_mac;
    Types::MacAddress m_dst_mac;
    Optional<uint16_t> m_vlan_id;            ///< VLAN tag (if present)
    
    // === L3 Information ===
    Types::IpAddress m_src_ip;
    Types::IpAddress m_dst_ip;
    ProtocolType m_protocol;
    uint8_t m_tos;                           ///< Type of Service / DSCP
    uint8_t m_ttl;
    Optional<uint16_t> m_src_port;           ///< L4 source port (if applicable)
    Optional<uint16_t> m_dst_port;           ///< L4 destination port (if applicable)
    
    // === Pipeline Metadata ===
    Types::Port m_ingress_port;
    Optional<Types::Port> m_egress_port;     ///< Determined by forwarding
    
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
        : packet_id(generateUuid())
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
        Types::MacAddress src_mac,
        Types::MacAddress dst_mac,
        Types::IpAddress src_ip,
        Types::IpAddress dst_ip,
        Types::Port ingress_port,
        ProtocolType protocol = ProtocolType::UNKNOWN
    ) : packet_id(generateUuid())
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
    [[nodiscard]] const Types::MacAddress& srcMac() const noexcept { return m_src_mac; }
    [[nodiscard]] const Types::MacAddress& dstMac() const noexcept { return m_dst_mac; }
    [[nodiscard]] const Types::IpAddress& srcIp() const noexcept { return m_src_ip; }
    [[nodiscard]] const Types::IpAddress& dstIp() const noexcept { return m_dst_ip; }
    [[nodiscard]] Types::Port ingressPort() const noexcept { return m_ingress_port; }
    [[nodiscard]] Optional<Types::Port> egressPort() const noexcept { return m_egress_port; }
    [[nodiscard]] ProtocolType protocol() const noexcept { return m_protocol; }
    [[nodiscard]] uint8_t ttl() const noexcept { return m_ttl; }
    [[nodiscard]] uint8_t dscp() const noexcept { return m_tos >> 2; }
    [[nodiscard]] Optional<uint16_t> vlanId() const noexcept { return m_vlan_id; }
    
    // === Setters ===
    void setSrcMac(Types::MacAddress mac) { m_src_mac = std::move(mac); }
    void setDstMac(Types::MacAddress mac) { m_dst_mac = std::move(mac); }
    void setSrcIp(Types::IpAddress ip) { m_src_ip = std::move(ip); }
    void setDstIp(Types::IpAddress ip) { m_dst_ip = std::move(ip); }
    void setIngressPort(Types::Port port) { m_ingress_port = port; }
    void setEgressPort(Types::Port port) { m_egress_port = port; }
    void setProtocol(ProtocolType proto) { m_protocol = proto; }
    void setTtl(uint8_t ttl) { m_ttl = ttl; }
    void setDscp(uint8_t dscp) { m_tos = (dscp << 2); }
    void setVlanId(uint16_t vlan) { m_vlan_id = vlan; }
    void setSrcPort(uint16_t port) { m_src_port = port; }
    void setDstPort(uint16_t port) { m_m_dst_port = port; }
    
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
        oss << "Packet[" << uuidToString(packet_id) << "] "
            << "seq=" << sequence_number
            << " " << srcMac().toString() << "->" << dstMac().toString()
            << " " << srcIp() << "->" << dstIp()
            << " stage=" << static_cast<int>(current_stage)
            << " latency=" << getLatencyUsec().count() << "us";
        if (is_dropped) {
            oss << " [DROPPED: " << drop_reason << "]";
        }
        return oss.str();
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
    AtomicUint64 m_counter{0};
};

/**
 * @brief Helper to assign sequence number to packet
 */
export inline void assignSequenceNumber(EnhancedPacket& pkt) {
    pkt.sequence_number = PacketSequenceCounter::instance().next();
}

} // namespace MiniSonic::DataPlane
