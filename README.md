# Mini SONiC

Protocol-oriented low-level networking stack implementing networking primitives, pluggable protocols, and zero-copy serialization.

## Documentation

Interactive HTML documentation available at [docs/index.html](docs/index.html).

Documentation sections:
- Overview - System architecture and features
- Architecture - Component interactions and dependencies
- Modules - C++20 module structure
- Data Flow - Packet processing pipeline
- Cross-Platform - Platform support
- Build - Build process and targets

Open `docs/index.html` in a browser to view interactive diagrams.

## Overview

Mini SONiC implements core networking primitives:

- Pluggable protocol architecture
- Zero-copy serialization (FlatBuffers)
- WebSocket gateway
- P2P gossip protocol
- Control/data plane separation
- SAI abstraction layer
- L2 switching with MAC learning
- L3 routing with LPM
- C++20 modules
- Cross-platform support
- Visual dashboard

## Architecture

```
                 +----------------------+
                 | Protocol Stack       |
                 | (Pluggable Handlers)|
                 +----------+-----------+
                            |
        +-------------------+-------------------+
        |                   |                   |
+-------v--------+   +-----v------+     +------v-------+
| WebSocket GW   |   | Gossip P2P  |     | Serialization|
+-------+--------+   +-----+------+     +------v-------+
        |                   |                   |
        +-------------------+-------------------+
                            |
                 +----------v-----------+
                 | Networking Layer     |
                 +----------+-----------+
                            |
        +-------------------+-------------------+
        |                                       |
+-------v--------+                     +---------v--------+
| L2 Service     |                     | L3 Service       |
| (MAC table)    |                     | (LPM Routing)    |
+-------+--------+                     +---------+--------+
        |                                        |
        +-------------------+--------------------+
                            |
                 +----------v-----------+
                 | SAI Abstraction     |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | Data Plane Pipeline  |
                 +----------------------+
```

## Project Structure

```
Mini_SONiC/
├── src/
│   ├── applications/   # Application module
│   ├── core/          # Core types, constants, events, utils
│   ├── networking/    # Networking module
│   ├── protocols/     # Protocol handlers (WebSocket, Gossip, Serialization)
│   ├── switching/     # L2/L3 services, SAI, data plane
│   ├── visualization/ # Visual dashboard
│   └── main.cpp       # Entry point
├── tests/             # Unit and integration tests
├── tools/             # Benchmarks, CLI, scripts
├── examples/          # Example applications
├── docs/              # Documentation
├── web_visualizer/    # Web-based visualization
├── CMakeLists.txt     # Build configuration
└── CMakePresets.json  # CMake presets for build configurations
```

## Features

### Protocol-Oriented Architecture
- Pluggable protocol handlers
- Protocol stack manager
- Zero-copy serialization (FlatBuffers)
- WebSocket gateway
- P2P gossip protocol

### Core Networking
- L2 switching with MAC learning
- L3 routing with LPM (binary trie)
- ARP cache management
- SAI abstraction layer

### C++20 Modules
- Modular design with C++20 modules
- Semantic type aliases
- Module dependency graph
- Incremental builds

### Cross-Platform Support
- Windows (MSVC)
- Unix/Linux (native)
- Platform abstraction layer

## Build

### Prerequisites
- C++20 compatible compiler (MSVC 19.5+, GCC 11+, Clang 13+)
- CMake 3.16+
- Python 3.7+
- Boost libraries (networking components)
- nlohmann/json (event serialization)

### Build with CMake Presets

CMakePresets.json defines build configurations:

```bash
# Configure with preset
cmake --preset msvc-debug

# Build
cmake --build --preset msvc-debug
```

Available presets:
- `msvc-debug` - Debug build with MSVC
- `msvc-release` - Release build with MSVC
- `gcc-debug` - Debug build with GCC
- `gcc-release` - Release build with GCC

### Build Targets

- `mini_sonic_modular` - Main C++20 modular application
- `mini_sonic_c` - C implementation
- `mini_sonic_windows` - Windows-specific C build
- `mini_sonic_event_server` - WebSocket event server for visualization
- `mini_sonic_benchmarks` - Performance benchmarks
- `mini_sonic_protocol_demo` - Protocol stack demonstration
- `mini_sonic_qt_visualizer` - Qt-based visualizer
- `mini_sonic_integration_tests` - Integration test suite
- `mini_sonic_core_tests` - Core unit tests
- `mini_sonic_protocol_tests` - Protocol unit tests
- `mini_sonic_switching_tests` - Switching unit tests

## Development

### Code Style

- Doxygen comments on all public APIs
- Semantic type aliases for domain concepts
- Constants centralized in `src/core/constants.ixx`

## Demo Usage

### Running the Demo

```bash
# Build the project
cmake --preset msvc-debug
cmake --build --preset msvc-debug

# Run the main application
.\build\msvc-debug\mini_sonic_modular.exe

# Run the event server for web visualization
.\build\msvc-debug\mini_sonic_event_server.exe
```

### Web Visualization

Open `web_visualizer/index.html` in a browser to view packet flow visualization.

### Topology Configuration

Topology is defined in `examples/config/topology.json`. The application loads this file at startup to configure network hosts, switches, and links.

## License

[Specify your license here]
