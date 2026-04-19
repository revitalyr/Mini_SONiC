#!/bin/bash

# Mini Switch Demo Setup Script
# Creates network namespaces and virtual ethernet pairs for testing

set -e

echo "=== Mini Switch Demo Setup ==="
echo "Setting up network topology:"
echo "ns1 --- veth1 ---[mini_switch]--- veth2 --- ns2"
echo ""

# Clean up any existing setup
echo "Cleaning up previous demo..."
sudo ip netns del ns1 2>/dev/null || true
sudo ip netns del ns2 2>/dev/null || true
sudo ip link del veth1 2>/dev/null || true
sudo ip link del veth2 2>/dev/null || true

# Create network namespaces
echo "Creating network namespaces..."
sudo ip netns add ns1
sudo ip netns add ns2

# Create virtual ethernet pair
echo "Creating virtual ethernet pair..."
sudo ip link add veth1 type veth peer name veth2

# Move interfaces to namespaces
echo "Moving interfaces to namespaces..."
sudo ip link set veth1 netns ns1
sudo ip link set veth2 netns ns2

# Configure interfaces in namespaces
echo "Configuring interfaces..."
sudo ip -n ns1 addr add 10.0.0.1/24 dev veth1
sudo ip -n ns2 addr add 10.0.0.2/24 dev veth2

# Bring interfaces up
echo "Bringing interfaces up..."
sudo ip -n ns1 link set veth1 up
sudo ip -n ns2 link set veth2 up

# Add routes in namespaces
echo "Adding routes..."
sudo ip -n ns1 route add default via 10.0.0.254 dev veth1
sudo ip -n ns2 route add default via 10.0.0.254 dev veth2

# Show configuration
echo ""
echo "=== Network Configuration ==="
echo "Namespace ns1:"
sudo ip -n ns1 addr show veth1
sudo ip -n ns1 route show

echo ""
echo "Namespace ns2:"
sudo ip -n ns2 addr show veth2
sudo ip -n ns2 route show

echo ""
echo "=== Demo Setup Complete ==="
echo ""
echo "To run the mini switch:"
echo "  sudo make"
echo "  sudo ./bin/mini_switch veth1 veth2"
echo ""
echo "To test connectivity:"
echo "  sudo ip netns exec ns1 ping 10.0.0.2"
echo "  sudo ip netns exec ns2 ping 10.0.0.1"
echo ""
echo "To cleanup:"
echo "  sudo ip netns del ns1"
echo "  sudo ip netns del ns2"
echo "  sudo ip link del veth1"
echo ""
