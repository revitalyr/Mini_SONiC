/**
 * @file pipeline_tracing.ixx
 * @brief Pipeline Event Tracing - Real packet flow events for visualization
 *
 * Uses Boost.Asio for async event delivery to visualization.
 * Each packet generates trace events as it flows through the pipeline stages.
 *
 * Events generated:
 * - packet_generated: New packet created
 * - packet_entered: Packet arrived at switch port
 * - packet_forward: Forwarding decision made
 * - packet_dropped: Packet was dropped
 * - packet_exited: Packet left switch
 *
 * Uses Boost.UUID for packet identification.
 */

module;

#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <optional>
#include <boost/asio.hpp>

export module MiniSonic.DataPlane.PipelineTracing;

import MiniSonic.DataPlane.PacketEnhanced;

export namespace MiniSonic::DataPlane {

/**
 * @brief Pipeline event types matching visualization schema
 */
export enum class PipelineEventType : uint8_t {
    PACKET_GENERATED = 0,       ///< New packet created at host
    PACKET_ENTERED_SWITCH = 1,  ///< Packet arrived at switch ingress port
    PACKET_FORWARD_DECISION = 2, ///< Forwarding decision made (L2/L3 lookup)
    PACKET_DROPPED = 3,         ///< Packet dropped by ACL, TTL, etc.
    PACKET_EXITED_SWITCH = 4,    ///< Packet left switch egress port
    ARP_REQUEST = 5,            ///< ARP request sent
    ARP_REPLY = 6,              ///< ARP reply received
    L2_LOOKUP_HIT = 7,          ///< MAC table lookup hit
    L2_LOOKUP_MISS = 8,         ///< MAC table lookup miss (flood)
    L3_LOOKUP_HIT = 9,          ///< Route lookup hit
    L3_LOOKUP_MISS = 10,       ///< Route lookup miss (drop)
    QUEUE_ENQUEUED = 11,        ///< Packet queued in output queue
    QUEUE_DEQUEUED = 12         ///< Packet dequeued for transmission
};

/**
 * @brief Single pipeline event for a packet
 */
export struct PipelineEvent {
    // Event identification
    std::string event_id;                ///< Unique event ID
    std::string packet_id;               ///< Related packet ID
    
    // Event metadata
    PipelineEventType type;
    std::string switch_name;             ///< Switch where event occurred
    std::chrono::steady_clock::time_point timestamp;
    
    // Port information
    std::optional<Types::PortId> ingress_port;
    std::optional<Types::PortId> egress_port;
    std::string next_hop_switch;         ///< For inter-switch links
    
    // Packet metadata at event time
    std::optional<uint16_t> vlan_id;
    std::optional<uint8_t> dscp;
    uint8_t ttl;
    ProtocolType protocol;
    
    // Event-specific data
    std::string reason;                  ///< Drop reason, lookup result, etc.
    uint64_t sequence_number;            ///< Global ordering
    
    // === Constructor ===
    PipelineEvent()
        : event_id(generateEventId())
        , packet_id()
        , type(PipelineEventType::PACKET_GENERATED)
        , timestamp(std::chrono::steady_clock::now())
        , ingress_port()
        , egress_port()
        , ttl(64)
        , protocol(ProtocolType::UNKNOWN)
        , sequence_number(0) {}
    
    // === Build from packet ===
    static PipelineEvent fromPacket(
        const EnhancedPacket& packet,
        PipelineEventType event_type,
        const std::string& switch_name
    ) {
        PipelineEvent event;
        event.packet_id = packet.packet_id;
        event.type = event_type;
        event.switch_name = switch_name;
        event.ingress_port = packet.ingressPort();
        event.egress_port = packet.egressPort();
        event.vlan_id = packet.vlanId();
        event.ttl = packet.ttl();
        event.protocol = packet.protocol();
        event.sequence_number = packet.sequence_number;
        return event;
    }

private:
    static std::string generateEventId() {
        static std::atomic<uint64_t> s_event_id_counter{0};
        return "ev-" + std::to_string(s_event_id_counter.fetch_add(1, std::memory_order_relaxed));
    }
};

/**
 * @brief Callback type for pipeline event consumers
 */
export using PipelineEventCallback = std::function<void(const PipelineEvent&)>;

/**
 * @brief Pipeline Tracer - generates and routes pipeline events
 * 
 * Uses Boost.Asio for async event delivery.
 * Thread-safe for use from multiple pipeline threads.
 */
export class PipelineTracer {
public:
    static PipelineTracer& instance();
    
    // Event subscription
    void subscribe(PipelineEventCallback callback);
    void unsubscribe(size_t subscriber_id);
    
    // Event generation - call from pipeline stages
    void traceGenerated(const EnhancedPacket& packet, const std::string& host_name);
    void traceEntered(const EnhancedPacket& packet, const std::string& switch_name);
    void traceForwardDecision(const EnhancedPacket& packet, const std::string& switch_name, 
                               const std::string& next_hop);
    void traceDropped(const EnhancedPacket& packet, const std::string& switch_name, 
                      const std::string& reason);
    void traceExited(const EnhancedPacket& packet, const std::string& switch_name);
    void traceL2Lookup(const EnhancedPacket& packet, const std::string& switch_name,
                       bool hit, const std::string& info = "");
    void traceL3Lookup(const EnhancedPacket& packet, const std::string& switch_name,
                       bool hit, const std::string& info = "");
    void traceArpEvent(const EnhancedPacket& packet, const std::string& switch_name,
                       bool is_reply, uint32_t resolved_ip);
    void traceQueueEvent(const EnhancedPacket& packet, const std::string& switch_name,
                         bool is_enqueue, uint32_t queue_depth);
    
    // Batch operations for performance
    void traceBatch(const std::vector<PipelineEvent>& events);
    
private:
    PipelineTracer() = default;
    
    void dispatchEvent(const PipelineEvent& event);
    
    // Thread-safe subscriber management
    std::mutex m_subscribers_mutex;
    std::vector<std::pair<size_t, PipelineEventCallback>> m_subscribers;
    std::atomic<size_t> m_next_subscriber_id{0};
};

/**
 * @brief Scoped packet tracing - RAII wrapper for packet lifecycle
 * 
 * Automatically generates enter/exit events for packet processing scope.
 * Usage:
 *   {
 *       ScopedPacketTracing trace(packet, "TOR1", tracer);
 *       // ... process packet ...
 *   } // trace.exit() called automatically
 */
export class ScopedPacketTracing {
public:
    ScopedPacketTracing(EnhancedPacket& packet, 
                        const std::string& switch_name,
                        PipelineTracer& tracer);
    
    ~ScopedPacketTracing();
    
    void markForwarded(const std::string& next_hop);
    void markDropped(const std::string& reason);
    void markL2Lookup(bool hit);
    void markL3Lookup(bool hit);
    
private:
    EnhancedPacket& m_packet;
    std::string m_switch_name;
    PipelineTracer& m_tracer;
    bool m_exited = false;
};

/**
 * @brief Pipeline Event Serializer for WebSocket/JSON output
 * 
 * Converts pipeline events to JSON format for visualization.
 */
export class PipelineEventSerializer {
public:
    static std::string toJson(const PipelineEvent& event);
    static std::string eventTypeToString(PipelineEventType type);
};

/**
 * @brief Async event dispatcher using Boost.Asio
 * 
 * Buffers events and dispatches them asynchronously to avoid
 * blocking the pipeline thread.
 */
export class AsyncEventDispatcher {
public:
    explicit AsyncEventDispatcher(boost::asio::io_context& io_context);
    
    void start();
    void stop();
    
    void postEvent(PipelineEvent event);
    void setOutputCallback(std::function<void(const std::string&)> callback);
    
private:
    void processEvents();
    
    boost::asio::io_context& m_io_context;
    std::optional<boost::asio::steady_timer> m_timer;
    std::function<void(const std::string&)> m_output_callback;
    
    // Event buffer with mutex protection
    std::mutex m_buffer_mutex;
    std::vector<PipelineEvent> m_event_buffer;
    std::atomic<size_t> m_dropped_count{0};  ///< Events dropped due to buffer full
};

} // namespace MiniSonic::DataPlane
