module;

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <unordered_map>
#include <cctype>

module MiniSonic.Utils;

namespace MiniSonic::Utils {
// Metrics Implementation
Metrics& Metrics::instance() {
    static Metrics instance;
    return instance;
}

void Metrics::inc(const std::string& name, Types::Count value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters[name].fetch_add(value, std::memory_order_relaxed);
}

void Metrics::dec(const std::string& name, Types::Count value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters[name].fetch_sub(value, std::memory_order_relaxed);
}

void Metrics::set(const std::string& name, Types::Count value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters[name].store(value, std::memory_order_relaxed);
}

Types::Count Metrics::getCounter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_counters.find(name);
    return it != m_counters.end() ? it->second.load() : 0;
}

void Metrics::setGauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gauges[name].store(value, std::memory_order_relaxed);
}

double Metrics::getGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_gauges.find(name);
    return it != m_gauges.end() ? it->second.load() : 0.0;
}

void Metrics::addLatency(Types::Count latency_us) {
    m_latency.count.fetch_add(1, std::memory_order_relaxed);
    m_latency.sum.fetch_add(latency_us, std::memory_order_relaxed);
    
    // Update min/max
    Types::Count current_min = m_latency.min.load();
    while (latency_us < current_min && 
           !m_latency.min.compare_exchange_weak(current_min, latency_us)) {
        // Retry if value changed
    }
    
    Types::Count current_max = m_latency.max.load();
    while (latency_us > current_max && 
           !m_latency.max.compare_exchange_weak(current_max, latency_us)) {
        // Retry if value changed
    }
    
    updateLatencySamples(latency_us);
}

std::string Metrics::getLatencyStats() const {
    Types::Count count = m_latency.count.load();
    if (count == 0) {
        return "No latency data";
    }
    
    Types::Count sum = m_latency.sum.load();
    Types::Count min_val = m_latency.min.load();
    Types::Count max_val = m_latency.max.load();
    
    std::lock_guard<std::mutex> lock(m_latency.samples_mutex);
    std::vector<Types::Count> samples = m_latency.samples;
    
    double mean = static_cast<double>(sum) / count;
    
    // Calculate percentiles
    std::sort(samples.begin(), samples.end());
    
    Types::Count p50 = samples.empty() ? 0 : samples[samples.size() * 0.5];
    Types::Count p95 = samples.empty() ? 0 : samples[samples.size() * 0.95];
    Types::Count p99 = samples.empty() ? 0 : samples[samples.size() * 0.99];
    
    std::ostringstream oss;
    oss << "Latency Statistics:\n"
         << "  Count: " << count << "\n"
         << "  Mean: " << std::fixed << std::setprecision(2) << mean << " μs\n"
         << "  Min: " << min_val << " μs\n"
         << "  Max: " << max_val << " μs\n"
         << "  P50: " << p50 << " μs\n"
         << "  P95: " << p95 << " μs\n"
         << "  P99: " << p99 << " μs\n";
    
    return oss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters.clear();
    m_gauges.clear();
    
    m_latency.count.store(0);
    m_latency.sum.store(0);
    m_latency.min.store(std::numeric_limits<Types::Count>::max());
    m_latency.max.store(0);
    
    {
        std::lock_guard<std::mutex> sample_lock(m_latency.samples_mutex);
        m_latency.samples.clear();
    }
}

std::string Metrics::getSummary() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ostringstream oss;
    oss << "=== Metrics Summary ===\n";
    
    // Counters
    if (!m_counters.empty()) {
        oss << "Counters:\n";
        for (const auto& [name, counter] : m_counters) {
            oss << "  " << name << ": " << counter.load() << "\n";
        }
        oss << "\n";
    }
    
    // Gauges
    if (!m_gauges.empty()) {
        oss << "Gauges:\n";
        for (const auto& [name, gauge] : m_gauges) {
            oss << "  " << name << ": " << gauge.load() << "\n";
        }
        oss << "\n";
    }
    
    // Latency
    if (m_latency.count.load() > 0) {
        oss << getLatencyStats();
    }
    
    return oss.str();
}

std::string Metrics::getJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ostringstream oss;
    oss << "{\n";
    
    // Counters
    if (!m_counters.empty()) {
        oss << "  \"counters\": {\n";
        bool first = true;
        for (const auto& [name, counter] : m_counters) {
            if (!first) oss << ",\n";
            oss << "    \"" << name << "\": " << counter.load();
            first = false;
        }
        oss << "\n  },\n";
    }
    
    // Gauges
    if (!m_gauges.empty()) {
        oss << "  \"gauges\": {\n";
        bool first = true;
        for (const auto& [name, gauge] : m_gauges) {
            if (!first) oss << ",\n";
            oss << "    \"" << name << "\": " << gauge.load();
            first = false;
        }
        oss << "\n  },\n";
    }
    
    // Latency
    if (m_latency.count.load() > 0) {
        Types::Count count = m_latency.count.load();
        Types::Count sum = m_latency.sum.load();
        double mean = static_cast<double>(sum) / count;
        
        oss << "  \"latency\": {\n"
            << "    \"count\": " << count << ",\n"
            << "    \"mean_us\": " << std::fixed << std::setprecision(2) << mean << ",\n"
            << "    \"min_us\": " << m_latency.min.load() << ",\n"
            << "    \"max_us\": " << m_latency.max.load() << "\n"
            << "  }\n";
    }
    
    oss << "}";
    return oss.str();
}

void Metrics::updateLatencySamples(Types::Count latency) {
    std::lock_guard<std::mutex> lock(m_latency.samples_mutex);
    
    if (m_latency.samples.size() >= LatencyStats::MAX_SAMPLES) {
        // Remove oldest sample (simple FIFO)
        m_latency.samples.erase(m_latency.samples.begin());
    }
    
    m_latency.samples.push_back(latency);
}

// StringUtils Implementation
namespace StringUtils {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;
    
    while (std::getline(iss, token, delimiter)) {
        result.push_back(token);
    }
    
    return result;
}

std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    if (parts.empty()) return "";
    
    std::ostringstream oss;
    oss << parts[0];
    
    for (size_t i = 1; i < parts.size(); ++i) {
        oss << delimiter << parts[i];
    }
    
    return oss.str();
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() && 
           str.substr(0, prefix.length()) == prefix;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.length() >= suffix.length() && 
           str.substr(str.length() - suffix.length()) == suffix;
}

} // namespace StringUtils

// TimeUtils Implementation
namespace TimeUtils {

std::string formatDuration(std::chrono::nanoseconds duration) {
    auto count = duration.count();
    
    if (count < 1000) {
        return std::to_string(count) + "ns";
    } else if (count < 1000000) {
        return std::to_string(count / 1000.0) + "μs";
    } else if (count < 1000000000) {
        return std::to_string(count / 1000000.0) + "ms";
    } else {
        return std::to_string(count / 1000000000.0) + "s";
    }
}

std::string formatTimestamp(std::chrono::steady_clock::time_point timestamp) {
    auto now = std::chrono::steady_clock::now();
    auto duration = timestamp.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::ostringstream oss;
    oss << ms << "ms";
    return oss.str();
}

std::chrono::steady_clock::time_point now() {
    return std::chrono::steady_clock::now();
}

Types::Count timestampToMicroseconds(std::chrono::steady_clock::time_point timestamp) {
    auto duration = timestamp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

} // namespace TimeUtils

// NetworkUtils Implementation
namespace NetworkUtils {

bool isValidMacAddress(const std::string& mac) {
    // Simple validation for MAC address format
    std::vector<std::string> parts = StringUtils::split(mac, ':');
    if (parts.size() != 6) return false;
    
    for (const auto& part : parts) {
        if (part.length() != 2) return false;
        
        for (char c : part) {
            if (!std::isxdigit(c)) return false;
        }
    }
    
    return true;
}

bool isValidIpAddress(const std::string& ip) {
    std::vector<std::string> parts = StringUtils::split(ip, '.');
    if (parts.size() != 4) return false;
    
    for (const auto& part : parts) {
        try {
            int value = std::stoi(part);
            if (value < 0 || value > 255) return false;
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

std::string formatMacAddress(const std::string& mac) {
    if (!isValidMacAddress(mac)) return mac;
    
    std::vector<std::string> parts = StringUtils::split(mac, ':');
    std::string result;
    
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += ":";
        result += StringUtils::toUpper(parts[i]);
    }
    
    return result;
}

std::string formatIpAddress(const std::string& ip) {
    if (!isValidIpAddress(ip)) return ip;
    return ip; // Already in correct format
}

Types::Count calculateChecksum(const std::vector<uint8_t>& data) {
    Types::Count checksum = 0;
    
    for (uint8_t byte : data) {
        checksum += byte;
    }
    
    return checksum & 0xFFFF; // 16-bit checksum
}

uint16_t htons(uint16_t hostshort) {
    // Convert host byte order to network byte order (big-endian)
    #if defined(_WIN32) || defined(_WIN64)
    return _byteswap_ushort(hostshort);
    #elif defined(__linux__)
    return __builtin_bswap16(hostshort);
    #else
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
    #endif
}

uint32_t htonl(uint32_t hostlong) {
    // Convert host byte order to network byte order (big-endian)
    #if defined(_WIN32) || defined(_WIN64)
    return _byteswap_ulong(hostlong);
    #elif defined(__linux__)
    return __builtin_bswap32(hostlong);
    #else
    return ((hostlong & 0xFF) << 24) | 
           ((hostlong & 0xFF00) << 8) | 
           ((hostlong >> 8) & 0xFF00) | 
           ((hostlong >> 24) & 0xFF);
    #endif
}

uint16_t ntohs(uint16_t netshort) {
    // Convert network byte order to host byte order
    return htons(netshort); // Same operation
}

uint32_t ntohl(uint32_t netlong) {
    // Convert network byte order to host byte order
    return htonl(netlong); // Same operation
}

} // namespace NetworkUtils

} // export namespace MiniSonic::Utils
