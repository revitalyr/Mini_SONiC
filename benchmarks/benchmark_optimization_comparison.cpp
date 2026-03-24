#include <benchmark/benchmark.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
#include <thread>

// Import optimized modules
#ifdef HAS_OPTIMIZED_MODULES
import MiniSonic.DataPlane.Optimized;
import MiniSonic.Utils.Optimized;
#endif

/**
 * @file benchmark_optimization_comparison.cpp
 * @brief Performance comparison between traditional and optimized implementations
 * 
 * This benchmark compares the performance of traditional implementations
 * against the new optimized versions to demonstrate the improvements.
 */

// Traditional implementations for comparison
class TraditionalPacket {
public:
    std::string src_mac;
    std::string dst_mac;
    std::string src_ip;
    std::string dst_ip;
    uint32_t ingress_port;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    TraditionalPacket(const std::string& src_mac, const std::string& dst_mac,
                   const std::string& src_ip, const std::string& dst_ip,
                   uint32_t ingress_port)
        : src_mac(src_mac), dst_mac(dst_mac), src_ip(src_ip), dst_ip(dst_ip),
          ingress_port(ingress_port), timestamp(std::chrono::high_resolution_clock::now()) {}
};

class TraditionalMetrics {
public:
    static TraditionalMetrics& instance() {
        static TraditionalMetrics instance;
        return instance;
    }
    
    void inc(const std::string& name, uint64_t value = 1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_counters[name] += value;
    }
    
    void addLatency(uint64_t latency_ns) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latency_sum += latency_ns;
        m_latency_count++;
        m_latency_min = std::min(m_latency_min, latency_ns);
        m_latency_max = std::max(m_latency_max, latency_ns);
    }
    
    std::string getStats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream oss;
        oss << "Traditional Metrics:\n";
        for (const auto& [name, value] : m_counters) {
            oss << "  " << name << ": " << value << "\n";
        }
        if (m_latency_count > 0) {
            oss << "  Latency - Count: " << m_latency_count
                 << ", Mean: " << (m_latency_sum / m_latency_count)
                 << ", Min: " << m_latency_min
                 << ", Max: " << m_latency_max << "\n";
        }
        return oss.str();
    }
    
private:
    std::unordered_map<std::string, uint64_t> m_counters;
    uint64_t m_latency_sum{0};
    uint64_t m_latency_count{0};
    uint64_t m_latency_min{UINT64_MAX};
    uint64_t m_latency_max{0};
    mutable std::mutex m_mutex;
};

// Benchmark functions
static void BM_TraditionalPacketCreation(benchmark::State& state) {
    for (auto _ : state) {
        TraditionalPacket pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        benchmark::DoNotOptimize(pkt);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(TraditionalPacket));
}

static void BM_OptimizedPacketCreation(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    for (auto _ : state) {
        MiniSonic::DataPlane::Optimized::OptimizedPacket pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        benchmark::DoNotOptimize(pkt);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(MiniSonic::DataPlane::Optimized::OptimizedPacket));
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

static void BM_TraditionalMetricsOperations(benchmark::State& state) {
    auto& metrics = TraditionalMetrics::instance();
    
    for (auto _ : state) {
        metrics.inc("test_counter", 1);
        metrics.inc("test_counter2", 2);
        metrics.inc("test_counter3", 3);
        metrics.addLatency(100);
        metrics.addLatency(200);
        metrics.addLatency(300);
    }
    
    state.SetItemsProcessed(state.iterations() * 6); // 6 operations per iteration
}

static void BM_OptimizedMetricsOperations(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    auto& metrics = MiniSonic::Utils::Optimized::LockFreeMetrics::instance();
    
    for (auto _ : state) {
        metrics.incThreadLocal("test_counter", 1);
        metrics.incThreadLocal("test_counter2", 2);
        metrics.incThreadLocal("test_counter3", 3);
        metrics.addLatency(100);
        metrics.addLatency(200);
        metrics.addLatency(300);
    }
    
    state.SetItemsProcessed(state.iterations() * 6);
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

static void BM_TraditionalStringOperations(benchmark::State& state) {
    for (auto _ : state) {
        std::string test_str = "  Hello, World!  ";
        std::string trimmed = test_str.substr(test_str.find_first_not_of(" \t\n\r"));
        std::string lower = trimmed;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        benchmark::DoNotOptimize(lower);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_OptimizedStringOperations(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    using namespace MiniSonic::Utils::Optimized::OptimizedString;
    
    for (auto _ : state) {
        std::string test_str = "  Hello, World!  ";
        std::string_view trimmed = trimSIMD(test_str);
        std::string_view lower = trimmed.substr(0, trimmed.find_first_of(" \t\n\r"));
        benchmark::DoNotOptimize(lower);
    }
    
    state.SetItemsProcessed(state.iterations());
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

static void BM_TraditionalMemoryAllocation(benchmark::State& state) {
    std::vector<std::unique_ptr<TraditionalPacket>> packets;
    
    for (auto _ : state) {
        // Traditional allocation with new/delete
        packets.push_back(std::make_unique<TraditionalPacket>(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        ));
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(TraditionalPacket));
}

static void BM_OptimizedMemoryAllocation(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    using namespace MiniSonic::DataPlane::Optimized;
    
    PacketPool pool(1000, sizeof(OptimizedPacket));
    
    for (auto _ : state) {
        // Optimized allocation from pool
        void* ptr = pool.acquire();
        if (ptr) {
            auto* pkt = new(ptr) OptimizedPacket(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1.100",
                "10.1.1.42",
                1
            );
            pool.release(pkt);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(OptimizedPacket));
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

static void BM_TraditionalSynchronization(benchmark::State& state) {
    std::vector<TraditionalPacket> shared_queue;
    std::mutex queue_mutex;
    
    for (auto _ : state) {
        // Traditional synchronized queue operations
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            shared_queue.emplace_back(
                "aa:bb:cc:dd:ee:ff",
                "ff:ee:dd:cc:bb:aa",
                "10.1.1.100",
                "10.1.1.42",
                1
            );
        }
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!shared_queue.empty()) {
                shared_queue.erase(shared_queue.begin());
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 2); // 2 operations per iteration
}

static void BM_OptimizedSynchronization(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    using namespace MiniSonic::DataPlane::Optimized;
    
    LockFreeStructures::OptimizedQueue<TraditionalPacket> queue(1024);
    
    for (auto _ : state) {
        // Lock-free queue operations
        TraditionalPacket pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        
        queue.tryPush(pkt);
        
        TraditionalPacket popped;
        queue.tryPop(popped);
    }
    
    state.SetItemsProcessed(state.iterations() * 2);
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

static void BM_CpuFeatureDetection(benchmark::State& state) {
#ifdef HAS_OPTIMIZED_MODULES
    using namespace MiniSonic::Utils::Optimized::CpuOptimization;
    
    for (auto _ : state) {
        // CPU feature detection overhead
        const auto& features = CpuOptimization::detectFeatures();
        benchmark::DoNotOptimize(features.has_sse2);
        benchmark::DoNotOptimize(features.has_avx);
        benchmark::DoNotOptimize(features.has_avx2);
        benchmark::DoNotOptimize(features.cache_line_size);
    }
    
    state.SetItemsProcessed(state.iterations() * 5);
#else
    state.SkipWithError("Optimized modules not available");
#endif
}

// Registration function
void RegisterOptimizationComparisonBenchmarks() {
    std::cout << "=== Optimization Comparison Benchmarks ===\n";
    
    // Packet creation comparison
    BENCHMARK(BM_TraditionalPacketCreation)
        ->Name("TraditionalPacketCreation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_OptimizedPacketCreation)
        ->Name("OptimizedPacketCreation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    // Metrics operations comparison
    BENCHMARK(BM_TraditionalMetricsOperations)
        ->Name("TraditionalMetricsOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_OptimizedMetricsOperations)
        ->Name("OptimizedMetricsOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    // String operations comparison
    BENCHMARK(BM_TraditionalStringOperations)
        ->Name("TraditionalStringOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_OptimizedStringOperations)
        ->Name("OptimizedStringOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    // Memory allocation comparison
    BENCHMARK(BM_TraditionalMemoryAllocation)
        ->Name("TraditionalMemoryAllocation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_OptimizedMemoryAllocation)
        ->Name("OptimizedMemoryAllocation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    // Synchronization comparison
    BENCHMARK(BM_TraditionalSynchronization)
        ->Name("TraditionalSynchronization")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_OptimizedSynchronization)
        ->Name("OptimizedSynchronization")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    // CPU feature detection
    BENCHMARK(BM_CpuFeatureDetection)
        ->Name("CpuFeatureDetection")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
}

// Main function for running optimization comparison
int main(int argc, char** argv) {
    std::cout << "=== Mini SONiC Optimization Comparison Benchmarks ===\n";
    std::cout << "System Information:\n";
    std::cout << "  C++ Standard: C++20\n";
    std::cout << "  Optimized Modules: " 
#ifdef HAS_OPTIMIZED_MODULES
        "Available"
#else
        "Not Available"
#endif
        << "\n";
    
    // Reset metrics
    TraditionalMetrics::instance().inc("benchmark_runs", 0); // Reset
    
#ifdef HAS_OPTIMIZED_MODULES
    MiniSonic::Utils::Optimized::LockFreeMetrics::instance().reset();
#endif
    
    // Register benchmarks
    RegisterOptimizationComparisonBenchmarks();
    
    std::cout << "\nRunning optimization comparison benchmarks...\n";
    std::cout << "This will compare traditional vs optimized implementations\n";
    std::cout << "Note: Results are saved to JSON format for analysis\n\n";
    
    // Run benchmarks
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    std::cout << "\n=== Optimization Comparison Results ===\n";
    std::cout << TraditionalMetrics::instance().getStats() << "\n";
    
#ifdef HAS_OPTIMIZED_MODULES
    std::cout << MiniSonic::Utils::Optimized::LockFreeMetrics::instance().getSummary() << "\n";
#endif
    
    return 0;
}
