module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cstdint>
#include <ranges>
#include <expected>
#include <span>
#include <print>

export module MiniSonic.Utils;

/**
 * @brief Internal types for the modular system
 */
namespace Types {
    // Semantic type aliases for domain clarity (C++23)
    export using PortId = uint16_t;           // Port identifier (semantic alias)
    export using MacAddress = uint64_t;       // MAC address (semantic alias)
    export using IpAddress = uint32_t;        // IP address (semantic alias)
    export using VlanId = uint16_t;           // VLAN identifier (semantic alias)
    export using PacketCount = uint64_t;      // Number of packets (semantic alias)
    export using ByteCount = uint64_t;        // Number of bytes (semantic alias)
    export using Timestamp = uint64_t;        // Unix timestamp (semantic alias)
    export using SequenceNumber = uint32_t;    // Sequence number (semantic alias)
    export using BufferLength = size_t;        // Buffer length in bytes (semantic alias)
    export using EntryCount = size_t;         // Number of entries (semantic alias)
    export using SocketFd = int;              // Socket file descriptor (semantic alias)
    export {
        using Count = uint64_t;
        template<typename T>
        using Atomic = std::atomic<T>;
    }
}

export namespace MiniSonic::Utils {

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
    Types::Count getCounter(const std::string& name) const;

    // Gauge operations
    void setGauge(const std::string& name, double value);
    double getGauge(const std::string& name) const;

    // Latency tracking
    void addLatency(Types::Count latency_us);
    std::string getLatencyStats() const;

    // General operations
    void reset();
    std::string getSummary() const;
    std::string getJson() const;

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
