module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <utility>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <ranges>
#include <span>
#include <optional>
#include "core/common/types.hpp"

export module MiniSonic.DataPlane;

// Import networking module
import MiniSonic.Networking;

// Import SAI module for SaiInterface
import MiniSonic.SAI;

import MiniSonic.L2L3;

// Import Utils module for Types namespace
import MiniSonic.Core.Utils;

// Import Events module for visualization events
import MiniSonic.Core.Events;

export namespace MiniSonic::DataPlane {

// Forward declarations
class Pipeline;
class PipelineThread;

/**

* @brief Packet processing pipeline
 *
 * Orchestrates the processing of packets through various stages using modern C++23.
 */
export class Pipeline {
public:
    explicit Pipeline(SAI::SaiInterface& sai, const std::string& switch_id = "SWITCH0");
    ~Pipeline() = default;

    ::MiniSonic::L3::L3Service& getL3();

    /**
     * @brief Process a single packet
     * @param pkt Packet to process
     */
    void process(::MiniSonic::DataPlane::Packet& pkt);

    /**
     * @brief Process multiple packets using ranges (C++23)
     * @param packets Range of packets to process
     */
    void processBatch(std::span<::MiniSonic::DataPlane::Packet> packets);

    /**
     * @brief Get pipeline statistics
     */
    [[nodiscard]] std::string getStats() const;

    /**
     * @brief Set switch ID for event emission
     */
    void setSwitchId(const std::string& switch_id) { m_switch_id = switch_id; }

    /**
     * @brief Get switch ID
     */
    [[nodiscard]] std::string switchId() const { return m_switch_id; }

private:
    SAI::SaiInterface& m_sai;
    std::string m_switch_id;
    Events::EventBus& m_event_bus;
    std::atomic<uint64_t> m_packet_counter{0};
    ::MiniSonic::L2::L2Service m_l2_service;
    ::MiniSonic::L3::L3Service m_l3_service;
};

/**
 * @brief Lock-free Single Producer Single Consumer queue
 * 
 * High-performance queue for inter-thread packet communication.
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#endif
export template<typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(::MiniSonic::Types::Count capacity)
        : m_capacity(capacity), m_buffer(capacity), m_head(0), m_tail(0) {}
    
    ~SPSCQueue() = default;
    
    // Disable copy/move
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;
    
    // Push operation (producer)
    bool push(T&& item) {
        const ::MiniSonic::Types::Count current_head = m_head.load(std::memory_order_relaxed);
        const ::MiniSonic::Types::Count next_head = (current_head + 1) % m_capacity;
        
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        m_buffer[current_head] = std::move(item);
        m_head.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Pop operation (consumer)
    bool pop(T& item) {
        const ::MiniSonic::Types::Count current_tail = m_tail.load(std::memory_order_relaxed);
        
        if (current_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        item = std::move(m_buffer[current_tail]);
        m_tail.store((current_tail + 1) % m_capacity, std::memory_order_release);
        return true;
    }
    
    // Utility methods
    [[nodiscard]] ::MiniSonic::Types::Count size() const {
        const ::MiniSonic::Types::Count head = m_head.load(std::memory_order_acquire);
        const ::MiniSonic::Types::Count tail = m_tail.load(std::memory_order_acquire);
        
        if (head >= tail) {
            return head - tail;
        } else {
            return m_capacity - (tail - head);
        }
    }
    
    [[nodiscard]] ::MiniSonic::Types::Count capacity() const noexcept {
        return m_capacity;
    }
    
    [[nodiscard]] bool empty() const {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    [[nodiscard]] bool full() const {
        const ::MiniSonic::Types::Count head = m_head.load(std::memory_order_acquire);
        const ::MiniSonic::Types::Count tail = m_tail.load(std::memory_order_acquire);
        return ((head + 1) % m_capacity) == tail;
    }

private:
    alignas(64) const ::MiniSonic::Types::Count m_capacity;
    alignas(64) std::vector<T> m_buffer;
    alignas(64) std::atomic<::MiniSonic::Types::Count> m_head;
    alignas(64) std::atomic<::MiniSonic::Types::Count> m_tail;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/**
 * @brief Dedicated thread for batch packet processing
 * 
 * High-performance pipeline thread that processes packets in batches
 * to improve cache efficiency and reduce per-packet overhead.
 */
export class PipelineThread {
public:
    /**
     * @brief Construct pipeline thread
     * @param pipeline Reference to packet processing pipeline
     * @param queue Reference to SPSC queue for packet input
     * @param batch_size Maximum number of packets to process in one batch
     */
    PipelineThread(
        Pipeline& pipeline, 
        SPSCQueue<::MiniSonic::DataPlane::Packet>& queue,
        ::MiniSonic::Types::Count batch_size = 32
    );

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~PipelineThread();

    /**
     * @brief Start the pipeline thread
     */
    void start();

    /**
     * @brief Stop the pipeline thread gracefully
     */
    void stop();

    /**
     * @brief Check if thread is running
     * @return true if thread is active
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get processing statistics
     */
    [[nodiscard]] std::string getStats() const;

private:
    /**
     * @brief Main processing loop
     */
    void run();

    /**
     * @brief Process a batch of packets
     * @param batch Vector of packets to process
     */
    void processBatch(std::vector<::MiniSonic::DataPlane::Packet>& batch);

    /**
     * @brief Add timestamp to packet for latency measurement
     */
    void markPacketTimestamp(::MiniSonic::DataPlane::Packet& pkt);

    // Core components
    Pipeline& m_pipeline;
    SPSCQueue<::MiniSonic::DataPlane::Packet>& m_queue;

    // Configuration
    const ::MiniSonic::Types::Count m_batch_size;

    // Thread management
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    // Statistics
    std::atomic<::MiniSonic::Types::Count> m_packets_processed{0};
    std::atomic<::MiniSonic::Types::Count> m_batches_processed{0};
    std::atomic<::MiniSonic::Types::Count> m_empty_cycles{0};
    std::atomic<::MiniSonic::Types::Count> m_total_processing_time_us{0};

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
};

} // export namespace MiniSonic::DataPlane
