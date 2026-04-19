module;

// Import standard library modules
import <memory>;
import <string>;
import <vector>;
import <atomic>;
import <mutex>;
import <shared_mutex>;
import <thread>;
import <chrono>;
import <iostream>;
import <sstream>;
import <algorithm>;
import <bit>;
import <immintrin.h>;

export module MiniSonic.Utils.Optimized;

export namespace MiniSonic::Utils::Optimized {

/**
 * @brief Lock-free metrics with minimal contention
 * 
 * Uses atomic operations and per-thread local storage
 * to eliminate synchronization overhead.
 */
export class LockFreeMetrics {
public:
    static LockFreeMetrics& instance();
    
    // Thread-local counter operations (no synchronization)
    void incThreadLocal(const std::string& name, uint64_t value = 1) noexcept;
    void setThreadLocal(const std::string& name, uint64_t value) noexcept;
    
    // Global gauge operations (minimal synchronization)
    void setGauge(const std::string& name, double value) noexcept;
    double getGauge(const std::string& name) const noexcept;
    
    // Latency tracking with per-thread buffers
    void addLatency(uint64_t latency_ns) noexcept;
    
    // Aggregate operations
    void reset() noexcept;
    std::string getSummary() const noexcept;
    std::string getJson() const noexcept;

private:
    LockFreeMetrics() = default;
    ~LockFreeMetrics() = default;
    
    // Prevent copying
    LockFreeMetrics(const LockFreeMetrics&) = delete;
    LockFreeMetrics& operator=(const LockFreeMetrics&) = delete;
    
    struct alignas(64) Counter {
        std::atomic<uint64_t> value{0};
        char padding[64 - sizeof(std::atomic<uint64_t>)];
    };
    
    struct alignas(64) Gauge {
        std::atomic<double> value{0.0};
        char padding[64 - sizeof(std::atomic<double>)];
    };
    
    struct alignas(64) LatencyStats {
        std::atomic<uint64_t> count{0};
        std::atomic<uint64_t> sum{0};
        std::atomic<uint64_t> min{UINT64_MAX};
        std::atomic<uint64_t> max{0};
        char padding[64 - (4 * sizeof(std::atomic<uint64_t>))];
    };
    
    // Thread-local storage for counters
    thread_local static std::unordered_map<std::string, uint64_t> t_counters;
    thread_local static bool t_initialized = false;
    
    // Global storage for gauges and aggregated counters
    std::unordered_map<std::string, Counter> m_gauges;
    std::unordered_map<std::string, Counter> m_aggregated_counters;
    LatencyStats m_latency_stats;
    
    mutable std::shared_mutex m_aggregation_mutex;
    
    void aggregateThreadLocalCounters() noexcept;
};

/**
 * @brief High-performance string operations
 * 
 * Optimized string utilities with SIMD acceleration
 * and minimal memory allocations.
 */
export namespace OptimizedString {
    // SIMD-accelerated string operations
    std::string_view trimSIMD(std::string_view str) noexcept;
    std::vector<std::string_view> splitSIMD(std::string_view str, char delimiter) noexcept;
    bool equalsSIMD(std::string_view a, std::string_view b) noexcept;
    uint64_t hashSIMD(std::string_view str) noexcept;
    
    // Memory-efficient string building
    class StringBuilder {
    public:
        explicit StringBuilder(size_t initial_capacity = 256);
        ~StringBuilder() = default;
        
        // Disable copying
        StringBuilder(const StringBuilder&) = delete;
        StringBuilder& operator=(const StringBuilder&) = delete;
        
        // Append operations
        StringBuilder& append(std::string_view str) noexcept;
        StringBuilder& append(char ch) noexcept;
        StringBuilder& append(const char* str, size_t len) noexcept;
        
        // Conversion
        std::string toString() && noexcept;
        std::string_view toStringView() const noexcept;
        
        // Capacity management
        void reserve(size_t capacity) noexcept;
        void clear() noexcept;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] size_t capacity() const noexcept;
        
    private:
        std::unique_ptr<char[]> m_buffer;
        size_t m_size{0};
        size_t m_capacity{0};
        
        void ensureCapacity(size_t required) noexcept;
    };
}

/**
 * @brief Lock-free data structures
 * 
 * High-performance concurrent data structures
 * with minimal synchronization overhead.
 */
export namespace LockFreeStructures {
    // Lock-free stack
    template<typename T>
    class LockFreeStack {
    public:
        LockFreeStack() = default;
        ~LockFreeStack() = default;
        
        // Disable copying
        LockFreeStack(const LockFreeStack&) = delete;
        LockFreeStack& operator=(const LockFreeStack&) = delete;
        
        bool push(const T& item) noexcept;
        bool push(T&& item) noexcept;
        bool pop(T& item) noexcept;
        [[nodiscard]] bool empty() const noexcept;
        
    private:
        struct Node {
            T data;
            Node* next;
        };
        
        std::atomic<Node*> m_head{nullptr};
        std::atomic<Node*> m_to_be_deleted{nullptr};
        
        void retireNodes() noexcept;
    };
    
    // Lock-free queue (optimized version)
    template<typename T>
    class OptimizedQueue {
    public:
        explicit OptimizedQueue(size_t capacity);
        ~OptimizedQueue() = default;
        
        // Disable copying
        OptimizedQueue(const OptimizedQueue&) = delete;
        OptimizedQueue& operator=(const OptimizedQueue&) = delete;
        
        bool tryPush(const T& item) noexcept;
        bool tryPush(T&& item) noexcept;
        bool tryPop(T& item) noexcept;
        
        // Batch operations
        size_t tryPushBatch(const T* items, size_t count) noexcept;
        size_t tryPopBatch(T* items, size_t max_count) noexcept;
        
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] size_t capacity() const noexcept;
        
    private:
        struct alignas(64) Slot {
            std::atomic<uint64_t> sequence{0};
            alignas(alignof(T)) T data;
            std::atomic<bool> ready{false};
        };
        
        std::vector<Slot> m_buffer;
        const size_t m_buffer_mask;
        std::atomic<size_t> m_head{0};
        std::atomic<size_t> m_tail{0};
        
        static constexpr size_t BUFFER_SIZE = 1; // Power of 2
    };
}

/**
 * @brief Memory optimization utilities
 * 
 * Advanced memory management and optimization techniques.
 */
export namespace MemoryOptimization {
    // Cache-line aligned allocator
    template<typename T>
    class CacheLineAllocator {
    public:
        static constexpr size_t CACHE_LINE_SIZE = 64;
        
        [[nodiscard]] static T* allocate(size_t count = 1) noexcept;
        static void deallocate(T* ptr) noexcept;
        
        // Aligned new/delete operators
        static void* operator new(size_t size) noexcept;
        static void operator delete(void* ptr) noexcept;
        
    private:
        static constexpr size_t ALIGNMENT = alignof(T) > CACHE_LINE_SIZE ? 
                                      alignof(T) : CACHE_LINE_SIZE;
    };
    
    // Object pool for small objects
    template<typename T, size_t PoolSize = 1024>
    class ObjectPool {
    public:
        ObjectPool() = default;
        ~ObjectPool() = default;
        
        // Disable copying
        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;
        
        [[nodiscard]] T* acquire() noexcept;
        void release(T* obj) noexcept;
        
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] size_t capacity() const noexcept;
        [[nodiscard]] std::string getStats() const;
        
    private:
        struct alignas(64) PoolSlot {
            alignas(T) std::byte storage[sizeof(T)];
            std::atomic<bool> in_use{false};
        };
        
        std::array<PoolSlot, PoolSize> m_slots;
        std::atomic<size_t> m_next_slot{0};
        std::atomic<size_t> m_allocated_count{0};
    };
    
    // Memory-mapped buffer utilities
    class MappedBuffer {
    public:
        MappedBuffer(const std::string& filename, size_t size);
        ~MappedBuffer();
        
        // Disable copying
        MappedBuffer(const MappedBuffer&) = delete;
        MappedBuffer& operator=(const MappedBuffer&) = delete;
        
        [[nodiscard]] void* data() noexcept;
        [[nodiscard]] const void* data() const noexcept;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool isValid() const noexcept;
        
        bool sync() noexcept;
        
    private:
        void* m_mapped_data{nullptr};
        size_t m_mapped_size{0};
        int m_file_descriptor{-1};
        bool m_is_valid{false};
    };
}

/**
 * @brief CPU optimization utilities
 * 
 * CPU-specific optimizations and feature detection.
 */
export namespace CpuOptimization {
    // CPU feature detection
    struct CpuFeatures {
        bool has_sse2{false};
        bool has_sse3{false};
        bool has_sse4_1{false};
        bool has_sse4_2{false};
        bool has_avx{false};
        bool has_avx2{false};
        bool has_avx512{false};
        bool has_bmi1{false};
        bool has_bmi2{false};
        size_t cache_line_size{64};
        size_t l1_cache_size{32768};
        size_t l2_cache_size{262144};
        size_t l3_cache_size{8388608};
    };
    
    // Feature detection
    [[nodiscard]] const CpuFeatures& detectFeatures() noexcept;
    [[nodiscard]] bool hasFeature(const std::string& feature) const noexcept;
    
    // CPU-specific optimizations
    void prefetchData(const void* addr) noexcept;
    void prefetchInstruction(const void* addr) noexcept;
    void memoryBarrier() noexcept;
    void pauseCpu() noexcept;
    
    // Thread affinity
    bool setThreadAffinity(int cpu_id) noexcept;
    int getCurrentCpu() noexcept;
    void setRealtimePriority() noexcept;
    
    // Cache optimization
    void flushCacheLine(const void* addr) noexcept;
    void prefetchCacheLine(const void* addr) noexcept;
}

} // export namespace MiniSonic::Utils.Optimized
