module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include "cross_platform.h"
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

export module MiniSonic.DataPlane;

// Import networking module
import MiniSonic.Networking;

// Import SAI module for SaiInterface
import MiniSonic.SAI;

// Import Utils module for Types namespace
import MiniSonic.Utils;

export namespace MiniSonic::DataPlane {

// Using declarations for standard library types
using std::vector;
using std::string;
using std::thread;
using std::atomic;
using std::chrono::high_resolution_clock;
using std::move;

// Forward declarations
class Packet;
class Pipeline;
class PipelineThread;

/**
 * @brief Network packet representation
 * 
 * Represents a network packet with L2 and L3 headers.
 */
export class Packet {
public:
    Types::MacAddress m_src_mac;
    Types::MacAddress m_dst_mac;
    Types::IpAddress m_src_ip;
    Types::IpAddress m_dst_ip;
    Types::Port m_ingress_port;
    
    // Timestamp for latency measurement
    std::chrono::high_resolution_clock::time_point timestamp;

    // Default constructor
    Packet() = default;

    // Parameterized constructor
    Packet(
        Types::MacAddress src_mac,
        Types::MacAddress dst_mac,
        Types::IpAddress src_ip,
        Types::IpAddress dst_ip,
        Types::Port ingress_port
    ) : m_src_mac(std::move(src_mac)),
        m_dst_mac(std::move(dst_mac)),
        m_src_ip(std::move(src_ip)),
        m_dst_ip(std::move(dst_ip)),
        m_ingress_port(ingress_port),
        timestamp(std::chrono::high_resolution_clock::now()) {}

    // Move constructor
    Packet(Packet&& other) noexcept = default;

    // Move assignment operator
    Packet& operator=(Packet&& other) noexcept = default;

    // Copy constructor
    Packet(const Packet& other) = default;

    // Copy assignment operator
    Packet& operator=(const Packet& other) = default;

    // Destructor
    ~Packet() = default;

    // Getters with const correctness
    [[nodiscard]] const Types::MacAddress& srcMac() const noexcept { return m_src_mac; }
    [[nodiscard]] const Types::MacAddress& dstMac() const noexcept { return m_dst_mac; }
    [[nodiscard]] const Types::IpAddress& srcIp() const noexcept { return m_src_ip; }
    [[nodiscard]] const Types::IpAddress& dstIp() const noexcept { return m_dst_ip; }
    [[nodiscard]] Types::Port ingressPort() const noexcept { return m_ingress_port; }

    // Setters
    void setSrcMac(Types::MacAddress mac) { m_src_mac = std::move(mac); }
    void setDstMac(Types::MacAddress mac) { m_dst_mac = std::move(mac); }
    void setSrcIp(Types::IpAddress ip) { m_src_ip = std::move(ip); }
    void setDstIp(Types::IpAddress ip) { m_dst_ip = std::move(ip); }
    void setIngressPort(Types::Port port) { m_ingress_port = port; }
    
    // Update timestamp
    void updateTimestamp() noexcept {
        timestamp = std::chrono::high_resolution_clock::now();
    }
};

/**
 * @brief Packet processing pipeline
 * 
 * Orchestrates the processing of packets through various stages.
 */
export class Pipeline {
public:
    explicit Pipeline(SAI::SaiInterface& sai);
    ~Pipeline() = default;
    
    /**
     * @brief Process a single packet
     * @param pkt Packet to process
     */
    void process(Packet& pkt);
    
    /**
     * @brief Get pipeline statistics
     */
    [[nodiscard]] std::string getStats() const;

private:
    SAI::SaiInterface& m_sai;
};

/**
 * @brief Lock-free Single Producer Single Consumer queue
 * 
 * High-performance queue for inter-thread packet communication.
 */
export template<typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity)
        : m_capacity(capacity), m_buffer(capacity), m_head(0), m_tail(0) {}
    
    ~SPSCQueue() = default;
    
    // Disable copy/move
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;
    
    // Push operation (producer)
    bool push(T&& item) {
        const size_t current_head = m_head.load(std::memory_order_relaxed);
        const size_t next_head = (current_head + 1) % m_capacity;
        
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        m_buffer[current_head] = std::move(item);
        m_head.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Pop operation (consumer)
    bool pop(T& item) {
        const size_t current_tail = m_tail.load(std::memory_order_relaxed);
        
        if (current_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        item = std::move(m_buffer[current_tail]);
        m_tail.store((current_tail + 1) % m_capacity, std::memory_order_release);
        return true;
    }
    
    // Utility methods
    [[nodiscard]] size_t size() const {
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        
        if (head >= tail) {
            return head - tail;
        } else {
            return m_capacity - (tail - head);
        }
    }
    
    [[nodiscard]] size_t capacity() const noexcept {
        return m_capacity;
    }
    
    [[nodiscard]] bool empty() const {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
    [[nodiscard]] bool full() const {
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        return ((head + 1) % m_capacity) == tail;
    }

private:
    alignas(64) const size_t m_capacity;
    alignas(64) std::vector<T> m_buffer;
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
};

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
        SPSCQueue<Packet>& queue,
        Types::Count batch_size = 32
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
    void processBatch(std::vector<Packet>& batch);

    /**
     * @brief Add timestamp to packet for latency measurement
     */
    void markPacketTimestamp(Packet& pkt);

    // Core components
    Pipeline& m_pipeline;
    SPSCQueue<Packet>& m_queue;

    // Configuration
    const Types::Count m_batch_size;

    // Thread management
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    // Statistics
    std::atomic<Types::Count> m_packets_processed{0};
    std::atomic<Types::Count> m_batches_processed{0};
    std::atomic<Types::Count> m_empty_cycles{0};
    std::atomic<Types::Count> m_total_processing_time_us{0};

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
};

} // export namespace MiniSonic::DataPlane
