# Mini SONiC

Protocol-oriented networking stack. C++20 modules, Boost.Asio, Qt6 visualization.

## Documentation

Interactive: `docs/index.html`

## Components

| Component | Implementation |
|-----------|---------------|
| Protocol Stack | Pluggable handlers, WebSocket, Gossip |
| Control Plane | L2/L3 services, MAC learning, LPM |
| Data Plane | Packet pipeline, SAI abstraction |
| Visualization | Qt6, Boost.Asio, std::function callbacks |

## Architecture

```
Protocol Stack -> Networking Layer -> L2/L3 Services -> SAI -> Data Plane
```

## Project Structure

```
src/
  applications/    # Application module
  core/           # Types, constants, events, utils
  networking/     # Networking module
  protocols/      # WebSocket, Gossip, Serialization
  switching/      # L2/L3, SAI, data plane
  visualization/  # Qt6 visualizer, event server
tests/            # Unit and integration tests
tools/            # Benchmarks, CLI
examples/         # Demo applications
docs/             # HTML documentation
```

## Build Requirements

- Compiler: MSVC 19.5+ / GCC 11+ / Clang 13+
- CMake 3.16+
- Libraries: Boost.Asio, nlohmann/json, Qt6 (optional)

## Build Commands

```bash
cmake --preset msvc-debug
cmake --build --preset msvc-debug
```

## Build Targets

| Target | Description |
|--------|-------------|
| `mini_sonic_modular` | Main C++20 application |
| `mini_sonic_qt_visualizer` | Qt6 visualizer |
| `mini_sonic_event_server` | WebSocket event server |
| `mini_sonic_benchmarks` | Performance benchmarks |
| `mini_sonic_*_tests` | Test suites |

## Semantic Types

```cpp
using PortNo = std::uint16_t;
using SpeedMultiplier = int;
using PacketId = std::string;
using SwitchId = std::string;
```

## Usage

```bash
# Run main application
./build/msvc-debug/mini_sonic_modular.exe

# Run Qt visualizer
./build/msvc-debug-qt/mini_sonic_qt_visualizer.exe

# View web visualization
open web_visualizer/index.html
```

## Configuration

Topology: `examples/config/topology.json`
