/**
 * @file arp_pending_queue.ixx
 * @brief ARP Pending Queue - Fixes packet drops on missing ARP
 * 
 * When a packet arrives for an unknown destination MAC (ARP miss),
 * instead of dropping it, we:
 * 1. Queue the packet in a pending queue
 * 2. Send ARP request
 * 3. When ARP reply arrives, dequeue and forward packets
 * 
 * Uses MiniSonic.Boost.Wrappers for lock-free queue implementation.
 */

module;

#include <chrono>
#include <list>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/uuid/uuid.hpp>

export module MiniSonic.L3.ArpPendingQueue;

import MiniSonic.DataPlane.PacketEnhanced;

export namespace MiniSonic::L3 {
/**
 * @brief Pending packet entry with timeout
 */
export struct PendingPacket {
    std::string packet_id;                   ///< Original packet ID for tracing
    std::shared_ptr<DataPlane::EnhancedPacket> packet;
    std::chrono::steady_clock::time_point queued_at;
    uint32_t pending_ip;                     ///< IP we're waiting ARP for
    int retry_count;                         ///< ARP retry attempts
    
    PendingPacket()
        : packet_id()
        , packet(nullptr)
        , pending_ip(0)
        , retry_count(0) {
        queued_at = std::chrono::steady_clock::now();
    }
    
    PendingPacket(std::shared_ptr<DataPlane::EnhancedPacket> pkt, uint32_t wait_ip)
        : packet_id(pkt->packet_id)
        , packet(std::move(pkt))
        , pending_ip(wait_ip)
        , retry_count(0) {
        queued_at = std::chrono::steady_clock::now();
    }
};

/**
 * @brief ARP Pending Queue Manager
 * 
 * Manages packets waiting for ARP resolution.
 * Uses MiniSonic.Boost.Wrappers for lock-free queues and synchronization.
 */
export class ArpPendingQueue {
public:
    static constexpr size_t MAX_PENDING_PACKETS = 1024;
    static constexpr auto ARP_TIMEOUT = std::chrono::seconds(2);
    static constexpr int MAX_RETRIES = 3;
    
    ArpPendingQueue()
        : m_pending_packets(MAX_PENDING_PACKETS) {}
    
    /**
     * @brief Queue a packet pending ARP resolution
     * @param packet The packet to queue
     * @param pending_ip The IP address we're waiting for ARP
     * @return true if packet was queued, false if queue is full
     */
    bool enqueue(std::shared_ptr<DataPlane::EnhancedPacket> packet, uint32_t pending_ip) {
        // Create pending entry
        PendingPacket entry(std::move(packet), pending_ip);
        
        // Try to push to lockfree queue first
        auto* ptr = new PendingPacket(entry);
        if (!m_pending_packets.push(ptr)) {
            delete ptr;
            return false;
        }
        
        // Track which IP this packet is waiting for (thread-safe)
        {
            LockGuard lock(m_ip_map_mutex);
            m_ip_to_packets[pending_ip].push_back(ptr);
        }
        
        return true;
    }
    
    /**
     * @brief Process ARP reply - forward all pending packets for this IP
     * @param resolved_ip The IP that now has ARP entry
     * @param callback Function to call for each packet to forward
     * @return Number of packets forwarded
     */
    template<typename ForwardCallback>
    size_t onArpResolved(uint32_t resolved_ip, ForwardCallback&& callback) {
        std::lock_guard<std::mutex> lock(m_ip_map_mutex);
        
        auto it = m_ip_to_packets.find(resolved_ip);
        if (it == m_ip_to_packets.end()) {
            return 0;
        }
        
        size_t forwarded = 0;
        for (auto* ptr : it->second) {
            if (ptr && ptr->packet) {
                // Mark packet as ARP resolved and forward
                ptr->packet->recordStage(DataPlane::PipelineStage::L3_LOOKUP);
                callback(*ptr->packet);
                forwarded++;
                
                // Mark as processed (but don't delete yet - will be cleaned up by cleanup())
                ptr->pending_ip = 0;
            }
        }
        
        m_ip_to_packets.erase(it);
        return forwarded;
    }
    
    /**
     * @brief Get all pending IPs that need ARP requests sent
     * @return List of IPs that need ARP requests
     */
    std::vector<uint32_t> getPendingIps() {
        std::lock_guard<std::mutex> lock(m_ip_map_mutex);
        
        std::vector<uint32_t> pending;
        for (const auto& [ip, packets] : m_ip_to_packets) {
            if (!packets.empty()) {
                pending.push_back(ip);
            }
        }
        return pending;
    }
    
    /**
     * @brief Clean up timed out entries
     * @param drop_callback Called for each dropped packet
     * @return Number of dropped packets
     */
    template<typename DropCallback>
    size_t cleanup(DropCallback&& drop_callback) {
        auto now = std::chrono::steady_clock::now();
        size_t dropped = 0;
        
        std::lock_guard<std::mutex> lock(m_ip_map_mutex);
        
        auto ip_it = m_ip_to_packets.begin();
        while (ip_it != m_ip_to_packets.end()) {
            auto& packets = ip_it->second;
            
            auto pkt_it = packets.begin();
            while (pkt_it != packets.end()) {
                auto* ptr = *pkt_it;
                if (!ptr || !ptr->packet) {
                    pkt_it = packets.erase(pkt_it);
                    continue;
                }
                
                auto age = now - ptr->queued_at;
                
                if (age > ARP_TIMEOUT) {
                    if (ptr->retry_count >= MAX_RETRIES) {
                        // Drop packet - ARP resolution failed
                        ptr->packet->markDropped("ARP resolution timeout");
                        drop_callback(*ptr->packet);
                        dropped++;
                        
                        // Remove from IP map
                        pkt_it = packets.erase(pkt_it);
                    } else {
                        // Retry ARP
                        ptr->retry_count++;
                        ptr->queued_at = now;
                        ++pkt_it;
                    }
                } else {
                    ++pkt_it;
                }
            }
            
            if (packets.empty()) {
                ip_it = m_ip_to_packets.erase(ip_it);
            } else {
                ++ip_it;
            }
        }
        
        return dropped;
    }
    
    /**
     * @brief Get statistics about pending queue
     */
    struct Stats {
        size_t pending_count;
        size_t unique_ips;
        size_t oldest_packet_age_ms;
    };
    
    Stats getStats() const {
        std::lock_guard<std::mutex> lock(m_ip_map_mutex);
        
        Stats stats{};
        stats.unique_ips = m_ip_to_packets.size();
        
        auto now = std::chrono::steady_clock::now();
        auto oldest = now;
        
        for (const auto& [ip, packets] : m_ip_to_packets) {
            stats.pending_count += packets.size();
            for (const auto* ptr : packets) {
                if (ptr && ptr->queued_at < oldest) {
                    oldest = ptr->queued_at;
                }
            }
        }
        
        stats.oldest_packet_age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - oldest).count();
        
        return stats;
    }
    
private:
    // Lockfree queue for high-performance enqueue
    boost::lockfree::queue<PendingPacket*> m_pending_packets;
    
    // Map from IP to list of pending packets (needs mutex protection)
    mutable std::mutex m_ip_map_mutex;
    std::unordered_map<uint32_t, std::list<PendingPacket*>> m_ip_to_packets;
};

/**
 * @brief Simplified ARP Pending Queue using Boost.SPSC for single-producer/single-consumer
 * 
 * Use this when there's a single ARP resolver thread.
 */
export template<size_t Capacity = 1024>
class ArpPendingSPSCQueue {
public:
    /**
     * @brief Try to enqueue a packet
     * @return true if successful
     */
    bool tryEnqueue(std::shared_ptr<DataPlane::EnhancedPacket> packet, uint32_t pending_ip) {
        PendingEntry entry;
        entry.packet_id = packet->packet_id;
        entry.packet = std::move(packet);
        entry.pending_ip = pending_ip;
        entry.queued_at = std::chrono::steady_clock::now();
        
        return m_queue.push(entry);
    }
    
    /**
     * @brief Try to dequeue a packet
     * @return true if packet was dequeued
     */
    bool tryDequeue(std::shared_ptr<DataPlane::EnhancedPacket>& packet, uint32_t& pending_ip) {
        PendingEntry entry;
        if (!m_queue.pop(entry)) {
            return false;
        }
        
        packet = std::move(entry.packet);
        pending_ip = entry.pending_ip;
        return true;
    }
    
    /**
     * @brief Get number of pending packets
     */
    size_t size() const {
        return m_queue.read_available();
    }
    
private:
    struct PendingEntry {
        std::string packet_id;
        std::shared_ptr<DataPlane::EnhancedPacket> packet;
        uint32_t pending_ip;
        std::chrono::steady_clock::time_point queued_at;
    };
    
    boost::lockfree::spsc_queue<PendingEntry> m_queue{Capacity};
};

} // namespace MiniSonic::L3
