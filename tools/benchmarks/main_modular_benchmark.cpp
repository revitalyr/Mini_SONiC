import MiniSonic.App;
import MiniSonic.Utils;
import <benchmark/benchmark.h>;
import <iostream>;
import <chrono>;

/**
 * @file main_modular_benchmark.cpp
 * @brief Modular benchmarks for Mini SONiC
 * 
 * This file contains benchmarks that work with the new C++20 module system.
 */

// Forward declarations for benchmark functions
void RegisterModularBenchmarks();

int main(int argc, char** argv) {
    std::cout << "=== Mini SONiC Modular Performance Benchmarks ===\n";
    std::cout << "System Information:\n";
    std::cout << "  C++ Standard: C++20\n";
    std::cout << "  Module System: Enabled\n";
    std::cout << "  Compiler: " << 
#ifdef _MSC_VER
        "MSVC " << _MSC_VER << "\n";
#elif defined(__GNUC__)
        "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "\n";
#elif defined(__clang__)
        "Clang " << __clang_major__ << "." << __clang_minor__ << "\n";
#else
        "Unknown\n";
#endif

#ifdef HAS_BOOST_ASIO
    std::cout << "  Boost.Asio: Available\n";
#else
    std::cout << "  Boost.Asio: Not available - using fallback\n";
#endif
    
    // Reset metrics before benchmarks
    MiniSonic::Utils::Metrics::instance().reset();
    
    // Register modular benchmarks
    RegisterModularBenchmarks();
    
    std::cout << "\nRunning modular benchmarks...\n";
    std::cout << "Note: Results are saved to JSON format for analysis\n\n";
    
    // Run benchmarks
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    std::cout << "\n=== Modular Benchmark Results Summary ===\n";
    std::cout << MiniSonic::Utils::Metrics::instance().getSummary() << "\n";
    
    return 0;
}

// Modular benchmark implementations
static void BM_ModularPacketCreation(benchmark::State& state) {
    for (auto _ : state) {
        MiniSonic::DataPlane::Packet pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            1
        );
        benchmark::DoNotOptimize(pkt);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(MiniSonic::DataPlane::Packet));
}

static void BM_ModularMetricsOperations(benchmark::State& state) {
    auto& metrics = MiniSonic::Utils::Metrics::instance();
    
    for (auto _ : state) {
        metrics.inc("test_counter", 1);
        metrics.setGauge("test_gauge", 42.5);
        metrics.addLatency(100);
    }
    
    state.SetItemsProcessed(state.iterations() * 3); // 3 operations per iteration
}

static void BM_ModularStringUtilities(benchmark::State& state) {
    using namespace MiniSonic::Utils::StringUtils;
    
    std::string test_str = "  Hello, World!  ";
    
    for (auto _ : state) {
        std::string trimmed = trim(test_str);
        std::string lower = toLower(trimmed);
        std::vector<std::string> parts = split(lower, ',');
        benchmark::DoNotOptimize(parts);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_ModularTimeUtilities(benchmark::State& state) {
    using namespace MiniSonic::Utils::TimeUtils;
    
    for (auto _ : state) {
        auto now = now();
        auto timestamp = timestampToMicroseconds(now);
        auto formatted = formatDuration(std::chrono::microseconds(timestamp));
        benchmark::DoNotOptimize(formatted);
    }
    
    state.SetItemsProcessed(state.iterations());
}

static void BM_ModularNetworkValidation(benchmark::State& state) {
    using namespace MiniSonic::Utils::NetworkUtils;
    
    std::string mac = "aa:bb:cc:dd:ee:ff";
    std::string ip = "192.168.1.1";
    
    for (auto _ : state) {
        bool valid_mac = isValidMacAddress(mac);
        bool valid_ip = isValidIpAddress(ip);
        std::string formatted_mac = formatMacAddress(mac);
        std::string formatted_ip = formatIpAddress(ip);
        
        benchmark::DoNotOptimize(valid_mac);
        benchmark::DoNotOptimize(valid_ip);
        benchmark::DoNotOptimize(formatted_mac);
        benchmark::DoNotOptimize(formatted_ip);
    }
    
    state.SetItemsProcessed(state.iterations() * 4);
}

static void BM_ModularQueueOperations(benchmark::State& state) {
    MiniSonic::DataPlane::SPSCQueue<MiniSonic::DataPlane::Packet> queue(1024);
    
    // Pre-populate queue
    for (int i = 0; i < 100; ++i) {
        MiniSonic::DataPlane::Packet pkt(
            "aa:bb:cc:dd:ee:ff",
            "ff:ee:dd:cc:bb:aa",
            "10.1.1.100",
            "10.1.1.42",
            i
        );
        queue.push(std::move(pkt));
    }
    
    MiniSonic::DataPlane::Packet pkt;
    
    for (auto _ : state) {
        bool popped = queue.pop(pkt);
        if (popped) {
            queue.push(std::move(pkt)); // Push back for next iteration
        }
        benchmark::DoNotOptimize(popped);
    }
    
    state.SetItemsProcessed(state.iterations());
}

void RegisterModularBenchmarks() {
    BENCHMARK(BM_ModularPacketCreation)
        ->Name("ModularPacketCreation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_ModularMetricsOperations)
        ->Name("ModularMetricsOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_ModularStringUtilities)
        ->Name("ModularStringUtilities")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_ModularTimeUtilities)
        ->Name("ModularTimeUtilities")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_ModularNetworkValidation)
        ->Name("ModularNetworkValidation")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
    
    BENCHMARK(BM_ModularQueueOperations)
        ->Name("ModularQueueOperations")
        ->Unit(benchmark::kNanosecond)
        ->MeasureProcessCPUTime()
        ->UseRealTime();
}
