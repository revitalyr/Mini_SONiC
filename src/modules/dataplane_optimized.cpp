module;

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
#include <bit>
#include <immintrin.h> // For SIMD instructions
#include <sched.h>      // For NUMA
#include <sys/mman.h>    // For memory mapping
#include <fcntl.h>        // For file operations
#include <unistd.h>       // For system calls

module MiniSonic.DataPlane.Optimized;

namespace MiniSonic::DataPlane::Optimized {
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
        PoolBlock block;
        block.memory = std::make_unique<uint8_t[]>(packets_per_pool * packet_size);
        block.next_free.store(nullptr, std::memory_order_relaxed);
        block.allocated_count.store(0, std::memory_order_relaxed);
        block.free_count.store(0, std::memory_order_relaxed);
        
        // Initialize free list for this pool
        for (size_t j = 0; j < packets_per_pool; ++j) {
            void* packet_ptr = block.memory.get() + (j * packet_size);
            
            void* current_head = block.next_free.load(std::memory_order_relaxed);
            *static_cast<void**>(packet_ptr) = current_head;
            block.next_free.store(packet_ptr, std::memory_order_relaxed);
        }
        
        m_pools.push_back(std::move(block));
    }
}

void* PacketPool::acquire() noexcept {
    // Round-robin pool selection for better load balancing
    const size_t pool_index = m_next_pool.fetch_add(1, std::memory_order_relaxed) % m_pools.size();
    auto& pool = m_pools[pool_index];
    
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
    
    // Find which pool this packet belongs to
    for (auto& pool : m_pools) {
        uint8_t* pool_start = pool.memory.get();
        uint8_t* pool_end = pool_start + (pool.allocated_count.load() * m_packet_size);
        
        if (packet >= pool_start && packet < pool_end) {
            // Return to pool's free list
            void* current_head = pool.next_free.load(std::memory_order_relaxed);
            *static_cast<void**>(packet) = current_head;
            pool.next_free.store(packet, std::memory_order_release);
            pool.free_count.fetch_add(1, std::memory_order_relaxed);
            break;
        }
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
        const auto& pool = m_pools[i];
        oss << "  Pool " << i << ": allocated=" << pool.allocated_count.load()
             << ", free=" << pool.free_count.load() << "\n";
    }
    
    return oss.str();
}

// LockFreeRingBuffer Implementation
template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPush(const T& item) noexcept {
    const size_t current_head = m_head.load(std::memory_order_acquire);
    const size_t next_head = (current_head + 1) & BUFFER_MASK;
    
    // Check if buffer is full
    if (next_head == m_tail.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Copy item to buffer
    m_buffer[current_head] = item;
    
    // Update head atomically
    m_head.store(next_head, std::memory_order_release);
    return true;
}

template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPush(T&& item) noexcept {
    const size_t current_head = m_head.load(std::memory_order_acquire);
    const size_t next_head = (current_head + 1) & BUFFER_MASK;
    
    // Check if buffer is full
    if (next_head == m_tail.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Move item to buffer
    m_buffer[current_head] = std::move(item);
    
    // Update head atomically
    m_head.store(next_head, std::memory_order_release);
    return true;
}

template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::tryPop(T& item) noexcept {
    const size_t current_tail = m_tail.load(std::memory_order_acquire);
    const size_t current_head = m_head.load(std::memory_order_acquire);
    
    // Check if buffer is empty
    if (current_tail == current_head) {
        return false;
    }
    
    // Copy item from buffer
    item = m_buffer[current_tail];
    
    // Update tail atomically
    const size_t next_tail = (current_tail + 1) & BUFFER_MASK;
    m_tail.store(next_tail, std::memory_order_release);
    return true;
}

template<typename T, size_t Size>
size_t LockFreeRingBuffer<T, Size>::tryPopBatch(T* buffer, size_t max_count) noexcept {
    size_t popped = 0;
    
    while (popped < max_count && tryPop(buffer[popped])) {
        ++popped;
    }
    
    return popped;
}

template<typename T, size_t Size>
size_t LockFreeRingBuffer<T, Size>::size() const noexcept {
    const size_t head = m_head.load(std::memory_order_acquire);
    const size_t tail = m_tail.load(std::memory_order_acquire);
    
    if (head >= tail) {
        return head - tail;
    } else {
        return Size - (tail - head);
    }
}

template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::empty() const noexcept {
    return m_head.load(std::memory_order_acquire) == 
           m_tail.load(std::memory_order_acquire);
}

template<typename T, size_t Size>
bool LockFreeRingBuffer<T, Size>::full() const noexcept {
    const size_t next_head = (m_head.load(std::memory_order_acquire) + 1) & BUFFER_MASK;
    return next_head == m_tail.load(std::memory_order_acquire);
}

template<typename T, size_t Size>
size_t LockFreeRingBuffer<T, Size>::capacity() const noexcept {
    return Size;
}

// OptimizedPacket Implementation
OptimizedPacket::OptimizedPacket(
    const std::string& src_mac,
    const std::string& dst_mac,
    const std::string& src_ip,
    const std::string& dst_ip,
    uint32_t ingress_port
) : m_ingress_port(ingress_port),
    m_timestamp(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count()
    )) {
    
    // Parse and pack MAC addresses
    auto parse_mac = [](const std::string& mac_str, uint8_t* mac_array) {
        std::istringstream iss(mac_str);
        std::string octet;
        for (int i = 0; i < 6; ++i) {
            std::getline(iss, octet, ':');
            mac_array[i] = static_cast<uint8_t>(std::stoi(octet, nullptr, 16));
        }
    };
    
    parse_mac(src_mac, m_src_mac);
    parse_mac(dst_mac, m_dst_mac);
    
    // Parse and pack IP addresses
    auto parse_ip = [](const std::string& ip_str) -> uint32_t {
        std::istringstream iss(ip_str);
        std::string octet;
        uint32_t ip = 0;
        for (int i = 0; i < 4; ++i) {
            std::getline(iss, octet, '.');
            ip = (ip << 8) | static_cast<uint32_t>(std::stoi(octet));
        }
        return ip;
    };
    
    m_src_ip = parse_ip(src_ip);
    m_dst_ip = parse_ip(dst_ip);
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
    
    // Prefetch batch for better cache performance
    prefetchBatch(packets, count);
    
    const auto start_time = std::chrono::high_resolution_clock::now();
    
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
    
    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time
    ).count();
    
    // Update statistics
    // Note: In real implementation, this would use metrics module
}

void OptimizedProcessor::processPacketSIMD(OptimizedPacket& packet) noexcept {
    // SIMD-optimized MAC lookup
    const uint8_t* src_mac = packet.srcMac();
    
    // Use SIMD for fast MAC comparison (simplified example)
    __m128i mac_vec = _mm_loadu_si128(src_mac);
    __m128i result = _mm_cmpeq_epi8(mac_vec, _mm_setzero_si128());
    
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
        _mm_prefetch(&packets[i], _MM_HINT_T0);
        if (i + 1 < count) _mm_prefetch(&packets[i + 1], _MM_HINT_T0);
        if (i + 2 < count) _mm_prefetch(&packets[i + 2], _MM_HINT_T0);
        if (i + 3 < count) _mm_prefetch(&packets[i + 3], _MM_HINT_T0);
        if (i + 4 < count) _mm_prefetch(&packets[i + 4], _MM_HINT_T0);
        if (i + 5 < count) _mm_prefetch(&packets[i + 5], _MM_HINT_T0);
        if (i + 6 < count) _mm_prefetch(&packets[i + 6], _MM_HINT_T0);
        if (i + 7 < count) _mm_prefetch(&packets[i + 7], _MM_HINT_T0);
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
    
    // Map file into memory
    m_mapped_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_file_descriptor);
    if (m_mapped_data == MAP_FAILED) {
        std::cerr << "[MMAP] Failed to map file\n";
        close(m_file_descriptor);
        m_file_descriptor = -1;
        return;
    }
    
    m_is_valid = true;
    
    std::cout << "[MMAP] Mapped " << size << " bytes from " << filename << "\n";
}

MemoryMappedIO::~MemoryMappedIO() {
    if (m_is_valid && m_mapped_data != nullptr) {
        munmap(m_mapped_data, m_mapped_size);
        std::cout << "[MMAP] Unmapped " << m_mapped_size << " bytes\n";
    }
    
    if (m_file_descriptor != -1) {
        close(m_file_descriptor);
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
    
    return msync(m_mapped_data, m_mapped_size, MS_SYNC) == 0;
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
    
    if (isNumaAvailable()) {
        numa_free(ptr);
    } else {
        std::free(ptr);
    }
}

int NumaAllocator::getNodeCount() noexcept {
    if (isNumaAvailable()) {
        return numa_max_node() + 1;
    } else {
        return 1;
    }
}

int NumaAllocator::getCurrentNode() noexcept {
    if (isNumaAvailable()) {
        return numa_node_of_cpu(sched_getcpu());
    } else {
        return 0;
    }
}

bool NumaAllocator::isNumaAvailable() noexcept {
    return numa_available() != -1;
}

void* NumaAllocator::allocateNumaAware(size_t size, int numa_node) noexcept {
    // NUMA-aware allocation
    return numa_alloc_onnode(size, numa_node);
}

void* NumaAllocator::allocateStandard(size_t size) noexcept {
    // Standard allocation with alignment
    void* ptr = std::aligned_alloc(64, size);
    return ptr;
}

} // export namespace MiniSonic::DataPlane::Optimized
