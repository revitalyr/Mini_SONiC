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
#include <thread>

using namespace MiniSonic;

/**
 * @file benchmark_basic.cpp
 * @brief Basic benchmarks without Boost dependencies
 * 
 * This file contains essential benchmarks that work without Boost
 * for fundamental performance measurements.
 */

// Global test setup
static Sai::SimulatedSai* g_sai = nullptr;
static DataPlane::Pipeline* g_pipeline = nullptr;

static void SetUpBasicBenchmarks() {
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

static void TearDownBasicBenchmarks() {
    delete g_pipeline;
    delete g_sai;
    g_pipeline = nullptr;
    g_sai = nullptr;
}

// Basic packet creation benchmark
static void BM_BasicPacketCreation(benchmark::State& state) {
    SetUpBasicBenchmarks();
    
    for (auto _ : state) {
        DataPlane::Packet pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        benchmark::DoNotOptimize(pkt);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(DataPlane::Packet));
    
    TearDownBasicBenchmarks();
}

// Basic packet processing benchmark
static void BM_BasicPipelineProcessing(benchmark::State& state) {
    SetUpBasicBenchmarks();
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:ee:ff",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    for (auto _ : state) {
        g_pipeline->process(pkt);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(DataPlane::Packet));
    
    TearDownBasicBenchmarks();
}

// L2 MAC lookup benchmark
static void BM_BasicL2Lookup(benchmark::State& state) {
    SetUpBasicBenchmarks();
    
    // Pre-populate FDB
    auto& l2_service = g_pipeline->getL2();
    for (int i = 0; i < 1000; ++i) {
        char mac[18];
        snprintf(mac, sizeof(mac), "aa:bb:cc:dd:%02x:%02x", i >> 8, i & 0xff);
        // Use public method instead of private learn method
        DataPlane::Packet test_pkt(
            mac,
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            i % 24 + 1
        );
        l2_service.handle(test_pkt); // This will learn the MAC
    }
    
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:01:02",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    for (auto _ : state) {
        bool result = l2_service.handle(pkt);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    TearDownBasicBenchmarks();
}

// L3 route lookup benchmark
static void BM_BasicL3Lookup(benchmark::State& state) {
    SetUpBasicBenchmarks();
    
    L3::LpmTrie trie;
    
    // Add routes
    trie.insert("10.0.0.0", 8, "1.1.1.1");
    trie.insert("10.1.0.0", 16, "2.2.2.2");
    trie.insert("10.1.1.0", 24, "3.3.3.3");
    trie.insert("192.168.0.0", 16, "192.168.1.1");
    trie.insert("0.0.0.0", 0, "0.0.0.0");
    
    const std::string test_ip = "10.1.1.42";
    
    for (auto _ : state) {
        auto result = trie.lookup(test_ip).value_or("");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Memory usage benchmark
static void BM_BasicMemoryUsage(benchmark::State& state) {
    const int num_packets = state.range(0);
    
    for (auto _ : state) {
        std::vector<DataPlane::Packet> packets;
        packets.reserve(num_packets);
        
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
        
        benchmark::DoNotOptimize(packets);
    }
    
    state.SetItemsProcessed(state.iterations() * num_packets);
    state.SetBytesProcessed(state.iterations() * num_packets * sizeof(DataPlane::Packet));
}

// Throughput benchmark
static void BM_BasicThroughput(benchmark::State& state) {
    SetUpBasicBenchmarks();
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets
    for (int i = 0; i < packet_count; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1." + std::to_string(100 + i),
            "10.1.1." + std::to_string(1 + i),
            i % 24 + 1
        );
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (auto pkt : packets) {
            g_pipeline->process(pkt);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(state.iterations() * packet_count);
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    double mbps = (pps * sizeof(DataPlane::Packet) * 8) / (1024 * 1024);
    
    state.SetItemsProcessed(static_cast<int64_t>(total_packets));
    state.SetBytesProcessed(static_cast<int64_t>(total_packets * sizeof(DataPlane::Packet)));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)) + ",Mbps=" + std::to_string(static_cast<int>(mbps)));
    
    TearDownBasicBenchmarks();
}

// Register basic benchmarks
void RegisterBasicBenchmarks() {
    BENCHMARK(BM_BasicPacketCreation)
        ->Name("BasicPacketCreation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BasicPipelineProcessing)
        ->Name("BasicPipelineProcessing")
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BasicL2Lookup)
        ->Name("BasicL2Lookup")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BasicL3Lookup)
        ->Name("BasicL3Lookup")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BasicMemoryUsage)
        ->Name("BasicMemoryUsage")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BasicThroughput)
        ->Name("BasicThroughput")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
}
