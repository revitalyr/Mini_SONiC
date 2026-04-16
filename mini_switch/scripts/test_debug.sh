#!/bin/bash
# Quick debug test

set -e

echo "=== Quick Debug Test ==="

# Clean up
pkill -f "mini_switch" 2>/dev/null || true
ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip link del veth_sw1 2>/dev/null || true
ip link del veth_sw2 2>/dev/null || true

# Setup
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

echo "Setup complete"

# Start switch
echo "Starting switch with debug..."
sudo ./bin/mini_switch veth_sw1 veth_sw2 --visual-only &
SW_PID=$!

sleep 2

# Test
echo "Sending ARP request..."
ip netns exec ns1 arping -c 1 10.0.0.2 &
sleep 3

# Cleanup
pkill -f "mini_switch" 2>/dev/null || true
ip netns del ns1 2>/dev/null || true
ip netns del ns2 2>/dev/null || true
ip link del veth_sw1 2>/dev/null || true
ip link del veth_sw2 2>/dev/null || true

echo "Test complete"
