#!/bin/bash
# Simple script to run mini-switch with dashboard

set -e

echo "=== Mini Switch Simple Dashboard Demo ==="

# Clean up any existing setup
echo "Cleaning up previous demo..."
pkill -f "mini_switch_simple" 2>/dev/null || true
ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip link del veth_sw1 2>/dev/null || true
ip link del veth_sw2 2>/dev/null || true

# Create network namespaces
echo "Setting up network namespaces..."
ip netns add ns1
ip netns add ns2
ip link add veth1 type veth peer name veth_sw1
ip link add veth2 type veth peer name veth_sw2
ip link set veth1 netns ns1
ip link set veth2 netns ns2
ip -n ns1 addr add 10.0.0.1/24 dev veth1
ip -n ns2 addr add 10.0.0.2/24 dev veth2
ip -n ns1 link set veth1 up
ip -n ns2 link set veth2 up
ip link set veth_sw1 up
ip link set veth_sw2 up
ip link set veth_sw1 promisc on
ip link set veth_sw2 promisc on

echo "Network setup complete!"
echo "ns1: 10.0.0.1/24 (veth1 <-> veth_sw1)"
echo "ns2: 10.0.0.2/24 (veth2 <-> veth_sw2)"

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    pkill -f "mini_switch_simple" 2>/dev/null || true
    sleep 1
    ip netns del ns1 2>/dev/null || true
    ip netns del ns2 2>/dev/null || true
    ip link del veth_sw1 2>/dev/null || true
    ip link del veth_sw2 2>/dev/null || true
    echo "Cleanup complete."
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

# Check if binary exists
if [ ! -f "./bin/mini_switch_simple" ]; then
    echo "Building simple dashboard version..."
    make -f Makefile.simple
fi

echo ""
echo "🚀 Starting mini-switch with simple dashboard..."
echo "📊 You will see real-time statistics and packet flow"
echo "🎯 Press 'q' in dashboard to quit"
echo ""

# Start mini-switch
sudo ./bin/mini_switch_simple veth_sw1 veth_sw2 --simple-dashboard &
SW_PID=$!

sleep 2

# Start some traffic in background
echo "🔄 Generating test traffic..."
sleep 2
ip netns exec ns1 ping -c 2 10.0.0.2 &

echo ""
echo "✅ Dashboard is running!"
echo "📱 Watch the terminal for real-time visualization"
echo "⌨️  Press Ctrl+C here to stop everything"

wait $SW_PID
