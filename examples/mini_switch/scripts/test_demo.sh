#!/bin/bash
# Quick test script for mini-switch demo

set -e

echo "=== Quick Mini Switch Test ==="

# Clean up any existing setup
echo "Cleaning up previous test..."
ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip link del veth_sw1 2>/dev/null || true
ip link del veth_sw2 2>/dev/null || true

# Create network namespaces
echo "Creating test namespaces..."
ip netns add ns1
ip netns add ns2

# Create veth pairs
ip link add veth1 type veth peer name veth_sw1
ip link add veth2 type veth peer name veth_sw2

# Move veth ends to namespaces
ip link set veth1 netns ns1
ip link set veth2 netns ns2

# Configure IP addresses
ip -n ns1 addr add 10.0.0.1/24 dev veth1
ip -n ns2 addr add 10.0.0.2/24 dev veth2

# Bring up interfaces
ip -n ns1 link set veth1 up
ip -n ns2 link set veth2 up
ip link set veth_sw1 up
ip link set veth_sw2 up

echo "Network setup complete."
echo "ns1: 10.0.0.1/24"
echo "ns2: 10.0.0.2/24"

# Function to cleanup
cleanup() {
    echo ""
    echo "Cleaning up..."
    pkill -f "mini_switch" 2>/dev/null || true
    ip netns del ns1 2>/dev/null || true
    ip netns del ns2 2>/dev/null || true
    ip link del veth_sw1 2>/dev/null || true
    ip link del veth_sw2 2>/dev/null || true
    echo "Cleanup complete."
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

echo ""
echo "Starting mini-switch for 10 seconds..."
sudo timeout 10s ./bin/mini_switch veth_sw1 veth_sw2 &
SW_PID=$!

sleep 2

echo "Testing connectivity..."
if ip netns exec ns1 ping -c 2 -W 1 10.0.0.2 >/dev/null 2>&1; then
    echo "✅ SUCCESS: Ping from ns1 to ns2 works!"
else
    echo "❌ FAILED: Ping from ns1 to ns2 failed"
fi

echo ""
echo "Test completed. Mini-switch will stop automatically."
