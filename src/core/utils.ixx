module;

// Global module fragment for standard library includes
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cstdint>
#include <span>
#include <cstddef>

export module MiniSonic.Core.Utils;

import MiniSonic.Core.Types;

export namespace MiniSonic::Utils {

/**
 * @brief Lock-free Single Producer Single Consumer (SPSC) Queue
 *
 * High-performance queue for packet buffering between threads.
 * Only one thread can push (producer) and one thread can pop (consumer).
 * Uses atomic operations for thread safety without locks.
 *
 * @tparam T Type of elements to store in the queue
 */
template<typename T>
class SPSCQueue {
public:
    /**
     * @brief Construct SPSC queue with specified capacity
     * @param size Maximum number of elements the queue can hold
     */
    explicit SPSCQueue(Types::Count size)
        : m_buffer(size), m_capacity(size) {}

    /**
     * @brief Push element to queue (producer only)
     * @param item Element to push
     * @return true if element was pushed, false if queue is full
     */
    [[nodiscard]] bool push(const T& item) noexcept {
        const auto current_head = m_head.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) % m_capacity;

        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }

        m_buffer[current_head] = item;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Push element to queue using move semantics (producer only)
     * @param item Element to push (will be moved)
     * @return true if element was pushed, false if queue is full
     */
    [[nodiscard]] bool push(T&& item) noexcept {
        const auto current_head = m_head.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) % m_capacity;

        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }

        m_buffer[current_head] = std::move(item);
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop element from queue (consumer only)
     * @param item Reference to store popped element
     * @return true if element was popped, false if queue is empty
     */
    [[nodiscard]] bool pop(T& item) noexcept {
        const auto current_tail = m_tail.load(std::memory_order_relaxed);

        if (current_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }

        item = std::move(m_buffer[current_tail]);
        m_tail.store((current_tail + 1) % m_capacity, std::memory_order_release);
        return true;
    }

    /**
     * @brief Check if queue is empty
     * @return true if queue is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return m_head.load(std::memory_order_acquire) ==
               m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if queue is full
     * @return true if queue is full
     */
    [[nodiscard]] bool full() const noexcept {
        const auto next_head = (m_head.load(std::memory_order_acquire) + 1) % m_capacity;
        return next_head == m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Get current size of queue
     * @return Approximate number of elements in queue
     */
    [[nodiscard]] Types::Count size() const noexcept {
        const auto head = m_head.load(std::memory_order_acquire);
        const auto tail = m_tail.load(std::memory_order_acquire);

        if (head >= tail) {
            return head - tail;
        } else {
            return m_capacity - tail + head;
        }
    }

    /**
     * @brief Get queue capacity
     * @return Maximum number of elements queue can hold
     */
    [[nodiscard]] Types::Count capacity() const noexcept {
        return m_capacity;
    }

private:
    std::vector<T> m_buffer;
    const Types::Count m_capacity;
    alignas(64) std::atomic<Types::Count> m_head{0};  // Producer index
    alignas(64) std::atomic<Types::Count> m_tail{0};  // Consumer index
};

/**
 * @brief Thread-safe metrics collection system
 *
 * Provides performance monitoring and statistics collection.
 */
export class Metrics {
public:
    static Metrics& instance();

    // Counter operations
    void inc(const std::string& name, Types::Count value = 1);
    void dec(const std::string& name, Types::Count value = 1);
    void set(const std::string& name, Types::Count value);
    [[nodiscard]] Types::Count getCounter(const std::string& name) const;

    // Gauge operations
    void setGauge(const std::string& name, double value);
    [[nodiscard]] double getGauge(const std::string& name) const;

    // Latency tracking
    void addLatency(Types::Count latency_us);
    [[nodiscard]] std::string getLatencyStats() const;

    // General operations
    void reset();
    [[nodiscard]] std::string getSummary() const;
    [[nodiscard]] std::string getJson() const;

private:
    Metrics() = default;
    ~Metrics() = default;

    // Prevent copying
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Types::Atomic<Types::Count>> m_counters;
    std::unordered_map<std::string, std::atomic<double>> m_gauges;

    // Latency tracking
    struct LatencyStats {
        std::atomic<Types::Count> count{0};
        std::atomic<Types::Count> sum{0};
        std::atomic<Types::Count> min{std::numeric_limits<Types::Count>::max()};
        std::atomic<Types::Count> max{0};
        std::vector<Types::Count> samples; // For percentile calculation
        mutable std::mutex samples_mutex;
        static constexpr size_t MAX_SAMPLES = 10000;
    };

    LatencyStats m_latency;

    void updateLatencySamples(Types::Count latency);
};

/**
 * @brief String utilities using C++23 std::ranges
 */
export namespace StringUtils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    std::string toLower(const std::string& str);
    std::string toUpper(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);

    // C++23 ranges-based utilities
    std::string trimView(std::string_view str);
    std::vector<std::string> splitView(std::string_view str, char delimiter);
}

/**
 * @brief Time utilities using C++23 chrono literals
 */
export namespace TimeUtils {
    using namespace std::chrono_literals;
    
    std::string formatDuration(std::chrono::nanoseconds duration);
    std::string formatTimestamp(std::chrono::steady_clock::time_point timestamp);
    std::chrono::steady_clock::time_point now();
    Types::Count timestampToMicroseconds(std::chrono::steady_clock::time_point timestamp);
}

/**
 * @brief Network utilities using C++23 std::span and std::byteswap
 */
export namespace NetworkUtils {
    bool isValidMacAddress(const std::string& mac);
    bool isValidIpAddress(const std::string& ip);
    std::string formatMacAddress(const std::string& mac);
    std::string formatIpAddress(const std::string& ip);
    
    // C++23: Use std::span for efficient data viewing
    Types::Count calculateChecksum(std::span<const uint8_t> data);
    
    // C++23: Use std::byteswap for network byte order conversion
    uint16_t htons(uint16_t hostshort);
    uint32_t htonl(uint32_t hostlong);
    uint16_t ntohs(uint16_t netshort);
    uint32_t ntohl(uint32_t netlong);
}

} // export namespace MiniSonic::Utils
