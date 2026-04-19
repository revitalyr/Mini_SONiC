#include <benchmark/benchmark.h>
#include "dataplane/packet.h"
#include "dataplane/pipeline.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "l3/lpm_trie.h"
#include "sai/simulated_sai.h"
#include "utils/metrics.hpp"

using namespace MiniSonic;

/**
 * @file benchmark_packet_processing.cpp
 * @brief Benchmarks for packet processing performance
 * 
 * This file contains comprehensive benchmarks for measuring the performance
 * of packet processing components including L2 switching, L3 routing,
 * and end-to-end pipeline processing.
 */

// Global test setup
static Sai::SimulatedSai* g_sai = nullptr;
static DataPlane::Pipeline* g_pipeline = nullptr;

static void SetUpPacketProcessing() {
    if (!g_sai) {
        g_sai = new Sai::SimulatedSai();
        g_pipeline = new DataPlane::Pipeline(*g_sai);
        
        // Add test routes
        g_pipeline->getL3().addRoute("10.0.0.0", 8, "1.1.1.1");
        g_pipeline->getL3().addRoute("10.1.0.0", 16, "2.2.2.2");
        g_pipeline->getL3().addRoute("10.1.1.0", 24, "3.3.3.3");
        g_pipeline->getL3().addRoute("0.0.0.0", 0, "192.168.1.1"); // Default route
    }
}

static void TearDownPacketProcessing() {
    delete g_pipeline;
    delete g_sai;
    g_pipeline = nullptr;
    g_sai = nullptr;
}

// Benchmark packet creation
static void BM_PacketCreation(benchmark::State& state) {
    SetUpPacketProcessing();
    
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
    
    TearDownPacketProcessing();
}

// Benchmark packet copying
static void BM_PacketCopy(benchmark::State& state) {
    SetUpPacketProcessing();
    
    DataPlane::Packet original(
        "aa:bb:cc:dd:ee:ff",
        "ff:ee:dd:cc:bb:aa",
        "10.1.1.100",
        "10.1.1.42",
        1
    );
    
    for (auto _ : state) {
        DataPlane::Packet copy = original;
        benchmark::DoNotOptimize(copy);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(DataPlane::Packet));
    
    TearDownPacketProcessing();
}

// Benchmark L2 MAC lookup
static void BM_L2MacLookup(benchmark::State& state) {
    SetUpPacketProcessing();
    
    // Pre-populate FDB
    auto& l2_service = g_pipeline->getL2();
    for (int i = 0; i < 1000; ++i) {
        char mac[18];
        snprintf(mac, sizeof(mac), "aa:bb:cc:dd:%02x:%02x", i >> 8, i & 0xff);
        DataPlane::Packet learn_pkt(
            mac, "ff:ff:ff:ff:ff:ff", "0.0.0.0", "0.0.0.0", i % 24 + 1
        );
        // Use public handle method to learn MAC, as learn() is private
        l2_service.handle(learn_pkt);
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
    
    TearDownPacketProcessing();
}

// Benchmark L3 route lookup
static void BM_L3RouteLookup(benchmark::State& state) {
    SetUpPacketProcessing();
    
    L3::LpmTrie trie;
    
    // Add routes with different prefix lengths
    trie.insert("10.0.0.0", 8, "1.1.1.1");
    trie.insert("10.1.0.0", 16, "2.2.2.2");
    trie.insert("10.1.1.0", 24, "3.3.3.3");
    trie.insert("192.168.0.0", 16, "192.168.1.1");
    trie.insert("172.16.0.0", 12, "172.16.1.1");
    trie.insert("0.0.0.0", 0, "0.0.0.0"); // Default route
    
    const std::string test_ip = "10.1.1.42";
    
    for (auto _ : state) {
        auto result = trie.lookup(test_ip).value_or("");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark full pipeline processing
static void BM_PipelineProcessing(benchmark::State& state) {
    SetUpPacketProcessing();
    
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
    
    TearDownPacketProcessing();
}

// Benchmark batch processing
static void BM_BatchProcessing(benchmark::State& state) {
    SetUpPacketProcessing();
    
    const int batch_size = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(batch_size);
    
    // Create batch of packets
    for (int i = 0; i < batch_size; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1." + std::to_string(100 + i),
            "10.1.1.42",
            i % 24 + 1
        );
    }
    
    for (auto _ : state) {
        for (auto pkt : packets) {
            g_pipeline->process(pkt);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch_size * sizeof(DataPlane::Packet));
    
    TearDownPacketProcessing();
}

// Register benchmarks
void RegisterPacketProcessingBenchmarks() {
    BENCHMARK(BM_PacketCreation)
        ->Name("PacketCreation")
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_PacketCopy)
        ->Name("PacketCopy")
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_L2MacLookup)
        ->Name("L2MacLookup")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_L3RouteLookup)
        ->Name("L3RouteLookup")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_PipelineProcessing)
        ->Name("PipelineProcessing")
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_BatchProcessing)
        ->Name("BatchProcessing")
        ->Arg(1)
        ->Arg(8)
        ->Arg(16)
        ->Arg(32)
        ->Arg(64)
        ->Arg(128)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
}
