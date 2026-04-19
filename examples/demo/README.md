# Mini SONiC Real Network Demo

This demonstration uses compiled Mini SONiC components to validate system functionality, not simulation.

## Demo Description

The demo creates a real switch network using:
- Real Mini SONiC classes (`SimulatedSai`, `Pipeline`, `PipelineThread`, `SPSCQueue`)
- Real packets in Mini SONiC format (`DataPlane::Packet`)
- Real L2/L3 service processing procedures
- Real packet processing threads

## Demo Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   RealSW1       │    │   RealSW2       │    │   RealSW3       │
│                 │    │                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │SimulatedSai │ │    │ │SimulatedSai │ │    │ │SimulatedSai │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │  Pipeline   │ │    │ │  Pipeline   │ │    │ │  Pipeline   │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │SPSCQueue    │◄┼────┼──►│SPSCQueue    │◄┼────┼──►│SPSCQueue    │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │PipelineThrd │ │    │ │PipelineThrd │ │    │ │PipelineThrd │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Build and Run

### Prerequisites

1. Compile the main Mini SONiC project:
   ```bash
   cd ..
   cmake --build build --config Release
   ```

2. Verify that `build/mini_sonic_lib.lib` exists

### Quick Start

Windows:
```cmd
cd demo
build_and_run_demo.bat
```

### Manual Build

```bash
cd demo
mkdir build && cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DMINI_SONIC_BUILD_DIR=../../build ..
cmake --build . --config Release
```

### Run Demo

```bash
# Basic configuration
./mini_sonic_real_demo --switches 3 --rate 100 --duration 30

# Stress test
./mini_sonic_real_demo --switches 5 --rate 200 --duration 60

# Extended configuration
./mini_sonic_real_demo --switches 8 --rate 100 --duration 120
```

## Command Line Parameters

- `--switches <n>`: Number of switches (default: 3)
- `--rate <n>`: Packets per second (default: 100)
- `--duration <n>`: Duration in seconds (default: 30)
- `--no-viz`: Disable visualization
- `--help`: Show help

## Demo Features

### 1. Real Packet Processing
- Generates actual `DataPlane::Packet` objects
- Processes packets through real `Pipeline`
- Uses actual `SPSCQueue` queue
- Processing in real `PipelineThread` threads

### 2. L2/L3 Functionality
- MAC Learning: Switches learn MAC addresses
- L2 Forwarding: Forwarding based on MAC tables
- IP Routing: L3 routing (if available)
- SAI Abstraction: Operation through SAI interface

### 3. Performance
- Throughput: Measurement of actual throughput
- Latency: Measurement of processing delay
- Queue Depth: Monitoring queue depth
- Packet Loss: Tracking packet loss

### 4. Scalability
- Multi-threading: Real multi-threaded processing
- Lock-free Queues: Lock-free queue implementation
- Memory Management: Packet memory management
- Resource Usage: Resource usage monitoring

## Example Output

```
=== Mini SONiC REAL Network Demo ===
Switches: 3
Packet Rate: 100 pps
Duration: 30 seconds
Log File: demo_output.log
=====================================

[12345] SW1: Initialized - Real Mini SONiC components loaded
[12346] SW2: Initialized - Real Mini SONiC components loaded  
[12347] SW3: Initialized - Real Mini SONiC components loaded

[12348] SW1: Started - Pipeline thread active
[12349] SW2: Started - Pipeline thread active
[12350] SW3: Started - Pipeline thread active

[12351] PKT0: GENERATOR -> SW1 (Generated packet)
[12352] PKT0: INGRESS -> RealSW1 (Packet received)
[12353] RealSW1: MAC Learning - Learned MAC 00:11:22:33:44:01 on port 5
[12354] PKT0: RealSW1 -> EGRESS (Processed in 45μs)

=== REAL-TIME STATISTICS ===
Packets Generated: 1500
Packets Processed: 1485  
Packets Dropped: 15
MAC Learning Events: 89
Route Lookups: 1245
Throughput: 98.50 pps
Average Latency: 47.23 μs
============================
```

## Logging

The demo creates a detailed log file `demo_output.log` with:
- Timestamps of all events
- Packet flow through the system
- Performance statistics
- MAC learning events
- Errors and packet losses

## Validation

The demo validates project functionality by:

1. Compilation: Successful build with real components
2. Execution: Running real processing procedures
3. Integration: Interaction of all system components
4. Performance: Measurement of actual metrics
5. Scalability: Operation under load

## Comparison with Simulation

| Simulation (HTML) | Real Demo |
|-------------------|-----------|
| Visual animation | Actual packet processing |
| Algorithm simulation | Real function calls |
| Fake metrics | Real measurements |
| JavaScript code | Compiled C++ |
| Browser demo | Standalone application |

This demo validates that Mini SONiC operates correctly with actual compiled components.
