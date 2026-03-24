#pragma once

#include "common/types.hpp"
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace MiniSonic::Utils {

/**
 * @brief High-performance metrics collection system
 * 
 * Thread-safe metrics collection for packet processing performance.
 * Uses atomic operations for counters and lock-free data structures.
 */
class Metrics {
public:
    /**
     * @brief Get singleton instance
     */
    [[nodiscard]] static Metrics& instance() noexcept {
        static Metrics metrics;
        return metrics;
    }

    /**
     * @brief Increment a counter metric
     * @param name Name of the counter
     * @param value Value to add (default: 1)
     */
    void inc(const Types::String& name, Types::Count value = 1) noexcept {
        auto& counter = m_counters[name];
        counter.fetch_add(value, std::memory_order_relaxed);
    }

    /**
     * @brief Set a gauge metric
     * @param name Name of the gauge
     * @param value Value to set
     */
    void set(const Types::String& name, Types::Count value) noexcept {
        auto& gauge = m_gauges[name];
        gauge.store(value, std::memory_order_relaxed);
    }

    /**
     * @brief Add latency measurement
     * @param latency_us Latency in microseconds
     */
    void addLatency(Types::Count latency_us) noexcept {
        // Simple moving average for latency
        auto current = m_avg_latency.load(std::memory_order_relaxed);
        const auto alpha = 0.1; // Smoothing factor
        const auto new_avg = current * (1.0 - alpha) + latency_us * alpha;
        m_avg_latency.store(static_cast<Types::Count>(new_avg), std::memory_order_relaxed);
        
        // Track min/max
        auto current_min = m_min_latency.load(std::memory_order_relaxed);
        auto current_max = m_max_latency.load(std::memory_order_relaxed);
        
        if (latency_us < current_min || current_min == 0) {
            m_min_latency.store(latency_us, std::memory_order_relaxed);
        }
        if (latency_us > current_max) {
            m_max_latency.store(latency_us, std::memory_order_relaxed);
        }
        
        m_latency_count.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Get counter value
     * @param name Name of the counter
     * @return Current counter value
     */
    [[nodiscard]] Types::Count getCounter(const Types::String& name) const {
        const auto it = m_counters.find(name);
        return (it != m_counters.end()) ? it->second.load() : 0;
    }

    /**
     * @brief Get gauge value
     * @param name Name of the gauge
     * @return Current gauge value
     */
    [[nodiscard]] Types::Count getGauge(const Types::String& name) const {
        const auto it = m_gauges.find(name);
        return (it != m_gauges.end()) ? it->second.load() : 0;
    }

    /**
     * @brief Get average latency
     * @return Average latency in microseconds
     */
    [[nodiscard]] Types::Count getAvgLatency() const noexcept {
        return m_avg_latency.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get minimum latency
     * @return Minimum latency in microseconds
     */
    [[nodiscard]] Types::Count getMinLatency() const noexcept {
        return m_min_latency.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get maximum latency
     * @return Maximum latency in microseconds
     */
    [[nodiscard]] Types::Count getMaxLatency() const noexcept {
        return m_max_latency.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get total latency measurements count
     * @return Number of latency measurements
     */
    [[nodiscard]] Types::Count getLatencyCount() const noexcept {
        return m_latency_count.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset all metrics
     */
    void reset() noexcept {
        for (auto& [name, counter] : m_counters) {
            counter.store(0, std::memory_order_relaxed);
        }
        for (auto& [name, gauge] : m_gauges) {
            gauge.store(0, std::memory_order_relaxed);
        }
        m_avg_latency.store(0, std::memory_order_relaxed);
        m_min_latency.store(0, std::memory_order_relaxed);
        m_max_latency.store(0, std::memory_order_relaxed);
        m_latency_count.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Get all metrics as formatted string
     * @return Formatted metrics string
     */
    [[nodiscard]] Types::String getSummary() const {
        Types::String result;
        result += "=== Mini SONiC Metrics ===\n";
        
        // Counters
        result += "\nCounters:\n";
        for (const auto& [name, counter] : m_counters) {
            result += "  " + name + ": " + std::to_string(counter.load()) + "\n";
        }
        
        // Gauges
        result += "\nGauges:\n";
        for (const auto& [name, gauge] : m_gauges) {
            result += "  " + name + ": " + std::to_string(gauge.load()) + "\n";
        }
        
        // Latency stats
        result += "\nLatency (μs):\n";
        result += "  avg: " + std::to_string(getAvgLatency()) + "\n";
        result += "  min: " + std::to_string(getMinLatency()) + "\n";
        result += "  max: " + std::to_string(getMaxLatency()) + "\n";
        result += "  count: " + std::to_string(getLatencyCount()) + "\n";
        
        return result;
    }

private:
    Metrics() = default;
    ~Metrics() = default;

    // Rule of five
    Metrics(const Metrics& other) = delete;
    Metrics& operator=(const Metrics& other) = delete;
    Metrics(Metrics&& other) noexcept = delete;
    Metrics& operator=(Metrics&& other) noexcept = delete;

    // Counters (packet counts, error counts, etc.)
    Types::Map<Types::String, Types::Atomic<Types::Count>> m_counters;
    
    // Gauges (current values like queue sizes, connection counts)
    Types::Map<Types::String, Types::Atomic<Types::Count>> m_gauges;
    
    // Latency measurements
    Types::Atomic<Types::Count> m_avg_latency{0};
    Types::Atomic<Types::Count> m_min_latency{0};
    Types::Atomic<Types::Count> m_max_latency{0};
    Types::Atomic<Types::Count> m_latency_count{0};
};

} // namespace MiniSonic::Utils
