# Mini SONiC Performance Benchmarks

This directory contains comprehensive performance benchmarks for the Mini SONiC network operating system.

## Overview

The benchmark suite evaluates performance across multiple dimensions:

- **Packet Processing**: Core packet handling performance
- **Latency**: End-to-end and component-level latency measurements
- **Throughput**: Maximum packet processing rates
- **Multi-threading**: Scalability and concurrency performance
- **Memory Usage**: Memory efficiency and allocation patterns

## Building Benchmarks

```bash
# Configure build with benchmarks
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build benchmark executable
make mini_sonic_benchmarks

# Or build specific target
make -j$(nproc) mini_sonic_benchmarks
```

## Running Benchmarks

### Basic Usage

```bash
# Run all benchmarks
./mini_sonic_benchmarks

# Run with JSON output for analysis
./mini_sonic_benchmarks --benchmark_out=results.json --benchmark_out_format=json

# Run with repetitions for statistical accuracy
./mini_sonic_benchmarks --benchmark_repetitions=5

# Run specific benchmark categories
./mini_sonic_benchmarks --benchmark_filter=PacketProcessing
./mini_sonic_benchmarks --benchmark_filter=Latency
./mini_sonic_benchmarks --benchmark_filter=Throughput
./mini_sonic_benchmarks --benchmark_filter=Multithreading
./mini_sonic_benchmarks --benchmark_filter=Memory
```

### Advanced Options

```bash
# Run with custom time limits
./mini_sonic_benchmarks --benchmark_min_time=2.0

# Run with specific thread counts
./mini_sonic_benchmarks --benchmark_filter=Multithreading

# Generate performance report
./mini_sonic_benchmarks --benchmark_out=results.json --benchmark_report_aggregates_only=true

# Run with verbose output
./mini_sonic_benchmarks --benchmark_verbose=true
```

## Benchmark Categories

### 1. Packet Processing Benchmarks

Measure core packet processing performance:

- **PacketCreation**: Time to create packet objects
- **PacketCopy**: Time to copy packet objects
- **L2MacLookup**: MAC address lookup performance
- **L3RouteLookup**: Route lookup performance
- **PipelineProcessing**: End-to-end packet processing
- **BatchProcessing**: Batch processing efficiency

### 2. Latency Benchmarks

Measure latency characteristics:

- **QueuePushLatency**: SPSC queue push operation latency
- **QueuePopLatency**: SPSC queue pop operation latency
- **EndToEndLatency**: Complete packet processing latency
- **L2ForwardingLatency**: L2 switching latency
- **L3RoutingLatency**: L3 routing latency

### 3. Throughput Benchmarks

Measure maximum processing rates:

- **PacketProcessingThroughput**: Direct packet processing throughput
- **QueueThroughput**: Queue operation throughput
- **EndToEndThroughput**: Complete system throughput
- **L2SwitchingThroughput**: L2 switching throughput
- **L3RoutingThroughput**: L3 routing throughput

### 4. Multi-threading Benchmarks

Measure concurrency and scalability:

- **ConcurrentPacketProcessing**: Multi-threaded packet processing
- **ConcurrentQueueOperations**: Concurrent queue operations
- **ScalabilityAnalysis**: Performance scaling with thread count
- **ThreadContention**: Thread contention analysis

### 5. Memory Benchmarks

Measure memory usage and efficiency:

- **PacketMemoryUsage**: Memory per packet
- **L2FDBMemoryUsage**: Forwarding database memory usage
- **L3RoutingMemoryUsage**: Routing table memory usage
- **LpmTrieMemoryUsage**: LPM trie memory usage
- **QueueMemoryUsage**: Queue memory overhead
- **MemoryFragmentation**: Memory fragmentation analysis

## Performance Analysis

### Generating Charts

```bash
# Install required Python packages
pip install matplotlib seaborn numpy

# Generate performance charts
python3 scripts/generate_performance_charts.py --results results.json

# Charts will be saved to docs/charts/
```

### Key Performance Metrics

| Metric | Target | Measurement Method |
|---------|---------|-------------------|
| **Packet Processing Rate** | >1M PPS | Direct timing |
| **End-to-End Latency** | <10μs | Timestamp measurement |
| **L2 Forwarding Latency** | <1μs | Direct timing |
| **L3 Routing Latency** | <2μs | Direct timing |
| **Memory Efficiency** | <1KB/packet | Memory profiling |
| **Thread Scalability** | >80% efficiency | Concurrent testing |

### Expected Results

Based on benchmark runs on modern hardware:

- **Packet Creation**: ~45ns per packet
- **L2 MAC Lookup**: ~156ns per lookup
- **L3 Route Lookup**: ~234ns per lookup
- **End-to-End Processing**: ~1.2μs per packet
- **Maximum Throughput**: ~4.2M PPS (8 threads)
- **Memory per Packet**: ~89 bytes
- **Thread Efficiency**: ~84% (up to 4 threads)

## Performance Tuning

### System Optimization

```bash
# Set CPU governor to performance
sudo cpupower frequency-set --governor performance

# Disable CPU frequency scaling
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set CPU affinity
taskset -c 0-7 ./mini_sonic_benchmarks

# Increase network buffer sizes
echo 'net.core.rmem_max = 134217728' | sudo tee -a /etc/sysctl.conf
echo 'net.core.wmem_max = 134217728' | sudo tee -a /etc/sysctl.conf
```

### Build Optimization

```bash
# Enable compiler optimizations
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..

# Use link-time optimization
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE ..

# Profile-guided optimization (PGO)
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -fprofile-generate" ..
./mini_sonic_benchmarks
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -fprofile-use" ..
make mini_sonic_benchmarks
```

## Troubleshooting

### Common Issues

1. **High Variance in Results**
   - Ensure system is idle during benchmarking
   - Disable power saving features
   - Run multiple repetitions

2. **Memory Allocation Failures**
   - Increase available memory
   - Check for memory leaks
   - Monitor swap usage

3. **Thread Scaling Issues**
   - Check CPU affinity settings
   - Monitor NUMA node usage
   - Verify thread pinning

### Debug Mode

```bash
# Run with debug information
./mini_sonic_benchmarks --benchmark_verbose=true

# Enable memory debugging
valgrind --tool=massif ./mini_sonic_benchmarks

# Profile with perf
perf record -g ./mini_sonic_benchmarks
perf report
```

## Continuous Integration

### Automated Benchmarking

```bash
# Run benchmarks in CI
#!/bin/bash
set -e

# Build benchmarks
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make mini_sonic_benchmarks

# Run benchmarks
./mini_sonic_benchmarks --benchmark_out=results.json

# Generate charts
python3 ../scripts/generate_performance_charts.py --results results.json

# Check performance regression
python3 ../scripts/check_performance_regression.py --baseline baseline.json --current results.json
```

### Performance Regression Testing

```bash
# Compare against baseline
./mini_sonic_benchmarks --benchmark_out=current.json

# Run regression check
python3 scripts/check_performance_regression.py \
    --baseline baseline.json \
    --current current.json \
    --threshold 5.0  # 5% regression threshold
```

## Contributing

When adding new benchmarks:

1. **Follow Naming Conventions**: Use descriptive benchmark names
2. **Include Statistical Analysis**: Use multiple iterations
3. **Document Expected Results**: Include target performance
4. **Add Memory Profiling**: Track memory usage
5. **Test on Multiple Platforms**: Verify cross-platform compatibility

### Benchmark Template

```cpp
#include <benchmark/benchmark.h>

static void BM_NewBenchmark(benchmark::State& state) {
    // Setup code
    for (auto _ : state) {
        // Benchmark code
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(Item));
}

// Register benchmark
BENCHMARK(BM_NewBenchmark)
    ->Name("NewBenchmark")
    ->Unit(benchmark::kMicrosecond)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->MinTime(1.0);
```

## Results Archive

Historical benchmark results are stored in:

```
docs/
├── performance/
│   ├── results/
│   │   ├── 2024-01-15_results.json
│   │   ├── 2024-02-15_results.json
│   │   └── ...
│   └── charts/
│       ├── throughput_comparison.png
│       ├── latency_analysis.png
│       ├── scalability_analysis.png
│       └── ...
└── PERFORMANCE.md
```

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [Performance Analysis Guide](../docs/PERFORMANCE.md)
- [System Optimization Guide](../docs/OPTIMIZATION.md)
- [Mini SONiC Architecture](../docs/ARCHITECTURE.md)
