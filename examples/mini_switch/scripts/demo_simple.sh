#!/bin/bash
# Demo script with simple real-time dashboard

set -e

echo "=== Mini Switch Simple Dashboard Demo ==="
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
    pkill -f "mini_switch_simple" 2>/dev/null || true
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

echo "Starting mini-switch with simple dashboard..."
echo "You will see a real-time visualization dashboard."
echo ""

# Check if binary exists
if [ ! -f "./bin/mini_switch_simple" ]; then
    echo "Error: mini_switch_simple binary not found. Building first..."
    make simple
fi

# Start mini-switch in background with simple dashboard mode
echo "Executing: sudo ./bin/mini_switch_simple veth_sw1 veth_sw2 --simple-dashboard"
sudo ./bin/mini_switch_simple veth_sw1 veth_sw2 --simple-dashboard &
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
echo "1. Ping from ns1 to ns2 (should learn MAC addresses):"
echo "   ip netns exec ns1 ping -c 3 10.0.0.2"
echo ""

# Run ping test in background
ip netns exec ns1 ping -c 3 10.0.0.2 &
PING_PID=$!

# Let ping run while showing dashboard
sleep 5

echo ""
echo "2. Continuous traffic generation..."
echo "   Running continuous ping to show live dashboard updates"
echo ""

# Start continuous ping in background
ip netns exec ns1 ping 10.0.0.2 &
CONTINUOUS_PID=$!

echo ""
echo "=== Simple Dashboard Controls ==="
echo "• Press 'q' in dashboard to quit"
echo "• Watch real-time packet flow in console"
echo "• Monitor MAC/ARP table updates"
echo "• See live statistics"
echo ""
echo "Dashboard is running... Press Ctrl+C here to stop demo"

# Wait for mini-switch process (will be killed by cleanup or by 'q' in dashboard)
wait $SW_PID
