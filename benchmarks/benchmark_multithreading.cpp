#include <benchmark/benchmark.h>
#include "dataplane/packet.h"
#include "dataplane/pipeline.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "sai/simulated_sai.h"
#include "utils/spsc_queue.hpp"
#include "utils/metrics.hpp"
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <barrier>

using namespace MiniSonic;

/**
 * @file benchmark_multithreading.cpp
 * @brief Benchmarks for multi-threaded performance
 * 
 * This file contains comprehensive benchmarks for measuring performance
 * of multi-threaded scenarios including concurrent packet processing,
 * queue operations, and scalability analysis.
 */

// Global test setup
static Sai::SimulatedSai* g_sai = nullptr;
static DataPlane::Pipeline* g_pipeline = nullptr;

static void SetUpMultithreadingTests() {
    if (!g_sai) {
        g_sai = new Sai::SimulatedSai();
        g_pipeline = new DataPlane::Pipeline(*g_sai);
        
        // Add test routes
        g_pipeline->getL3().addRoute("10.0.0.0", 8, "1.1.1.1");
        g_pipeline->getL3().addRoute("10.1.0.0", 16, "2.2.2.2");
        g_pipeline->getL3().addRoute("10.1.1.0", 24, "3.3.3.3");
        g_pipeline->getL3().addRoute("0.0.0.0", 0, "192.168.1.1");
    }
}

static void TearDownMultithreadingTests() {
    delete g_pipeline;
    delete g_sai;
    g_pipeline = nullptr;
    g_sai = nullptr;
}

// Benchmark concurrent packet processing
static void BM_ConcurrentPacketProcessing(benchmark::State& state) {
    SetUpMultithreadingTests();
    
    const int num_threads = state.range(0);
    const int packets_per_thread = state.range(1);
    
    std::vector<std::thread> threads;
    std::atomic<int64_t> total_processed{0};
    std::barrier sync_point(num_threads);
    
    auto worker_func = [&](int thread_id) {
        std::vector<DataPlane::Packet> packets;
        packets.reserve(packets_per_thread);
        
        // Create packets for this thread
        for (int i = 0; i < packets_per_thread; ++i) {
            packets.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1." + std::to_string(100 + thread_id * 100 + i),
                "10.1.1." + std::to_string(1 + thread_id * 100 + i),
                (thread_id % 24) + 1
            );
        }
        
        // Synchronize start
        sync_point.arrive_and_wait();
        
        // Process packets
        for (const auto& pkt : packets) {
            g_pipeline->process(pkt);
            total_processed.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        total_processed.store(0, std::memory_order_relaxed);
        
        // Create worker threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker_func, i);
        }
        
        // Wait for completion
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(total_processed.load());
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    double efficiency = (pps / num_threads) / (pps / 1.0) * 100.0; // Relative to single thread
    
    state.SetItemsProcessed(total_processed.load());
    state.SetBytesProcessed(total_processed.load() * sizeof(DataPlane::Packet));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)) + ",Efficiency=" + std::to_string(static_cast<int>(efficiency)) + "%");
    
    TearDownMultithreadingTests();
}

// Benchmark concurrent queue operations
static void BM_ConcurrentQueueOperations(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int operations_per_thread = state.range(1);
    
    Utils::SPSCQueue<DataPlane::Packet> queue(1024);
    std::atomic<int64_t> total_pushed{0};
    std::atomic<int64_t> total_popped{0};
    
    auto producer_func = [&](int thread_id) {
        std::vector<DataPlane::Packet> packets;
        packets.reserve(operations_per_thread);
        
        // Create packets
        for (int i = 0; i < operations_per_thread; ++i) {
            packets.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1.100",
                "10.1.1.42",
                1
            );
        }
        
        // Push packets
        for (const auto& pkt : packets) {
            while (!queue.push(pkt)) {
                std::this_thread::yield();
            }
            total_pushed.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto consumer_func = [&](int thread_id) {
        DataPlane::Packet pkt;
        int popped = 0;
        
        while (popped < operations_per_thread) {
            if (queue.pop(pkt)) {
                popped++;
                total_popped.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    std::vector<std::thread> threads;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        total_pushed.store(0, std::memory_order_relaxed);
        total_popped.store(0, std::memory_order_relaxed);
        
        // Create producer threads
        for (int i = 0; i < num_threads / 2; ++i) {
            threads.emplace_back(producer_func, i);
        }
        
        // Create consumer threads
        for (int i = 0; i < num_threads / 2; ++i) {
            threads.emplace_back(consumer_func, i);
        }
        
        // Wait for completion
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_ops = static_cast<double>(total_pushed.load() + total_popped.load());
    double total_seconds = duration.count() / 1e6;
    double ops_per_sec = total_ops / total_seconds;
    
    state.SetItemsProcessed(total_pushed.load() + total_popped.load());
    state.SetLabel("OPS=" + std::to_string(static_cast<int64_t>(ops_per_sec)));
}

// Benchmark scalability analysis
static void BM_ScalabilityAnalysis(benchmark::State& state) {
    SetUpMultithreadingTests();
    
    const int num_threads = state.range(0);
    const int packets_per_thread = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int64_t> total_processed{0};
    
    auto worker_func = [&](int thread_id) {
        std::vector<DataPlane::Packet> packets;
        packets.reserve(packets_per_thread);
        
        // Create packets for this thread
        for (int i = 0; i < packets_per_thread; ++i) {
            packets.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1." + std::to_string(100 + thread_id * 1000 + i),
                "10.1.1." + std::to_string(1 + thread_id * 1000 + i),
                (thread_id % 24) + 1
            );
        }
        
        // Process packets
        for (const auto& pkt : packets) {
            g_pipeline->process(pkt);
            total_processed.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        total_processed.store(0, std::memory_order_relaxed);
        
        // Create worker threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker_func, i);
        }
        
        // Wait for completion
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(total_processed.load());
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    double scalability = pps / num_threads; // PPS per thread
    
    state.SetItemsProcessed(total_processed.load());
    state.SetBytesProcessed(total_processed.load() * sizeof(DataPlane::Packet));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)) + ",PerThread=" + std::to_string(static_cast<int64_t>(scalability)));
    
    TearDownMultithreadingTests();
}

// Benchmark thread contention
static void BM_ThreadContention(benchmark::State& state) {
    SetUpMultithreadingTests();
    
    const int num_threads = state.range(0);
    const int operations_per_thread = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int64_t> total_processed{0};
    
    auto worker_func = [&](int thread_id) {
        // Use the same pipeline to create contention
        for (int i = 0; i < operations_per_thread; ++i) {
            DataPlane::Packet pkt(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1." + std::to_string(100 + i),
                "10.1.1." + std::to_string(1 + i),
                1
            );
            
            g_pipeline->process(pkt);
            total_processed.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        total_processed.store(0, std::memory_order_relaxed);
        
        // Create worker threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker_func, i);
        }
        
        // Wait for completion
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(total_processed.load());
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    
    state.SetItemsProcessed(total_processed.load());
    state.SetBytesProcessed(total_processed.load() * sizeof(DataPlane::Packet));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)));
    
    TearDownMultithreadingTests();
}

// Register multi-threading benchmarks
void RegisterMultithreadingBenchmarks() {
    BENCHMARK(BM_ConcurrentPacketProcessing)
        ->Name("ConcurrentPacketProcessing")
        ->Args({1, 1000})
        ->Args({2, 1000})
        ->Args({4, 1000})
        ->Args({8, 1000})
        ->Args({16, 1000})
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_ConcurrentQueueOperations)
        ->Name("ConcurrentQueueOperations")
        ->Args({2, 1000})
        ->Args({4, 1000})
        ->Args({8, 1000})
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_ScalabilityAnalysis)
        ->Name("ScalabilityAnalysis")
        ->Arg(1)
        ->Arg(2)
        ->Arg(4)
        ->Arg(8)
        ->Arg(16)
        ->Arg(32)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_ThreadContention)
        ->Name("ThreadContention")
        ->Arg(1)
        ->Arg(2)
        ->Arg(4)
        ->Arg(8)
        ->Arg(16)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
}
