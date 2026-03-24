#include <benchmark/benchmark.h>
#include "dataplane/packet.h"
#include "dataplane/pipeline.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "l3/lpm_trie.h"
#include "sai/simulated_sai.h"
#include "utils/metrics.hpp"
#include <chrono>
#include <vector>
#include <algorithm>

using namespace MiniSonic;

/**
 * @file benchmark_latency.cpp
 * @brief Benchmarks for latency measurement
 * 
 * This file contains comprehensive benchmarks for measuring latency
 * of various components including packet processing, queue operations,
 * and end-to-end latency measurements.
 */

// Global test setup
static Sai::SimulatedSai* g_sai = nullptr;
static DataPlane::Pipeline* g_pipeline = nullptr;
static Utils::SPSCQueue<DataPlane::Packet>* g_queue = nullptr;

static void SetUpLatencyTests() {
    if (!g_sai) {
        g_sai = new Sai::SimulatedSai();
        g_pipeline = new DataPlane::Pipeline(*g_sai);
        g_queue = new Utils::SPSCQueue<DataPlane::Packet>(1024);
        
        // Add test routes
        g_pipeline->getL3().addRoute("10.0.0.0", 8, "1.1.1.1");
        g_pipeline->getL3().addRoute("10.1.0.0", 16, "2.2.2.2");
        g_pipeline->getL3().addRoute("10.1.1.0", 24, "3.3.3.3");
        g_pipeline->getL3().addRoute("0.0.0.0", 0, "192.168.1.1");
    }
}

static void TearDownLatencyTests() {
    delete g_queue;
    delete g_pipeline;
    delete g_sai;
    g_queue = nullptr;
    g_pipeline = nullptr;
    g_sai = nullptr;
}

// Benchmark SPSC Queue latency
static void BM_QueuePushLatency(benchmark::State& state) {
    SetUpLatencyTests();
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:ee:ff",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        bool result = g_queue->push(pkt);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        state.SetIterationTime(latency / 1e9); // Convert to seconds
        
        // Pop to prevent queue from filling
        DataPlane::Packet dummy;
        g_queue->pop(dummy);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    TearDownLatencyTests();
}

// Benchmark SPSC Queue pop latency
static void BM_QueuePopLatency(benchmark::State& state) {
    SetUpLatencyTests();
    
    // Pre-populate queue
    std::vector<DataPlane::Packet> packets;
    for (int i = 0; i < 100; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        g_queue->push(packets.back());
    }
    
    DataPlane::Packet pkt;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        bool result = g_queue->pop(pkt);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        state.SetIterationTime(latency / 1e9);
        
        // Re-push to keep queue populated
        if (result) {
            g_queue->push(pkt);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    
    TearDownLatencyTests();
}

// Benchmark end-to-end latency with timestamp
static void BM_EndToEndLatency(benchmark::State& state) {
    SetUpLatencyTests();
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:ee:ff",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    std::vector<double> latencies;
    latencies.reserve(state.iterations());
    
    for (auto _ : state) {
        auto start_time = std::chrono::high_resolution_clock::now();
        pkt.updateTimestamp();
        
        // Simulate pipeline processing
        g_pipeline->process(pkt);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time
        ).count();
        
        latencies.push_back(latency);
    }
    
    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();
    double p50 = latencies[latencies.size() * 0.5];
    double p95 = latencies[latencies.size() * 0.95];
    double p99 = latencies[latencies.size() * 0.99];
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Mean=" + std::to_string(mean) + "ns,P50=" + std::to_string(p50) + "ns,P95=" + std::to_string(p95) + "ns,P99=" + std::to_string(p99) + "ns");
    
    TearDownLatencyTests();
}

// Benchmark L2 forwarding latency
static void BM_L2ForwardingLatency(benchmark::State& state) {
    SetUpLatencyTests();
    
    // Pre-populate FDB
    auto& l2_service = g_pipeline->getL2();
    for (int i = 0; i < 1000; ++i) {
        char mac[18];
        snprintf(mac, sizeof(mac), "aa:bb:cc:dd:%02x:%02x", i >> 8, i & 0xff);
        l2_service.learn(mac, i % 24 + 1);
    }
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:01:02",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    std::vector<double> latencies;
    latencies.reserve(state.iterations());
    
    for (auto _ : state) {
        auto start_time = std::chrono::high_resolution_clock::now();
        bool result = l2_service.handle(pkt);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time
        ).count();
        
        latencies.push_back(latency);
        benchmark::DoNotOptimize(result);
    }
    
    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    double p95 = latencies[latencies.size() * 0.95];
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Mean=" + std::to_string(mean) + "ns,P95=" + std::to_string(p95) + "ns");
    
    TearDownLatencyTests();
}

// Benchmark L3 routing latency
static void BM_L3RoutingLatency(benchmark::State& state) {
    SetUpLatencyTests();
    
    // Pre-populate routing table
    auto& l3_service = g_pipeline->getL3();
    for (int i = 0; i < 100; ++i) {
        char network[16];
        char next_hop[16];
        snprintf(network, sizeof(network), "10.%d.%d.0", i / 256, i % 256);
        snprintf(next_hop, sizeof(next_hop), "10.%d.%d.1", i / 256, i % 256);
        l3_service.addRoute(network, 24, next_hop);
    }
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:ee:ff",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    std::vector<double> latencies;
    latencies.reserve(state.iterations());
    
    for (auto _ : state) {
        auto start_time = std::chrono::high_resolution_clock::now();
        bool result = l3_service.handle(pkt);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time
        ).count();
        
        latencies.push_back(latency);
        benchmark::DoNotOptimize(result);
    }
    
    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    double p95 = latencies[latencies.size() * 0.95];
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Mean=" + std::to_string(mean) + "ns,P95=" + std::to_string(p95) + "ns");
    
    TearDownLatencyTests();
}

// Register latency benchmarks
void RegisterLatencyBenchmarks() {
    BENCHMARK(BM_QueuePushLatency)
        ->Name("QueuePushLatency")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_QueuePopLatency)
        ->Name("QueuePopLatency")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_EndToEndLatency)
        ->Name("EndToEndLatency")
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0)
        ->Repetitions(5);
    
    BENCHMARK(BM_L2ForwardingLatency)
        ->Name("L2ForwardingLatency")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_L3RoutingLatency)
        ->Name("L3RoutingLatency")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
}
