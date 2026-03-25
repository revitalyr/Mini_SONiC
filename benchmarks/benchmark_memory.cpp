#include <benchmark/benchmark.h>
#include "dataplane/packet.h"
#include "dataplane/pipeline.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "l3/lpm_trie.h"
#include "sai/simulated_sai.h"
#include "utils/spsc_queue.hpp"
#include "utils/metrics.hpp"
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

using namespace MiniSonic;

/**
 * @file benchmark_memory.cpp
 * @brief Benchmarks for memory usage analysis
 * 
 * This file contains comprehensive benchmarks for measuring memory usage
 * of various components including packet storage, data structures,
 * and memory efficiency analysis.
 */

// Memory measurement utilities
class MemoryProfiler {
public:
    static size_t getCurrentMemoryUsage() {
#ifdef __linux__
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss * 1024; // Convert KB to bytes
#elif defined(_WIN32)
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        return 0; // Not implemented for this platform
#endif
    }
    
    static size_t getPeakMemoryUsage() {
#ifdef __linux__
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss * 1024;
#elif defined(_WIN32)
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.PeakWorkingSetSize;
        }
        return 0;
#else
        return 0;
#endif
    }
    
    static void resetPeakMemory() {
#ifdef __linux__
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        // Linux doesn't provide a way to reset peak memory
#elif defined(_WIN32)
        // Windows doesn't provide a way to reset peak memory
#endif
    }
};

// Benchmark packet memory usage
static void BM_PacketMemoryUsage(benchmark::State& state) {
    const int num_packets = state.range(0);
    
    for (auto _ : state) {
        std::vector<DataPlane::Packet> packets;
        packets.reserve(num_packets);
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        // Create packets
        for (int i = 0; i < num_packets; ++i) {
            packets.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1." + std::to_string(100 + i),
                "10.1.1." + std::to_string(1 + i),
                i % 24 + 1
            );
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(packets);
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,PerPacket=" + std::to_string(memory_used / num_packets) + "B");
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark L2 FDB memory usage
static void BM_L2FDBMemoryUsage(benchmark::State& state) {
    const int num_entries = state.range(0);
    
    for (auto _ : state) {
        L2::L2Service l2_service(*new Sai::SimulatedSai());
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        // Add MAC entries
        for (int i = 0; i < num_entries; ++i) {
            char mac[18];
            snprintf(mac, sizeof(mac), "aa:bb:cc:dd:%02x:%02x", (i >> 8) & 0xff, i & 0xff);
            DataPlane::Packet learn_pkt(
                mac, "ff:ff:ff:ff:ff:ff", "0.0.0.0", "0.0.0.0", i % 24 + 1
            );
            // Use public handle method
            l2_service.handle(learn_pkt);
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(l2_service);
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,PerEntry=" + std::to_string(memory_used / num_entries) + "B");
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark L3 routing table memory usage
static void BM_L3RoutingMemoryUsage(benchmark::State& state) {
    const int num_routes = state.range(0);
    
    for (auto _ : state) {
        L3::L3Service l3_service(*new Sai::SimulatedSai());
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        // Add routes
        for (int i = 0; i < num_routes; ++i) {
            char network[32];
            char next_hop[32];
            snprintf(network, sizeof(network), "10.%d.%d.0", i / 256, i % 256);
            snprintf(next_hop, sizeof(next_hop), "10.%d.%d.1", i / 256, i % 256);
            l3_service.addRoute(network, 24, next_hop);
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(l3_service);
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,PerRoute=" + std::to_string(memory_used / num_routes) + "B");
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark LPM trie memory usage
static void BM_LpmTrieMemoryUsage(benchmark::State& state) {
    const int num_routes = state.range(0);
    
    for (auto _ : state) {
        L3::LpmTrie trie;
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        // Add routes
        for (int i = 0; i < num_routes; ++i) {
            char network[32];
            char next_hop[32];
            snprintf(network, sizeof(network), "10.%d.%d.0", i / 256, i % 256);
            snprintf(next_hop, sizeof(next_hop), "10.%d.%d.1", i / 256, i % 256);
            trie.insert(network, 24, next_hop);
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(trie);
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,PerRoute=" + std::to_string(memory_used / num_routes) + "B");
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Benchmark queue memory usage
static void BM_QueueMemoryUsage(benchmark::State& state) {
    const int queue_size = state.range(0);
    
    for (auto _ : state) {
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        Utils::SPSCQueue<DataPlane::Packet> queue(queue_size);
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        // Prevent optimization by using a side effect
        benchmark::DoNotOptimize(queue.capacity());
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,PerSlot=" + std::to_string(memory_used / queue_size) + "B");
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark memory efficiency with different packet sizes
static void BM_PacketSizeMemoryEfficiency(benchmark::State& state) {
    const int packet_size = state.range(0);
    
    for (auto _ : state) {
        std::vector<uint8_t> packet_data(packet_size, 0xAA);
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        std::vector<DataPlane::Packet> packets;
        packets.reserve(1000);
        
        for (int i = 0; i < 1000; ++i) {
            packets.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1.100",
                "10.1.1.42",
                1
            );
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(packets);
        
        double efficiency = (1000.0 * sizeof(DataPlane::Packet)) / memory_used * 100.0;
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,Efficiency=" + std::to_string(static_cast<int>(efficiency)) + "%");
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Benchmark memory fragmentation
static void BM_MemoryFragmentation(benchmark::State& state) {
    const int num_allocations = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::unique_ptr<DataPlane::Packet>> packets;
        
        auto start_memory = MemoryProfiler::getCurrentMemoryUsage();
        
        // Allocate and deallocate packets to cause fragmentation
        for (int i = 0; i < num_allocations; ++i) {
            packets.push_back(std::make_unique<DataPlane::Packet>(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1.100",
                "10.1.1.42",
                1
            ));
        }
        
        // Random deallocation to cause fragmentation
        for (int i = 0; i < num_allocations / 2; ++i) {
            if (static_cast<size_t>(i * 2) < packets.size()) {
                packets[i * 2].reset();
            }
        }
        
        auto end_memory = MemoryProfiler::getCurrentMemoryUsage();
        size_t memory_used = end_memory - start_memory;
        
        benchmark::DoNotOptimize(packets);
        
        state.SetLabel("Memory=" + std::to_string(memory_used / 1024) + "KB,Fragments=" + std::to_string(num_allocations / 2));
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Register memory benchmarks
void RegisterMemoryBenchmarks() {
    BENCHMARK(BM_PacketMemoryUsage)
        ->Name("PacketMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_L2FDBMemoryUsage)
        ->Name("L2FDBMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_L3RoutingMemoryUsage)
        ->Name("L3RoutingMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_LpmTrieMemoryUsage)
        ->Name("LpmTrieMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_QueueMemoryUsage)
        ->Name("QueueMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_PacketSizeMemoryEfficiency)
        ->Name("PacketSizeMemoryEfficiency")
        ->Arg(64)
        ->Arg(128)
        ->Arg(256)
        ->Arg(512)
        ->Arg(1024)
        ->Arg(1500)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_MemoryFragmentation)
        ->Name("MemoryFragmentation")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
}
