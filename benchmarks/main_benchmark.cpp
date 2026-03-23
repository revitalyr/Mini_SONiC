#include <benchmark/benchmark.h>
#include "utils/metrics.hpp"
#include <iostream>

/**
 * @file main_benchmark.cpp
 * @brief Main entry point for Mini SONiC performance benchmarks
 * 
 * This file serves as the entry point for running comprehensive benchmarks
 * of the Mini SONiC networking stack. It includes all benchmark modules
 * and provides a unified interface for performance testing.
 */

// Forward declarations for benchmark functions
void RegisterPacketProcessingBenchmarks();
void RegisterLatencyBenchmarks();
void RegisterThroughputBenchmarks();
void RegisterMemoryBenchmarks();
void RegisterMultithreadingBenchmarks();

int main(int argc, char** argv) {
    std::cout << "=== Mini SONiC Performance Benchmarks ===\n";
    std::cout << "System Information:\n";
    std::cout << "  C++ Standard: C++20\n";
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
    
    // Reset metrics before benchmarks
    MiniSonic::Utils::Metrics::instance().reset();
    
    // Register all benchmark categories
    RegisterPacketProcessingBenchmarks();
    RegisterLatencyBenchmarks();
    RegisterThroughputBenchmarks();
    RegisterMemoryBenchmarks();
    RegisterMultithreadingBenchmarks();
    
    std::cout << "\nRunning benchmarks...\n";
    std::cout << "Note: Results are saved to JSON format for analysis\n\n";
    
    // Run benchmarks
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    std::cout << "\n=== Benchmark Results Summary ===\n";
    std::cout << MiniSonic::Utils::Metrics::instance().getSummary() << "\n";
    
    return 0;
}
