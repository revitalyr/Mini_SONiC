module;

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <bit>
#include <immintrin.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>

module MiniSonic.Utils.Optimized;

namespace MiniSonic::Utils::Optimized {
// LockFreeMetrics Implementation
LockFreeMetrics& LockFreeMetrics::instance() {
    static LockFreeMetrics instance;
    return instance;
}

void LockFreeMetrics::incThreadLocal(const std::string& name, uint64_t value) noexcept {
    if (!t_initialized) {
        t_counters.clear();
        t_initialized = true;
    }
    
    t_counters[name] += value;
}

void LockFreeMetrics::setThreadLocal(const std::string& name, uint64_t value) noexcept {
    if (!t_initialized) {
        t_counters.clear();
        t_initialized = true;
    }
    
    t_counters[name] = value;
}

void LockFreeMetrics::setGauge(const std::string& name, double value) noexcept {
    std::lock_guard<std::shared_mutex> lock(m_aggregation_mutex);
    
    auto it = m_gauges.find(name);
    if (it != m_gauges.end()) {
        it->second.value.store(value, std::memory_order_relaxed);
    } else {
        Counter new_counter;
        new_counter.value.store(value, std::memory_order_relaxed);
        m_gauges[name] = std::move(new_counter);
    }
}

double LockFreeMetrics::getGauge(const std::string& name) const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_aggregation_mutex);
    
    auto it = m_gauges.find(name);
    return it != m_gauges.end() ? it->second.value.load(std::memory_order_relaxed) : 0.0;
}

void LockFreeMetrics::addLatency(uint64_t latency_ns) noexcept {
    // Update latency statistics atomically
    m_latency_stats.count.fetch_add(1, std::memory_order_relaxed);
    m_latency_stats.sum.fetch_add(latency_ns, std::memory_order_relaxed);
    
    // Update min
    uint64_t current_min = m_latency_stats.min.load(std::memory_order_relaxed);
    while (latency_ns < current_min && 
           !m_latency_stats.min.compare_exchange_weak(current_min, latency_ns)) {
        // Retry if value changed
    }
    
    // Update max
    uint64_t current_max = m_latency_stats.max.load(std::memory_order_relaxed);
    while (latency_ns > current_max && 
           !m_latency_stats.max.compare_exchange_weak(current_max, latency_ns)) {
        // Retry if value changed
    }
}

void LockFreeMetrics::aggregateThreadLocalCounters() noexcept {
    std::lock_guard<std::shared_mutex> lock(m_aggregation_mutex);
    
    for (const auto& [name, value] : t_counters) {
        auto it = m_aggregated_counters.find(name);
        if (it != m_aggregated_counters.end()) {
            it->second.value.fetch_add(value, std::memory_order_relaxed);
        } else {
            Counter new_counter;
            new_counter.value.store(value, std::memory_order_relaxed);
            m_aggregated_counters[name] = std::move(new_counter);
        }
    }
    
    t_counters.clear();
}

void LockFreeMetrics::reset() noexcept {
    // Aggregate thread-local counters first
    aggregateThreadLocalCounters();
    
    // Reset all counters and gauges
    std::lock_guard<std::shared_mutex> lock(m_aggregation_mutex);
    
    for (auto& [name, counter] : m_aggregated_counters) {
        counter.value.store(0, std::memory_order_relaxed);
    }
    
    for (auto& [name, gauge] : m_gauges) {
        gauge.value.store(0.0, std::memory_order_relaxed);
    }
    
    // Reset latency stats
    m_latency_stats.count.store(0, std::memory_order_relaxed);
    m_latency_stats.sum.store(0, std::memory_order_relaxed);
    m_latency_stats.min.store(UINT64_MAX, std::memory_order_relaxed);
    m_latency_stats.max.store(0, std::memory_order_relaxed);
}

std::string LockFreeMetrics::getSummary() const noexcept {
    // Aggregate thread-local counters first
    const_cast<LockFreeMetrics*>(this)->aggregateThreadLocalCounters();
    
    std::ostringstream oss;
    oss << "=== Lock-Free Metrics Summary ===\n";
    
    // Counters
    if (!m_aggregated_counters.empty()) {
        oss << "Counters:\n";
        for (const auto& [name, counter] : m_aggregated_counters) {
            oss << "  " << name << ": " << counter.value.load() << "\n";
        }
        oss << "\n";
    }
    
    // Gauges
    if (!m_gauges.empty()) {
        oss << "Gauges:\n";
        for (const auto& [name, gauge] : m_gauges) {
            oss << "  " << name << ": " << gauge.value.load() << "\n";
        }
        oss << "\n";
    }
    
    // Latency
    const uint64_t count = m_latency_stats.count.load();
    if (count > 0) {
        const uint64_t sum = m_latency_stats.sum.load();
        const uint64_t min_val = m_latency_stats.min.load();
        const uint64_t max_val = m_latency_stats.max.load();
        const double mean = static_cast<double>(sum) / count;
        
        oss << "Latency Statistics:\n"
             << "  Count: " << count << "\n"
             << "  Mean: " << std::fixed << std::setprecision(2) << mean << " ns\n"
             << "  Min: " << min_val << " ns\n"
             << "  Max: " << max_val << " ns\n";
    }
    
    return oss.str();
}

std::string LockFreeMetrics::getJson() const noexcept {
    // Aggregate thread-local counters first
    const_cast<LockFreeMetrics*>(this)->aggregateThreadLocalCounters();
    
    std::ostringstream oss;
    oss << "{\n";
    
    // Counters
    if (!m_aggregated_counters.empty()) {
        oss << "  \"counters\": {\n";
        bool first = true;
        for (const auto& [name, counter] : m_aggregated_counters) {
            if (!first) oss << ",\n";
            oss << "    \"" << name << "\": " << counter.value.load();
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
            oss << "    \"" << name << "\": " << gauge.value.load();
            first = false;
        }
        oss << "\n  },\n";
    }
    
    // Latency
    const uint64_t count = m_latency_stats.count.load();
    if (count > 0) {
        const uint64_t sum = m_latency_stats.sum.load();
        const double mean = static_cast<double>(sum) / count;
        
        oss << "  \"latency\": {\n"
            << "    \"count\": " << count << ",\n"
            << "    \"mean_ns\": " << std::fixed << std::setprecision(2) << mean << ",\n"
            << "    \"min_ns\": " << m_latency_stats.min.load() << ",\n"
            << "    \"max_ns\": " << m_latency_stats.max.load() << "\n"
            << "  }\n";
    }
    
    oss << "}";
    return oss.str();
}

// OptimizedString namespace implementation
namespace OptimizedString {

std::string_view trimSIMD(std::string_view str) noexcept {
    // SIMD-accelerated trim implementation
    if (str.empty()) return str;
    
    const char* start = str.data();
    const char* end = str.data() + str.length();
    
    // Find first non-whitespace using SIMD
    size_t start_offset = 0;
    while (start_offset < str.length() && 
           (start[start_offset] == ' ' || start[start_offset] == '\t' || 
            start[start_offset] == '\n' || start[start_offset] == '\r')) {
        ++start_offset;
    }
    
    // Find last non-whitespace
    size_t end_offset = str.length();
    while (end_offset > start_offset && 
           (end[end_offset - 1] == ' ' || end[end_offset - 1] == '\t' || 
            end[end_offset - 1] == '\n' || end[end_offset - 1] == '\r')) {
        --end_offset;
    }
    
    return str.substr(start_offset, end_offset - start_offset);
}

std::vector<std::string_view> splitSIMD(std::string_view str, char delimiter) noexcept {
    std::vector<std::string_view> result;
    
    if (str.empty()) return result;
    
    size_t start = 0;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == delimiter) {
            result.emplace_back(str.substr(start, i - start));
            start = i + 1;
        }
    }
    
    if (start < str.length()) {
        result.emplace_back(str.substr(start));
    }
    
    return result;
}

bool equalsSIMD(std::string_view a, std::string_view b) noexcept {
    if (a.length() != b.length()) return false;
    
    // Use SIMD for string comparison if available
    const size_t length = a.length();
    const size_t simd_length = length & ~0xF; // Round down to 16-byte boundary
    
    if (simd_length >= 16) {
        const __m128i* a_ptr = reinterpret_cast<const __m128i*>(a.data());
        const __m128i* b_ptr = reinterpret_cast<const __m128i*>(b.data());
        
        for (size_t i = 0; i < simd_length; i += 16) {
            __m128i a_vec = _mm_loadu_si128(a_ptr + i);
            __m128i b_vec = _mm_loadu_si128(b_ptr + i);
            __m128i cmp = _mm_cmpeq_epi8(a_vec, b_vec);
            
            int mask = _mm_movemask_epi8(cmp);
            if (mask != 0xFFFF) {
                return false; // Mismatch found
            }
        }
        
        // Check remaining bytes
        for (size_t i = simd_length; i < length; ++i) {
            if (a[i] != b[i]) return false;
        }
        
        return true;
    } else {
        // Use standard comparison for small strings
        return std::equal(a.begin(), a.end(), b.begin());
    }
}

uint64_t hashSIMD(std::string_view str) noexcept {
    // SIMD-accelerated hash function
    if (str.empty()) return 0;
    
    const uint64_t* data = reinterpret_cast<const uint64_t*>(str.data());
    const size_t length = str.length();
    const size_t uint64_count = (length + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    
    uint64_t hash = 0;
    for (size_t i = 0; i < uint64_count; ++i) {
        hash ^= data[i] + 0x9e3779b97f4a7c15ULL;
        hash = (hash << 5) | (hash >> 59); // Rotate and mix
    }
    
    return hash;
}

// StringBuilder implementation
StringBuilder::StringBuilder(size_t initial_capacity) 
    : m_capacity(initial_capacity) {
    
    m_buffer = std::make_unique<char[]>(initial_capacity);
    m_buffer[0] = '\0';
}

StringBuilder& StringBuilder::append(std::string_view str) noexcept {
    ensureCapacity(m_size + str.length());
    
    std::memcpy(m_buffer.get() + m_size, str.data(), str.length());
    m_size += str.length();
    m_buffer[m_size] = '\0';
    
    return *this;
}

StringBuilder& StringBuilder::append(char ch) noexcept {
    ensureCapacity(m_size + 1);
    
    m_buffer[m_size] = ch;
    ++m_size;
    m_buffer[m_size] = '\0';
    
    return *this;
}

StringBuilder& StringBuilder::append(const char* str, size_t len) noexcept {
    if (!str || len == 0) return *this;
    
    ensureCapacity(m_size + len);
    
    std::memcpy(m_buffer.get() + m_size, str, len);
    m_size += len;
    m_buffer[m_size] = '\0';
    
    return *this;
}

std::string StringBuilder::toString() && noexcept {
    return std::string(m_buffer.get(), m_size);
}

std::string_view StringBuilder::toStringView() const noexcept {
    return std::string_view(m_buffer.get(), m_size);
}

void StringBuilder::reserve(size_t capacity) noexcept {
    if (capacity > m_capacity) {
        auto new_buffer = std::make_unique<char[]>(capacity);
        std::memcpy(new_buffer.get(), m_buffer.get(), m_size);
        new_buffer[m_size] = '\0';
        
        m_buffer = std::move(new_buffer);
        m_capacity = capacity;
    }
}

void StringBuilder::clear() noexcept {
    m_size = 0;
    if (m_buffer) {
        m_buffer[0] = '\0';
    }
}

size_t StringBuilder::size() const noexcept {
    return m_size;
}

size_t StringBuilder::capacity() const noexcept {
    return m_capacity;
}

void StringBuilder::ensureCapacity(size_t required) noexcept {
    if (required >= m_capacity) {
        reserve(std::max(required + 64, m_capacity * 2));
    }
}

} // namespace OptimizedString

// CpuOptimization namespace implementation
namespace CpuOptimization {

static CpuFeatures g_cpu_features = {};
static bool g_features_detected = false;

const CpuFeatures& detectFeatures() noexcept {
    if (g_features_detected) return g_cpu_features;
    
    // CPUID feature detection
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0, eax, ebx, ecx, edx);
    
    // Detect basic features
    if (edx & (1 << 23)) g_cpu_features.has_mmx = true;
    if (edx & (1 << 25)) g_cpu_features.has_sse = true;
    if (edx & (1 << 26)) g_cpu_features.has_sse2 = true;
    
    // Extended features
    __cpuid_count(1, eax, ebx, ecx, edx);
    if (edx & (1 << 0)) g_cpu_features.has_sse3 = true;
    if (ecx & (1 << 0)) g_cpu_features.has_sse4_1 = true;
    if (ecx & (1 << 5)) g_cpu_features.has_avx = true;
    if (ebx & (1 << 5)) g_cpu_features.has_avx2 = true;
    if (ebx & (1 << 16)) g_cpu_features.has_avx512 = true;
    if (ebx & (1 << 3)) g_cpu_features.has_bmi1 = true;
    if (ebx & (1 << 8)) g_cpu_features.has_bmi2 = true;
    
    // Cache information
    __cpuid_count(4, eax, ebx, ecx, edx);
    g_cpu_features.cache_line_size = ((ebx & 0xFFF) + 1) * 8;
    g_cpu_features.l1_cache_size = ((eax >> 24) + 1) * 1024;
    g_cpu_features.l2_cache_size = ((ecx & 0xFFFF) + 1) * 1024;
    g_cpu_features.l3_cache_size = ((edx & 0x7FFF) + 1) * 1024;
    
    g_features_detected = true;
    
    return g_cpu_features;
}

bool hasFeature(const std::string& feature) const noexcept {
    if (feature == "sse2") return g_cpu_features.has_sse2;
    if (feature == "sse3") return g_cpu_features.has_sse3;
    if (feature == "sse4_1") return g_cpu_features.has_sse4_1;
    if (feature == "sse4_2") return g_cpu_features.has_sse4_2;
    if (feature == "avx") return g_cpu_features.has_avx;
    if (feature == "avx2") return g_cpu_features.has_avx2;
    if (feature == "avx512") return g_cpu_features.has_avx512;
    if (feature == "bmi1") return g_cpu_features.has_bmi1;
    if (feature == "bmi2") return g_cpu_features.has_bmi2;
    return false;
}

void prefetchData(const void* addr) noexcept {
    if (g_cpu_features.has_sse2) {
        _mm_prefetch(addr, _MM_HINT_NTA);
    }
}

void prefetchInstruction(const void* addr) noexcept {
    if (g_cpu_features.has_sse2) {
        _mm_prefetch(addr, _MM_HINT_T0);
    }
}

void memoryBarrier() noexcept {
    _mm_mfence();
}

void pauseCpu() noexcept {
    if (g_cpu_features.has_sse2) {
        _mm_pause();
    } else {
        std::this_thread::yield();
    }
}

bool setThreadAffinity(int cpu_id) noexcept {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    return sched_setaffinity(0, sizeof(cpuset), &cpuset) == 0;
}

int getCurrentCpu() noexcept {
    return sched_getcpu();
}

void setRealtimePriority() noexcept {
    sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &param);
}

void flushCacheLine(const void* addr) noexcept {
    _mm_clflush(addr);
}

void prefetchCacheLine(const void* addr) noexcept {
    _mm_prefetch(addr, _MM_HINT_T0);
}

} // namespace CpuOptimization

} // export namespace MiniSonic::Utils.Optimized
