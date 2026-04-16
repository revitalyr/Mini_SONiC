# Mini SONiC

A simplified Network Operating System inspired by SONiC, demonstrating understanding of networking stacks, control/data plane separation, and SAI abstraction.

## 📚 Interactive Documentation

**NEW:** Check out the [interactive HTML documentation](docs/index.html) with Mermaid diagrams!

The documentation includes:
- 📊 **Overview** - System architecture and key features
- 🏗️ **Architecture** - Component interactions and module dependencies
- 🧩 **Modules** - C++20 module structure
- 🔄 **Data Flow** - Packet processing pipeline
- 💻 **Cross-Platform** - Windows/Unix support
- 🚀 **Build** - Build process and targets

Open `docs/index.html` in your browser to explore the interactive diagrams with zoom controls.

## Overview

Mini SONiC is a production-grade demonstration project that implements core networking concepts:

- **Control Plane / Data Plane Separation**
- **SAI (Switch Abstraction Interface) Layer**
- **L2 Switching with MAC learning**
- **L3 Routing with Longest Prefix Match (LPM)**
- **C++20 Modules for modular architecture**
- **Cross-platform support (Windows/Unix)**
- **Visual dashboard with ncurses**

## Architecture

```
                 +----------------------+
                 | CLI / Dashboard      |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | Config Manager       |
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
├── include/              # Shared headers
│   ├── constants.h      # Centralized string literals and constants
│   ├── cross_platform.h # Platform abstraction layer
│   └── freertos_config.h # FreeRTOS simulation stubs
├── src/                  # C++ modular source
│   ├── modules/         # C++20 modules
│   │   ├── dataplane.ixx  # Data plane module
│   │   ├── networking.ixx # Networking module
│   │   ├── l2l3.ixx       # L2/L3 services
│   │   ├── sai.ixx        # SAI abstraction
│   │   ├── utils.ixx      # Utilities
│   │   └── app.ixx        # Application
│   ├── realtime_demo.c   # Real-time packet processing demo
│   └── common/          # Common headers
├── mini_switch/         # C implementation
│   ├── include/         # Mini switch headers
│   ├── src/             # Mini switch source
│   └── CMakeLists.txt   # Mini switch build
└── CMakeLists.txt       # Main build configuration
```

## Features

### Core Networking
- **L2 Switching**: MAC address learning and forwarding
- **L3 Routing**: Longest Prefix Match using binary trie (O(32) lookup)
- **ARP Simulation**: Basic ARP cache management
- **SAI Abstraction**: Decouples control plane from ASIC implementation

### C++20 Modules
- **Modular Design**: Clean separation using C++20 modules
- **Type Safety**: Strong typing with semantic type aliases
- **No Circular Dependencies**: Proper module dependency graph
- **Fast Compilation**: Module-based incremental builds

### Cross-Platform Support
- **Windows**: MSVC with FreeRTOS simulation stubs
- **Unix/Linux**: Native FreeRTOS support
- **Platform Abstraction**: cross_platform.h for OS differences

### Visual Dashboard
- **ncurses Interface**: Real-time MAC table visualization
- **ARP Table Display**: ARP cache monitoring
- **Packet Statistics**: Live packet counters
- **Optional**: Disabled if curses unavailable

## Quick Start

### Prerequisites
- C++20 compatible compiler (MSVC 19.5+, GCC 11+, Clang 13+)
- CMake 3.16+
- Python 3.7+
- ncurses/PDCurses (optional, for visual dashboard)

### Build

```bash
# Clone the repository
git clone <repository-url>
cd Mini_SONiC

# Build the project
mkdir build && cd build
cmake ..
cmake --build .
```

### Build Targets

- `mini_sonic_modular` - C++20 modular application
- `mini_sonic_c` - C implementation
- `mini_sonic_windows` - Windows-specific C build
- `mini_sonic_demo` - Real-time demo
- `mini_sonic_benchmarks` - Performance benchmarks

## Development

### Code Style

- **Doxygen Comments**: All public APIs documented with Doxygen
- **Semantic Types**: Strong type aliases for domain concepts (e.g., `IpAddress`, `MacAddress`)
- **Constants**: All string literals centralized in `include/constants.h`
- **English Only**: All code, comments, and documentation in English

### Adding New Features

1. Define constants in `include/constants.h`
2. Add types in appropriate module (DataPlane, Networking, etc.)
3. Implement in corresponding .ixx and .cpp files
4. Update CMakeLists.txt if needed
5. Add Doxygen documentation

## License

[Specify your license here]
