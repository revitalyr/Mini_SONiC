// Standard C++ header inclusions
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <array>
#include <bit>
#include <iomanip>
#include <immintrin.h> // For SIMD instructions

#ifdef WIN32
#include "common/types.hpp"
    // Windows-specific headers
    #include <windows.h>
    #include <process.h>
#else
    // Linux/Unix-specific headers
    #include <numa.h>      // For NUMA API (numa_available, etc.)
    #include <sched.h>      // For sched_getcpu
    #include <sys/mman.h>    // For memory mapping
    #include <fcntl.h>        // For file operations
    #include <unistd.h>       // For system calls
#endif

namespace MiniSonic::DataPlane::Optimized {

/** 
 * Definitions for classes referenced in implementation
 */

class OptimizedPacket {
public:
    OptimizedPacket(const std::string& src_mac, const std::string& dst_mac,
                   const std::string& src_ip, const std::string& dst_ip,
                   uint32_t ingress_port);
    const uint8_t* srcMac() const { return m_src_mac; }
    uint32_t srcIp() const { return m_src_ip; }
    uint32_t dstIp() const { return m_dst_ip; }
    static constexpr size_t size() { return sizeof(OptimizedPacket); }
private:
    uint8_t m_src_mac[6], m_dst_mac[6];
    uint32_t m_src_ip, m_dst_ip, m_ingress_port;
    uint64_t m_timestamp;
};

template<typename T, size_t Size>
class LockFreeRingBuffer {
public:
    static constexpr size_t BUFFER_MASK = Size - 1;
    bool tryPush(const T& item) noexcept {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t next = (head + 1) & BUFFER_MASK;
        if (next == m_tail.load(std::memory_order_acquire)) return false;
        m_buffer[head] = item;
        m_head.store(next, std::memory_order_release);
        return true;
    }
    bool tryPush(T&& item) noexcept {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t next = (head + 1) & BUFFER_MASK;
        if (next == m_tail.load(std::memory_order_acquire)) return false;
        m_buffer[head] = std::move(item);
        m_head.store(next, std::memory_order_release);
        return true;
    }
    bool tryPop(T& item) noexcept {
        size_t tail = m_tail.load(std::memory_order_acquire);
        if (tail == m_head.load(std::memory_order_acquire)) return false;
        item = std::move(m_buffer[tail]);
        m_tail.store((tail + 1) & BUFFER_MASK, std::memory_order_release);
        return true;
    }
    size_t tryPopBatch(T* buffer, size_t max_count) noexcept {
        size_t count = 0;
        while (count < max_count && tryPop(buffer[count])) count++;
        return count;
    }
    size_t size() const noexcept { return (m_head.load() - m_tail.load()) & BUFFER_MASK; }
    bool empty() const noexcept { return m_head.load() == m_tail.load(); }
    bool full() const noexcept { return ((m_head.load() + 1) & BUFFER_MASK) == m_tail.load(); }
    size_t capacity() const noexcept { return Size; }
private:
    std::array<T, Size> m_buffer;
    std::atomic<size_t> m_head{0}, m_tail{0};
};

struct PoolBlock {
    std::unique_ptr<uint8_t[]> memory;
    std::atomic<void*> next_free{nullptr};
    std::atomic<size_t> allocated_count{0}, free_count{0};
};

class PacketPool {
public:
    PacketPool(size_t pool_size, size_t packet_size);
    void* acquire() noexcept;
    void release(void* packet) noexcept;
    size_t size() const noexcept;
    size_t capacity() const noexcept;
    size_t allocated() const noexcept;
    std::string getStats() const;
private:
    size_t m_pool_size, m_packet_size;
    std::vector<std::unique_ptr<PoolBlock>> m_pools;
    std::atomic<size_t> m_next_pool{0}, m_total_allocated{0}, m_total_freed{0};
};

class MemoryMappedIO {
public:
    MemoryMappedIO(const std::string& filename, size_t size);
    ~MemoryMappedIO();
    void* data() noexcept;
    const void* data() const noexcept;
    size_t size() const noexcept;
    bool sync() noexcept;
    bool isValid() const noexcept;
private:
#ifdef WIN32
    HANDLE m_file_handle;
    HANDLE m_mapping_handle;
#else
    int m_file_descriptor;
#endif
    void* m_mapped_data;
    size_t m_mapped_size;
    bool m_is_valid;
};

class OptimizedProcessor {
public:
    OptimizedProcessor(size_t batch_size);
    void processBatch(OptimizedPacket* packets, size_t count) noexcept;
    void processPacketSIMD(OptimizedPacket& packet) noexcept;
    void processPacketScalar(OptimizedPacket& packet) noexcept;
    void prefetchBatch(OptimizedPacket* packets, size_t count) noexcept;
    uint64_t getPacketsProcessed() const noexcept;
    uint64_t getBytesProcessed() const noexcept;
    double getProcessingRate() const noexcept;
    std::string getStats() const;
private:
    size_t m_batch_size;
    uint8_t m_l2_table[256][6];
    uint32_t m_l3_table[1024];
    std::atomic<uint64_t> m_packets_processed{0}, m_bytes_processed{0}, m_start_time{0};
    std::atomic<size_t> m_l2_entries{0}, m_l3_entries{0};
};

class NumaAllocator {
public:
    NumaAllocator();
    void* allocate(size_t size, int numa_node) noexcept;
    void deallocate(void* ptr) noexcept;
    int getNodeCount() noexcept;
    int getCurrentNode() noexcept;
    bool isNumaAvailable() noexcept;
    void* allocateNumaAware(size_t size, int numa_node) noexcept;
    void* allocateStandard(size_t size) noexcept;
};

// PacketPool Implementation
PacketPool::PacketPool(size_t pool_size, size_t packet_size)
    : m_pool_size(pool_size), m_packet_size(packet_size) {
    
    std::cout << "[POOL] Creating packet pool: " << pool_size 
              << " packets of " << packet_size << " bytes each\n";
    
    // Create multiple smaller pools for better cache locality
    const size_t pools_count = std::thread::hardware_concurrency();
    const size_t packets_per_pool = pool_size / pools_count;
    
    m_pools.reserve(pools_count);
    
    for (size_t i = 0; i < pools_count; ++i) {
        auto block = std::make_unique<PoolBlock>();
        block->memory = std::make_unique<uint8_t[]>(packets_per_pool * packet_size);
        block->next_free.store(nullptr, std::memory_order_relaxed);
        block->allocated_count.store(0, std::memory_order_relaxed);
        block->free_count.store(0, std::memory_order_relaxed);
        
        // Initialize free list for this pool
        for (size_t j = 0; j < packets_per_pool; ++j) {
            void* packet_ptr = block->memory.get() + (j * packet_size);
            
            void* current_head = block->next_free.load(std::memory_order_relaxed);
            *static_cast<void**>(packet_ptr) = current_head;
            block->next_free.store(packet_ptr, std::memory_order_relaxed);
        }
        
        m_pools.push_back(std::move(block));
    }
}

void* PacketPool::acquire() noexcept {
    // Round-robin pool selection for better load balancing
    const size_t pool_index = m_next_pool.fetch_add(1, std::memory_order_relaxed) % m_pools.size();
    auto& pool = *m_pools[pool_index];
    
    void* packet = pool.next_free.load(std::memory_order_acquire);
    if (packet) {
        // Pop from free list
        void* next = *static_cast<void**>(packet);
        pool.next_free.store(next, std::memory_order_release);
        pool.allocated_count.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Pool exhausted - allocate directly
        packet = std::malloc(m_packet_size);
        if (!packet) {
            std::cerr << "[POOL] Allocation failed!\n";
            return nullptr;
        }
    }
    
    m_total_allocated.fetch_add(1, std::memory_order_relaxed);
    return packet;
}

void PacketPool::release(void* packet) noexcept {
    if (!packet) return;
    
    bool returned_to_pool = false;
    const size_t packets_per_pool = m_pool_size / m_pools.size();
    const size_t pool_capacity_bytes = packets_per_pool * m_packet_size;

    // Find which pool this packet belongs to
    for (auto& pool_ptr : m_pools) {
        uint8_t* pool_start = pool_ptr->memory.get();
        uint8_t* pool_end = pool_start + pool_capacity_bytes;
        
        if (packet >= pool_start && packet < pool_end) {
            // Return to pool's free list
            void* current_head = pool_ptr->next_free.load(std::memory_order_relaxed);
            *static_cast<void**>(packet) = current_head;
            pool_ptr->next_free.store(packet, std::memory_order_release);
            pool_ptr->free_count.fetch_add(1, std::memory_order_relaxed);
            returned_to_pool = true;
            break;
        }
    }
    
    if (!returned_to_pool) {
        std::free(packet);
    }
    
    m_total_freed.fetch_add(1, std::memory_order_relaxed);
}

size_t PacketPool::size() const noexcept {
    return m_total_allocated.load() - m_total_freed.load();
}

size_t PacketPool::capacity() const noexcept {
    return m_pool_size;
}

size_t PacketPool::allocated() const noexcept {
    return m_total_allocated.load();
}

std::string PacketPool::getStats() const {
    std::ostringstream oss;
    oss << "Packet Pool Statistics:\n"
         << "  Capacity: " << m_pool_size << "\n"
         << "  Allocated: " << m_total_allocated.load() << "\n"
         << "  Freed: " << m_total_freed.load() << "\n"
         << "  In Use: " << size() << "\n"
         << "  Pools: " << m_pools.size() << "\n";
    
    for (size_t i = 0; i < m_pools.size(); ++i) {
        const auto& pool = *m_pools[i];
        oss << "  Pool " << i << ": allocated=" << pool.allocated_count.load()
             << ", free=" << pool.free_count.load() << "\n";
    }
    
    return oss.str();
}

// OptimizedPacket Implementation
OptimizedPacket::OptimizedPacket(
    const std::string& src_mac,
    const std::string& dst_mac,
    const std::string& src_ip_str, // Renamed to avoid conflict with member
    const std::string& dst_ip_str, // Renamed to avoid conflict with member
    uint32_t ingress_port // Renamed to avoid conflict with member
) : m_ingress_port(ingress_port),
    m_timestamp(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    )) {
    
    // Use conversion helpers from common/types.hpp
    MiniSonic::Types::MacAddress src_mac_uint = MiniSonic::Types::macToUint64(src_mac);
    MiniSonic::Types::MacAddress dst_mac_uint = MiniSonic::Types::macToUint64(dst_mac);

    std::memcpy(m_src_mac, &src_mac_uint, 6);
    std::memcpy(m_dst_mac, &dst_mac_uint, 6);

    m_src_ip = MiniSonic::Types::ipToUint32(src_ip_str);
    m_dst_ip = MiniSonic::Types::ipToUint32(dst_ip_str);
}

// OptimizedProcessor Implementation
OptimizedProcessor::OptimizedProcessor(size_t batch_size)
    : m_batch_size(batch_size) {
    
    std::cout << "[PROCESSOR] Creating optimized processor with batch size " 
              << batch_size << "\n";
    
    // Initialize lookup tables
    std::memset(m_l2_table, 0, sizeof(m_l2_table));
    std::memset(m_l3_table, 0, sizeof(m_l3_table));
    
    m_start_time.store(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    ), std::memory_order_relaxed);
}

void OptimizedProcessor::processBatch(OptimizedPacket* packets, size_t count) noexcept {
    if (count == 0) return;
    prefetchBatch(packets, count);
    
    // Process packets
    for (size_t i = 0; i < count; ++i) {
        // Choose processing method based on packet count
        if (count >= 16) {
            processPacketSIMD(packets[i]);
        } else {
            processPacketScalar(packets[i]);
        }
        
        m_packets_processed.fetch_add(1, std::memory_order_relaxed);
        m_bytes_processed.fetch_add(OptimizedPacket::size(), std::memory_order_relaxed);
    }
}

void OptimizedProcessor::processPacketSIMD(OptimizedPacket& packet) noexcept {
    // SIMD-optimized MAC lookup
    const uint8_t* src_mac = packet.srcMac();
    
    // Use SIMD for fast MAC comparison (simplified example)
    __m128i mac_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_mac));
    [[maybe_unused]] __m128i result = _mm_cmpeq_epi8(mac_vec, _mm_setzero_si128());
    
    // Extract first byte for quick lookup
    int mac_hash = src_mac[0] ^ src_mac[1] ^ src_mac[2] ^ src_mac[3];
    size_t table_index = mac_hash & 0xFF;
    
    // Fast L2 lookup
    if (m_l2_table[table_index][0] != 0) {
        // Found in MAC table - forward
        // In real implementation, this would set output port
    } else {
        // Learn new MAC
        std::memcpy(m_l2_table[table_index], src_mac, 6);
        m_l2_entries++;
    }
    
    // SIMD-optimized IP lookup (simplified)
    uint32_t src_ip = packet.srcIp();
    uint32_t ip_hash = src_ip ^ (src_ip >> 16);
    size_t l3_index = ip_hash & 0x3FF; // 1024 entries
    
    if (m_l3_table[l3_index] != 0) {
        // Found in routing table - forward
        // In real implementation, this would set next hop
    } else {
        // Add default route
        m_l3_table[l3_index] = packet.dstIp();
        m_l3_entries++;
    }
}

void OptimizedProcessor::processPacketScalar(OptimizedPacket& packet) noexcept {
    // Scalar processing for small batches
    const uint8_t* src_mac = packet.srcMac();
    
    // Fast MAC hash calculation
    int mac_hash = src_mac[0] ^ src_mac[1] ^ src_mac[2] ^ src_mac[3];
    size_t table_index = mac_hash & 0xFF;
    
    // L2 lookup
    if (m_l2_table[table_index][0] != 0) {
        // Forward to known port
    } else {
        // Learn new MAC
        std::memcpy(m_l2_table[table_index], src_mac, 6);
        m_l2_entries++;
    }
    
    // IP routing
    uint32_t src_ip = packet.srcIp();
    uint32_t ip_hash = src_ip ^ (src_ip >> 16);
    size_t l3_index = ip_hash & 0x3FF;
    
    if (m_l3_table[l3_index] != 0) {
        // Forward to known route
    } else {
        // Add default route
        m_l3_table[l3_index] = packet.dstIp();
        m_l3_entries++;
    }
}

void OptimizedProcessor::prefetchBatch(OptimizedPacket* packets, size_t count) noexcept {
    // Prefetch packets into cache for better performance
    for (size_t i = 0; i < count; i += 8) {
        // Prefetch next 8 packets
        _mm_prefetch(reinterpret_cast<const char*>(&packets[i]), _MM_HINT_T0);
        if (i + 1 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 1]), _MM_HINT_T0);
        if (i + 2 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 2]), _MM_HINT_T0);
        if (i + 3 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 3]), _MM_HINT_T0);
        if (i + 4 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 4]), _MM_HINT_T0);
        if (i + 5 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 5]), _MM_HINT_T0);
        if (i + 6 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 6]), _MM_HINT_T0);
        if (i + 7 < count) _mm_prefetch(reinterpret_cast<const char*>(&packets[i + 7]), _MM_HINT_T0);
    }
}

uint64_t OptimizedProcessor::getPacketsProcessed() const noexcept {
    return m_packets_processed.load(std::memory_order_relaxed);
}

uint64_t OptimizedProcessor::getBytesProcessed() const noexcept {
    return m_bytes_processed.load(std::memory_order_relaxed);
}

double OptimizedProcessor::getProcessingRate() const noexcept {
    const uint64_t packets = getPacketsProcessed();
    const uint64_t start_time = m_start_time.load(std::memory_order_relaxed);
    
    if (packets == 0 || start_time == 0) return 0.0;
    
    const auto current_time = std::chrono::high_resolution_clock::now();
    const auto elapsed_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            current_time.time_since_epoch()
        ).count()
    ) - start_time;
    
    if (elapsed_us == 0) return 0.0;
    
    return (static_cast<double>(packets) * 1000000.0) / elapsed_us;
}

std::string OptimizedProcessor::getStats() const {
    std::ostringstream oss;
    oss << "Optimized Processor Statistics:\n"
         << "  Packets Processed: " << getPacketsProcessed() << "\n"
         << "  Bytes Processed: " << getBytesProcessed() << "\n"
         << "  Processing Rate: " << std::fixed << std::setprecision(2) 
         << getProcessingRate() << " packets/sec\n"
         << "  L2 Entries: " << m_l2_entries << "\n"
         << "  L3 Entries: " << m_l3_entries << "\n"
         << "  Batch Size: " << m_batch_size << "\n";
    
    return oss.str();
}

// MemoryMappedIO Implementation
MemoryMappedIO::MemoryMappedIO(const std::string& filename, size_t size)
    : m_mapped_size(size) {
    
#ifdef WIN32
    m_file_handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_file_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "[MMAP] Failed to create file: " << filename << "\n";
        return;
    }
    
    m_mapping_handle = CreateFileMapping(m_file_handle, nullptr, PAGE_READWRITE,
                                        0, static_cast<DWORD>(size), nullptr);
    if (m_mapping_handle == nullptr) {
        std::cerr << "[MMAP] Failed to create file mapping\n";
        CloseHandle(m_file_handle);
        m_file_handle = INVALID_HANDLE_VALUE;
        return;
    }
    
    m_mapped_data = MapViewOfFile(m_mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (m_mapped_data == nullptr) {
        std::cerr << "[MMAP] Failed to map file\n";
        CloseHandle(m_mapping_handle);
        CloseHandle(m_file_handle);
        m_file_handle = INVALID_HANDLE_VALUE;
        m_mapping_handle = nullptr;
        return;
    }
#else
    m_file_descriptor = open(filename.c_str(), O_RDWR | O_CREAT, 0644);
    if (m_file_descriptor == -1) {
        std::cerr << "[MMAP] Failed to open file: " << filename << "\n";
        return;
    }
    
    // Set file size
    if (ftruncate(m_file_descriptor, size) == -1) {
        std::cerr << "[MMAP] Failed to set file size\n";
        close(m_file_descriptor);
        m_file_descriptor = -1;
        return;
    }
    
    // Map file into memory (added missing offset argument)
    m_mapped_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_file_descriptor, 0);
    if (m_mapped_data == MAP_FAILED) {
        std::cerr << "[MMAP] Failed to map file\n";
        close(m_file_descriptor);
        m_file_descriptor = -1;
        return;
    }
#endif
    
    m_is_valid = true;
    
    std::cout << "[MMAP] Mapped " << size << " bytes from " << filename << "\n";
}

MemoryMappedIO::~MemoryMappedIO() {
    if (m_is_valid && m_mapped_data != nullptr) {
#ifdef WIN32
        UnmapViewOfFile(m_mapped_data);
        CloseHandle(m_mapping_handle);
        CloseHandle(m_file_handle);
#else
        munmap(m_mapped_data, m_mapped_size);
        close(m_file_descriptor);
#endif
        std::cout << "[MMAP] Unmapped " << m_mapped_size << " bytes\n";
    }
}

void* MemoryMappedIO::data() noexcept {
    return m_mapped_data;
}

const void* MemoryMappedIO::data() const noexcept {
    return m_mapped_data;
}

size_t MemoryMappedIO::size() const noexcept {
    return m_mapped_size;
}

bool MemoryMappedIO::sync() noexcept {
    if (!m_is_valid) return false;
    
#ifdef WIN32
    return FlushViewOfFile(m_mapped_data, m_mapped_size) != FALSE;
#else
    return msync(m_mapped_data, m_mapped_size, MS_SYNC) == 0;
#endif
}

bool MemoryMappedIO::isValid() const noexcept {
    return m_is_valid;
}

// NumaAllocator Implementation
NumaAllocator::NumaAllocator() {
    std::cout << "[NUMA] NUMA allocator initialized\n";
    std::cout << "  NUMA nodes: " << getNodeCount() << "\n";
    std::cout << "  Current node: " << getCurrentNode() << "\n";
    std::cout << "  NUMA available: " << (isNumaAvailable() ? "Yes" : "No") << "\n";
}

void* NumaAllocator::allocate(size_t size, int numa_node) noexcept {
    if (isNumaAvailable() && numa_node >= 0) {
        return allocateNumaAware(size, numa_node);
    } else {
        return allocateStandard(size);
    }
}

void NumaAllocator::deallocate(void* ptr) noexcept {
    if (!ptr) return;
    
#ifdef WIN32
    _aligned_free(ptr);
#else
    // Always use std::free because we don't track size for numa_free
    std::free(ptr);
#endif
}

int NumaAllocator::getNodeCount() noexcept {
#ifdef WIN32
    return 1; // Windows NUMA detection would require additional setup
#else
    if (isNumaAvailable()) {
        return numa_max_node() + 1;
    } else {
        return 1;
    }
#endif
}

int NumaAllocator::getCurrentNode() noexcept {
#ifdef WIN32
    return 0; // Windows CPU affinity would require additional setup
#else
    if (isNumaAvailable()) {
        return numa_node_of_cpu(sched_getcpu());
    } else {
        return 0;
    }
#endif
}

bool NumaAllocator::isNumaAvailable() noexcept {
#ifdef WIN32
    return false; // NUMA not available in this Windows build
#else
    return numa_available() != -1;
#endif
}

void* NumaAllocator::allocateNumaAware(size_t size, [[maybe_unused]] int numa_node) noexcept {
    // NUMA-aware allocation
    // Fallback to standard allocation to ensure we can free it safely without size tracking
    // return numa_alloc_onnode(size, numa_node); 
    return allocateStandard(size);
}

void* NumaAllocator::allocateStandard(size_t size) noexcept {
    // Standard allocation with alignment
#ifdef WIN32
    return _aligned_malloc(size, 64);
#else
    return std::aligned_alloc(64, size);
#endif
}

} // namespace MiniSonic::DataPlane::Optimized
