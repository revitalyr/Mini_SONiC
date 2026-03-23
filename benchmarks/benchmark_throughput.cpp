#include <benchmark/benchmark.h>
#include "dataplane/packet.h"
#include "dataplane/pipeline.h"
#include "dataplane/pipeline_thread.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "sai/simulated_sai.h"
#include "utils/spsc_queue.hpp"
#include "utils/metrics.hpp"
#include <chrono>
#include <vector>
#include <thread>

using namespace MiniSonic;

/**
 * @file benchmark_throughput.cpp
 * @brief Benchmarks for throughput measurement
 * 
 * This file contains comprehensive benchmarks for measuring throughput
 * of various components including packet processing, queue operations,
 * and end-to-end throughput measurements.
 */

// Global test setup
static Sai::SimulatedSai* g_sai = nullptr;
static DataPlane::Pipeline* g_pipeline = nullptr;
static Utils::SPSCQueue<DataPlane::Packet>* g_queue = nullptr;
static DataPlane::PipelineThread* g_pipeline_thread = nullptr;

static void SetUpThroughputTests() {
    if (!g_sai) {
        g_sai = new Sai::SimulatedSai();
        g_pipeline = new DataPlane::Pipeline(*g_sai);
        g_queue = new Utils::SPSCQueue<DataPlane::Packet>(1024);
        g_pipeline_thread = new DataPlane::PipelineThread(*g_pipeline, *g_queue, 32);
        
        // Add test routes
        g_pipeline->getL3().addRoute("10.0.0.0", 8, "1.1.1.1");
        g_pipeline->getL3().addRoute("10.1.0.0", 16, "2.2.2.2");
        g_pipeline->getL3().addRoute("10.1.1.0", 24, "3.3.3.3");
        g_pipeline->getL3().addRoute("0.0.0.0", 0, "192.168.1.1");
        
        // Start pipeline thread
        g_pipeline_thread->start();
    }
}

static void TearDownThroughputTests() {
    delete g_pipeline_thread;
    delete g_queue;
    delete g_pipeline;
    delete g_sai;
    g_pipeline_thread = nullptr;
    g_queue = nullptr;
    g_pipeline = nullptr;
    g_sai = nullptr;
}

// Benchmark packet processing throughput
static void BM_PacketProcessingThroughput(benchmark::State& state) {
    SetUpThroughputTests();
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets
    for (int i = 0; i < packet_count; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1." + std::to_string(100 + i % 100),
            "10.1.1." + std::to_string(1 + i % 100),
            i % 24 + 1
        );
    }
    
    // Warm up
    for (int i = 0; i < 100; ++i) {
        g_pipeline->process(packets[i % packets.size()]);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (const auto& pkt : packets) {
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
    
    TearDownThroughputTests();
}

// Benchmark queue throughput
static void BM_QueueThroughput(benchmark::State& state) {
    SetUpThroughputTests();
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets
    for (int i = 0; i < packet_count; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (const auto& pkt : packets) {
            while (!g_queue->push(pkt)) {
                // Queue is full, wait a bit
                std::this_thread::yield();
            }
        }
        
        // Clear queue for next iteration
        DataPlane::Packet dummy;
        while (g_queue->pop(dummy)) {}
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(state.iterations() * packet_count);
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    
    state.SetItemsProcessed(static_cast<int64_t>(total_packets));
    state.SetBytesProcessed(static_cast<int64_t>(total_packets * sizeof(DataPlane::Packet)));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)));
    
    TearDownThroughputTests();
}

// Benchmark end-to-end throughput with pipeline thread
static void BM_EndToEndThroughput(benchmark::State& state) {
    SetUpThroughputTests();
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets
    for (int i = 0; i < packet_count; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1." + std::to_string(100 + i % 100),
            "10.1.1." + std::to_string(1 + i % 100),
            i % 24 + 1
        );
    }
    
    // Wait for pipeline thread to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (const auto& pkt : packets) {
            while (!g_queue->push(pkt)) {
                std::this_thread::yield();
            }
        }
        
        // Wait for processing to complete
        while (g_queue->size() > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
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
    
    TearDownThroughputTests();
}

// Benchmark L2 switching throughput
static void BM_L2SwitchingThroughput(benchmark::State& state) {
    SetUpThroughputTests();
    
    // Pre-populate FDB
    auto& l2_service = g_pipeline->getL2();
    for (int i = 0; i < 1000; ++i) {
        char mac[18];
        snprintf(mac, sizeof(mac), "aa:bb:cc:dd:%02x:%02x", i >> 8, i & 0xff);
        l2_service.learn(mac, i % 24 + 1);
    }
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets with known MAC addresses
    for (int i = 0; i < packet_count; ++i) {
        char src_mac[18];
        char dst_mac[18];
        snprintf(src_mac, sizeof(src_mac), "aa:bb:cc:dd:%02x:%02x", i >> 8, i & 0xff);
        snprintf(dst_mac, sizeof(dst_mac), "bb:cc:dd:ee:%02x:%02x", (i + 1) >> 8, (i + 1) & 0xff);
        
        packets.emplace_back(
            src_mac,
            dst_mac,
            "10.1.1.100",
            "10.1.1.42",
            1
        );
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (const auto& pkt : packets) {
            l2_service.handle(pkt);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(state.iterations() * packet_count);
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    
    state.SetItemsProcessed(static_cast<int64_t>(total_packets));
    state.SetBytesProcessed(static_cast<int64_t>(total_packets * sizeof(DataPlane::Packet)));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)));
    
    TearDownThroughputTests();
}

// Benchmark L3 routing throughput
static void BM_L3RoutingThroughput(benchmark::State& state) {
    SetUpThroughputTests();
    
    // Pre-populate routing table
    auto& l3_service = g_pipeline->getL3();
    for (int i = 0; i < 100; ++i) {
        char network[16];
        char next_hop[16];
        snprintf(network, sizeof(network), "10.%d.%d.0", i / 256, i % 256);
        snprintf(next_hop, sizeof(next_hop), "10.%d.%d.1", i / 256, i % 256);
        l3_service.addRoute(network, 24, next_hop);
    }
    
    const int packet_count = state.range(0);
    std::vector<DataPlane::Packet> packets;
    packets.reserve(packet_count);
    
    // Create packets with different destinations
    for (int i = 0; i < packet_count; ++i) {
        packets.emplace_back(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10." + std::to_string(i / 256) + "." + std::to_string(i % 256) + ".100",
            "10." + std::to_string((i + 1) / 256) + "." + std::to_string((i + 1) % 256) + ".100",
            1
        );
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        for (const auto& pkt : packets) {
            l3_service.handle(pkt);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    double total_packets = static_cast<double>(state.iterations() * packet_count);
    double total_seconds = duration.count() / 1e6;
    double pps = total_packets / total_seconds;
    
    state.SetItemsProcessed(static_cast<int64_t>(total_packets));
    state.SetBytesProcessed(static_cast<int64_t>(total_packets * sizeof(DataPlane::Packet)));
    state.SetLabel("PPS=" + std::to_string(static_cast<int64_t>(pps)));
    
    TearDownThroughputTests();
}

// Register throughput benchmarks
void RegisterThroughputBenchmarks() {
    BENCHMARK(BM_PacketProcessingThroughput)
        ->Name("PacketProcessingThroughput")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_QueueThroughput)
        ->Name("QueueThroughput")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_EndToEndThroughput)
        ->Name("EndToEndThroughput")
        ->Arg(100)
        ->Arg(1000)
        ->Arg(10000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(2.0)
        ->Repetitions(3);
    
    BENCHMARK(BM_L2SwitchingThroughput)
        ->Name("L2SwitchingThroughput")
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
    
    BENCHMARK(BM_L3RoutingThroughput)
        ->Name("L3RoutingThroughput")
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Unit(benchmark::kMicrosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->MinTime(1.0);
}
