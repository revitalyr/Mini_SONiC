module;

// Import standard library modules
import <memory>;
import <string>;
import <vector>;
import <atomic>;
import <mutex>;
import <iostream>;
import <sstream>;
import <chrono>;
import <algorithm>;
import <iomanip>;

export module MiniSonic.Utils;

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
 * @brief String utilities
 */
export namespace StringUtils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
    std::string toLower(const std::string& str);
    std::string toUpper(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);
}

/**
 * @brief Time utilities
 */
export namespace TimeUtils {
    std::string formatDuration(std::chrono::nanoseconds duration);
    std::string formatTimestamp(std::chrono::steady_clock::time_point timestamp);
    std::chrono::steady_clock::time_point now();
    Types::Count timestampToMicroseconds(std::chrono::steady_clock::time_point timestamp);
}

/**
 * @brief Network utilities
 */
export namespace NetworkUtils {
    bool isValidMacAddress(const std::string& mac);
    bool isValidIpAddress(const std::string& ip);
    std::string formatMacAddress(const std::string& mac);
    std::string formatIpAddress(const std::string& ip);
    Types::Count calculateChecksum(const std::vector<uint8_t>& data);
}

} // export namespace MiniSonic::Utils
