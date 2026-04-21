module;

#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

export module MiniSonic.Events;

export namespace MiniSonic::Events {

// =============================================================================
// EVENT BASE CLASS
// =============================================================================

/**
 * @brief Base class for all events
 */
struct Event {
    std::string type;
    uint64_t timestamp_ns;
    std::string switch_id;

    Event() = default;
    Event(const std::string& t, uint64_t ts, const std::string& sid)
        : type(t), timestamp_ns(ts), switch_id(sid) {}

    virtual ~Event() = default;
    virtual nlohmann::json toJson() const = 0;
};

// =============================================================================
// PACKET EVENTS
// =============================================================================

/**
 * @brief Packet information for visualization
 */
struct PacketInfo {
    uint64_t id;
    std::string src_mac;
    std::string dst_mac;
    std::string src_ip;
    std::string dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    std::string protocol;
    uint8_t dscp;
    uint8_t ttl;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"src_mac", src_mac},
            {"dst_mac", dst_mac},
            {"src_ip", src_ip},
            {"dst_ip", dst_ip},
            {"src_port", src_port},
            {"dst_port", dst_port},
            {"protocol", protocol},
            {"dscp", dscp},
            {"ttl", ttl}
        };
    }
};

/**
 * @brief Packet generated and sent to first switch
 */
struct PacketGenerated : public Event {
    PacketInfo packet;
    std::string ingress_switch;

    PacketGenerated(uint64_t ts, const PacketInfo& pkt, const std::string& is)
        : Event("PacketGenerated", ts, is), packet(pkt), ingress_switch(is) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"packet", packet.toJson()},
            {"ingress_switch", ingress_switch}
        };
    }
};

/**
 * @brief Packet entered a switch
 */
struct PacketEnteredSwitch : public Event {
    uint64_t packet_id;
    std::string ingress_port;

    PacketEnteredSwitch(uint64_t ts, const std::string& sid, uint64_t pid, const std::string& ip)
        : Event("PacketEnteredSwitch", ts, sid), packet_id(pid), ingress_port(ip) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"ingress_port", ingress_port}
        };
    }
};

/**
 * @brief Switch made forwarding decision
 */
struct PacketForwardDecision : public Event {
    uint64_t packet_id;
    std::string egress_port;
    std::string route;
    std::string ecmp_hash;
    std::string next_hop;

    PacketForwardDecision(uint64_t ts, const std::string& sid, uint64_t pid,
                         const std::string& ep, const std::string& r,
                         const std::string& eh, const std::string& nh)
        : Event("PacketForwardDecision", ts, sid), packet_id(pid),
          egress_port(ep), route(r), ecmp_hash(eh), next_hop(nh) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"egress_port", egress_port},
            {"route", route},
            {"ecmp_hash", ecmp_hash},
            {"next_hop", next_hop}
        };
    }
};

/**
 * @brief Packet exited a switch
 */
struct PacketExitedSwitch : public Event {
    uint64_t packet_id;
    std::string egress_port;

    PacketExitedSwitch(uint64_t ts, const std::string& sid, uint64_t pid, const std::string& ep)
        : Event("PacketExitedSwitch", ts, sid), packet_id(pid), egress_port(ep) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"egress_port", egress_port}
        };
    }
};

// =============================================================================
// PIPELINE EVENTS
// =============================================================================

/**
 * @brief Packet entered pipeline stage
 */
struct PipelineStageEntered : public Event {
    uint64_t packet_id;
    std::string stage;

    PipelineStageEntered(uint64_t ts, const std::string& sid, uint64_t pid, const std::string& s)
        : Event("PipelineStageEntered", ts, sid), packet_id(pid), stage(s) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"stage", stage}
        };
    }
};

/**
 * @brief Packet exited pipeline stage
 */
struct PipelineStageExited : public Event {
    uint64_t packet_id;
    std::string stage;
    uint64_t latency_ns;

    PipelineStageExited(uint64_t ts, const std::string& sid, uint64_t pid,
                        const std::string& s, uint64_t lat)
        : Event("PipelineStageExited", ts, sid), packet_id(pid), stage(s), latency_ns(lat) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"stage", stage},
            {"latency_ns", latency_ns}
        };
    }
};

/**
 * @brief ACL matched
 */
struct ACLMatch : public Event {
    uint64_t packet_id;
    uint32_t rule_id;
    std::string action;
    std::string details;

    ACLMatch(uint64_t ts, const std::string& sid, uint64_t pid,
            uint32_t rid, const std::string& a, const std::string& d)
        : Event("ACLMatch", ts, sid), packet_id(pid), rule_id(rid),
          action(a), details(d) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"rule_id", rule_id},
            {"action", action},
            {"details", details}
        };
    }
};

/**
 * @brief QoS classification
 */
struct QoSClassification : public Event {
    uint64_t packet_id;
    uint32_t queue;
    uint8_t dscp;

    QoSClassification(uint64_t ts, const std::string& sid, uint64_t pid,
                     uint32_t q, uint8_t d)
        : Event("QoSClassification", ts, sid), packet_id(pid), queue(q), dscp(d) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"queue", queue},
            {"dscp", dscp}
        };
    }
};

/**
 * @brief Packet queued
 */
struct PacketQueued : public Event {
    uint64_t packet_id;
    uint32_t queue;
    uint32_t queue_depth;

    PacketQueued(uint64_t ts, const std::string& sid, uint64_t pid,
                uint32_t q, uint32_t qd)
        : Event("PacketQueued", ts, sid), packet_id(pid), queue(q), queue_depth(qd) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"queue", queue},
            {"queue_depth", queue_depth}
        };
    }
};

/**
 * @brief Packet dropped
 */
struct PacketDropped : public Event {
    uint64_t packet_id;
    std::string reason;

    PacketDropped(uint64_t ts, const std::string& sid, uint64_t pid, const std::string& r)
        : Event("PacketDropped", ts, sid), packet_id(pid), reason(r) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packet_id", packet_id},
            {"reason", reason}
        };
    }
};

// =============================================================================
// SWITCH STATE EVENTS
// =============================================================================

/**
 * @brief Port state changed
 */
struct PortStateChanged : public Event {
    std::string port;
    std::string state;

    PortStateChanged(uint64_t ts, const std::string& sid, const std::string& p, const std::string& s)
        : Event("PortStateChanged", ts, sid), port(p), state(s) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"port", port},
            {"state", state}
        };
    }
};

/**
 * @brief Queue depth update
 */
struct QueueDepthUpdate : public Event {
    std::string port;
    std::map<uint32_t, uint32_t> queues;

    QueueDepthUpdate(uint64_t ts, const std::string& sid, const std::string& p,
                    const std::map<uint32_t, uint32_t>& q)
        : Event("QueueDepthUpdate", ts, sid), port(p), queues(q) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"port", port},
            {"queues", queues}
        };
    }
};

/**
 * @brief Switch statistics
 */
struct SwitchStats : public Event {
    uint64_t packets_per_sec;
    uint64_t drops_per_sec;
    double cpu_usage;

    SwitchStats(uint64_t ts, const std::string& sid, uint64_t pps,
               uint64_t dps, double cpu)
        : Event("SwitchStats", ts, sid), packets_per_sec(pps),
          drops_per_sec(dps), cpu_usage(cpu) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"switch_id", switch_id},
            {"packets_per_sec", packets_per_sec},
            {"drops_per_sec", drops_per_sec},
            {"cpu_usage", cpu_usage}
        };
    }
};

// =============================================================================
// TOPOLOGY EVENTS
// =============================================================================

/**
 * @brief Link state changed
 */
struct LinkStateChanged : public Event {
    std::string from;
    std::string to;
    std::string state;

    LinkStateChanged(uint64_t ts, const std::string& from_s, const std::string& to_s,
                   const std::string& s)
        : Event("LinkStateChanged", ts, from_s), from(from_s), to(to_s), state(s) {}

    nlohmann::json toJson() const override {
        return {
            {"type", type},
            {"timestamp_ns", timestamp_ns},
            {"from", from},
            {"to", to},
            {"state", state}
        };
    }
};

// =============================================================================
// EVENT BUS (PUBLISHER-SUBSCRIBER)
// =============================================================================

using EventCallback = std::function<void(const nlohmann::json&)>;

/**
 * @brief Event bus for publishing and subscribing to events
 */
class EventBus {
private:
    std::map<std::string, std::vector<EventCallback>> subscribers_;
    std::mutex mutex_;

public:
    void subscribe(const std::string& event_type, EventCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_[event_type].push_back(callback);
    }

    void publish(std::shared_ptr<Event> event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto json = event->toJson();
        auto it = subscribers_.find(event->type);
        if (it != subscribers_.end()) {
            for (auto& callback : it->second) {
                callback(json);
            }
        }
    }

    void publishJson(const nlohmann::json& json_event) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string type = json_event["type"];
        auto it = subscribers_.find(type);
        if (it != subscribers_.end()) {
            for (auto& callback : it->second) {
                callback(json_event);
            }
        }
    }
};

// Global event bus instance
export inline EventBus& getGlobalEventBus() {
    static EventBus instance;
    return instance;
}

} // export namespace MiniSonic::Events
