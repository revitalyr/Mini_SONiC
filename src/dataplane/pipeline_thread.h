#pragma once

#include "dataplane/pipeline.h"
#include "utils/spsc_queue.hpp"
#include "utils/metrics.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

namespace MiniSonic::DataPlane {

/**
 * @brief Dedicated thread for batch packet processing
 * 
 * High-performance pipeline thread that processes packets in batches
 * to improve cache efficiency and reduce per-packet overhead.
 * Uses lock-free SPSC queue for thread-safe communication.
 */
class PipelineThread {
public:
    /**
     * @brief Construct pipeline thread
     * @param pipeline Reference to packet processing pipeline
     * @param queue Reference to SPSC queue for packet input
     * @param batch_size Maximum number of packets to process in one batch
     */
    PipelineThread(
        Pipeline& pipeline, 
        Utils::SPSCQueue<Packet>& queue,
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
    [[nodiscard]] Types::String getStats() const;

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
    Utils::SPSCQueue<Packet>& m_queue;

    // Configuration
    const Types::Count m_batch_size;

    // Thread management
    std::thread m_thread;
    Types::AtomicBool m_running{false};

    // Statistics
    Types::Atomic<Types::Count> m_packets_processed{0};
    Types::Atomic<Types::Count> m_batches_processed{0};
    Types::Atomic<Types::Count> m_empty_cycles{0};
    Types::Atomic<Types::Count> m_total_processing_time_us{0};

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
};

} // namespace MiniSonic::DataPlane
