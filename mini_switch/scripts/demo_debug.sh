#!/bin/bash
# Enhanced demo script with debugging information

set -e

echo "=== Mini Switch Debug Demo ==="
echo "Setting up network namespaces..."

# Clean up any existing setup
echo "Cleaning up previous demo..."
ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip link del veth_sw1 2>/dev/null || true
ip link del veth_sw2 2>/dev/null || true

# Create network namespaces
ip netns add ns1
ip netns add ns2

# Create veth pairs
ip link add veth1 type veth peer name veth_sw1
ip link add veth2 type veth peer name veth_sw2

# Move veth ends to namespaces
ip link set veth1 netns ns1
ip link set veth2 netns ns2

# Configure IP addresses in namespaces
ip -n ns1 addr add 10.0.0.1/24 dev veth1
ip -n ns2 addr add 10.0.0.2/24 dev veth2

# Bring up interfaces in namespaces
ip -n ns1 link set veth1 up
ip -n ns2 link set veth2 up

# Bring up switch interfaces
ip link set veth_sw1 up
ip link set veth_sw2 up

# Enable promiscuous mode on switch interfaces
ip link set veth_sw1 promisc on
ip link set veth_sw2 promisc on

echo "Network namespaces setup complete."
echo "ns1: 10.0.0.1/24 (veth1 <-> veth_sw1)"
echo "ns2: 10.0.0.2/24 (veth2 <-> veth_sw2)"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    # Kill mini-switch if running
    pkill -f "mini_switch" 2>/dev/null || true
    sleep 1
    
    # Delete namespaces and interfaces
    ip netns del ns1 2>/dev/null || true
    ip netns del ns2 2>/dev/null || true
    ip link del veth_sw1 2>/dev/null || true
    ip link del veth_sw2 2>/dev/null || true
    
    echo "Cleanup complete."
    exit 0
}

# Set trap for cleanup
trap cleanup SIGINT SIGTERM EXIT

echo "Starting mini-switch (visual-only mode)..."
echo "You will see colored packet output and statistics."
echo ""

# Check if binary exists
if [ ! -f "./bin/mini_switch" ]; then
    echo "Error: mini_switch binary not found. Building first..."
    make
fi

# Start mini-switch in background with visual-only mode
echo "Executing: sudo ./bin/mini_switch veth_sw1 veth_sw2 --visual-only"
sudo ./bin/mini_switch veth_sw1 veth_sw2 --visual-only &
SW_PID=$!

echo "Mini-switch started with PID: $SW_PID"

# Wait for switch to start
sleep 3

# Check if switch is still running
if ! kill -0 $SW_PID 2>/dev/null; then
    echo "Error: Mini-switch failed to start or crashed immediately"
    echo "Check if you have proper permissions and dependencies"
    exit 1
fi

echo "Mini-switch is running successfully!"
echo ""

echo "=== Testing Connectivity ==="
echo "1. Checking interface status..."
echo "veth_sw1 status:"
ip link show veth_sw1
echo ""
echo "veth_sw2 status:"
ip link show veth_sw2
echo ""

echo "2. Ping from ns1 to ns2 (should learn MAC addresses):"
echo "   ip netns exec ns1 ping -c 3 10.0.0.2"
echo ""

# Run ping test with verbose output
echo "Running ping test..."
ip netns exec ns1 ping -c 3 -W 2 10.0.0.2 || echo "Ping failed - check switch output"

echo ""
echo "3. Ping from ns2 to ns1:"
echo "   ip netns exec ns2 ping -c 3 10.0.0.1"
echo ""

ip netns exec ns2 ping -c 3 -W 2 10.0.0.1 || echo "Ping failed - check switch output"

echo ""
echo "4. ARP table check:"
echo "ns1 ARP table:"
ip netns exec ns1 arp -n
echo ""
echo "ns2 ARP table:"
ip netns exec ns2 arp -n
echo ""

echo "5. Route table check:"
echo "ns1 routes:"
ip netns exec ns1 route -n
echo ""
echo "ns2 routes:"
ip netns exec ns2 route -n
echo ""

echo "=== Continuous Test ==="
echo "Running continuous ping for 15 seconds to show packet visualization..."
echo "Watch for colored packets: 🟢[L2] 🟡[FLOOD] 🔵[ARP]"
echo ""

# Start continuous ping in background
ip netns exec ns1 ping 10.0.0.2 &
PING_PID=$!

# Let it run for 15 seconds
sleep 15

# Stop ping
kill $PING_PID 2>/dev/null || true

echo ""
echo "=== Final Statistics ==="
echo "Waiting for final statistics update..."
sleep 2

echo ""
echo "Demo completed. Mini-switch will stop automatically on cleanup."
echo ""

# Wait for mini-switch process (will be killed by cleanup)
wait $SW_PID
