# Mini SONiC

A protocol-oriented low-level networking stack and protocol-oriented system, demonstrating understanding of networking primitives, pluggable protocols, and zero-copy serialization.

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

Mini SONiC is a production-grade protocol-oriented networking stack that implements core networking primitives:

- **Pluggable Protocol Architecture** - Extensible protocol handler interface
- **Zero-Copy Serialization** - FlatBuffers-based serialization layer
- **Native WebSocket Gateway** - Real-time bidirectional communication
- **P2P Gossip Protocol** - Lightweight peer discovery and membership
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
├── include/              # Shared headers
│   ├── constants.h      # Centralized string literals and constants
│   ├── cross_platform.h # Platform abstraction layer
│   └── freertos_config.h # FreeRTOS simulation stubs
├── src/                  # C++ modular source
│   ├── modules/         # C++20 modules
│   │   ├── protocol.ixx      # Protocol handler interface (pluggable)
│   │   ├── serialization.ixx # FlatBuffers zero-copy serialization
│   │   ├── websocket.ixx     # Native WebSocket gateway
│   │   ├── gossip.ixx        # P2P gossip protocol
│   │   ├── dataplane.ixx     # Data plane module
│   │   ├── networking.ixx    # Networking module
│   │   ├── l2l3.ixx          # L2/L3 services
│   │   ├── sai.ixx           # SAI abstraction
│   │   ├── utils.ixx         # Utilities
│   │   └── app.ixx           # Application
│   ├── realtime_demo.c   # Real-time packet processing demo
│   └── common/          # Common headers
├── mini_switch/         # C implementation
│   ├── include/         # Mini switch headers
│   ├── src/             # Mini switch source
│   └── CMakeLists.txt   # Mini switch build
└── CMakeLists.txt       # Main build configuration
```

## Features

### Protocol-Oriented Architecture
- **Pluggable Protocol Handlers**: Extensible IProtocolHandler interface for custom protocols
- **Protocol Stack Manager**: Unified management of multiple protocol implementations
- **Zero-Copy Serialization**: FlatBuffers-based serialization for high performance
- **Native WebSocket Gateway**: C++ implementation replacing Python demo
- **P2P Gossip Protocol**: Lightweight peer discovery and membership management

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

### Using the Protocol Stack

```cpp
#include "protocol.ixx"
#include "websocket.ixx"
#include "gossip.ixx"

using namespace MiniSonic::Protocol;
using namespace MiniSonic::WebSocket;
using namespace MiniSonic::Gossip;

// Create protocol stack
ProtocolStack stack;

// Register WebSocket gateway
auto ws_config = WebSocketFactory::defaultGatewayConfig();
ws_config.listen_port = 8080;
auto ws_gateway = WebSocketFactory::createGateway(ws_config);
stack.registerHandler("websocket", std::move(ws_gateway));

// Register P2P gossip protocol
auto gossip_config = GossipFactory::defaultConfig();
auto gossip_protocol = GossipFactory::createProtocol(
    GossipFactory::generatePeerId(),
    gossip_config
);
stack.registerHandler("gossip", std::move(gossip_protocol));

// Set global message handler
stack.setGlobalMessageHandler([](const string& protocol, const Message& msg) {
    std::cout << "Received message from " << protocol << "\n";
});

// Start all protocols
stack.startAll();

// Broadcast message through all protocols
Message message(MessageType::DATA, {0x01, 0x02, 0x03});
stack.broadcast(message);

// Stop when done
stack.stopAll();
```

## License

[Specify your license here]
