# Mini Switch - Demo Ready L2/L3 Software Switch

A demonstration-ready software switch with real-time packet visualization. Suitable for demonstrating networking expertise.

## Key Features

- Real-time L2/L3 forwarding
- MAC Learning - automatic network topology learning
- ARP Processing - IP to MAC address resolution
- VLAN Support - IEEE 802.1Q tagging
- ICMP Ping Replies - ping request responses
- Color Visualization - colored packet output
- Interactive CLI - show mac, show arp commands
- Multi-port Threading - operation on multiple interfaces
- Network Namespaces - isolated test environment

## Quick Start

```bash
# Clone and build
cd mini_switch
make

# Run demo (sudo required for network namespaces)
sudo make demo
```

## Demo Setup

### Split-screen Terminal
```
┌─────────────────┬─────────────────┐
│  ns1 terminal   │  ns2 terminal   │
│  ip netns exec  │  ip netns exec  │
│  ns1 ping 10.0.0.2 │  ns2 ping 10.0.0.1 │
└─────────────────┴─────────────────┘
┌─────────────────────────────────────┐
│  mini_switch output                 │
│  [L2] Port 0 -> Port 1             │
│  [ARP] Port 1 -> Port 0            │
│  [FLOOD] Port 0 -> ALL             │
│  mini-switch> show mac              │
└─────────────────────────────────────┘
```

### Example Output
```
[ARP] Port 0 -> Port 1 | 02:42:AC:11:00:02 -> 02:42:AC:11:00:03
[L2]  Port 1 -> Port 0 | 02:42:AC:11:00:03 -> 02:42:AC:11:00:02

=== MAC Table ===
Port 0: 02:42:AC:11:00:02
Port 1: 02:42:AC:11:00:03
================

mini-switch> show arp
=== ARP Table ===
10.0.0.2 -> 02:42:AC:11:00:03
10.0.0.1 -> 02:42:AC:11:00:02
================
```

## Project Structure

```
mini_switch/
├── src/
│   ├── main.c           # Main code with threading + CLI
│   ├── visual.c         # Colored packet visualization
│   ├── forwarding.c     # L2/L3 forwarding logic
│   ├── mac_table.c      # MAC learning table
│   ├── arp.c           # ARP handling
│   └── ...
├── include/
│   ├── visual.h         # Visualization interfaces
│   └── ...
├── scripts/
│   └── demo.sh          # Automated network setup
├── bin/
│   └── mini_switch      # Compiled switch
├── Makefile
├── DEMO.md             # Detailed guide
└── README_DEMO.md      # This file
```

## Test Commands

```bash
# Basic ping test
ip netns exec ns1 ping -c 3 10.0.0.2

# Continuous ping (for visualization)
ip netns exec ns1 ping 10.0.0.2

# CLI commands in switch
mini-switch> show mac
mini-switch> show arp
mini-switch> help
```

## Color Scheme

- [L2] - Known MAC, direct forwarding
- [FLOOD] - Unknown MAC, flood to all ports
- [ARP] - ARP packet (request/reply)
- [VLAN 100] - VLAN tagged packet

## Architecture

### Multi-threading Design
- RX Threads - one per port for packet reception
- Worker Thread - packet processing from queue
- CLI Thread - interactive commands
- Visual Thread - periodic table output

### Lock-free Queue
- Ring Buffer for high-performance packet transfer
- Atomic operations for thread-safety
- Zero-copy forwarding where possible

### Packet Processing Pipeline
```
Raw Packet -> Parse -> Learn MAC -> Lookup -> Forward -> Visualize
```

## Performance

- Throughput: Limited only by network interfaces
- Latency: < 1ms for local veth pairs
- Memory: Minimal consumption (ring buffers)
- CPU: Efficient lock-free processing

## Build and Dependencies

```bash
# Requirements
- GCC with C11 support
- Linux kernel headers
- pthread library
- sudo privileges for network namespaces

# Build
make clean && make

# Debug version
make debug

# Install
make install
```

## Documentation

- DEMO.md - Detailed demo guide
- src/ - Commented source code
- include/ - Module interfaces

## Ready for Use

The project is fully prepared for demonstration or portfolio use. Demonstrates practical knowledge of networking technologies and C systems programming.

