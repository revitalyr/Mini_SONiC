#!/bin/bash

# Mini SONiC Demo Script
# Demonstrates multi-switch topology and packet forwarding

echo "=== Mini SONiC Demo ==="
echo "Starting 3-switch topology..."

# Start the topology
docker-compose -f docker/docker-compose.yml up -d

echo "Waiting for switches to initialize..."
sleep 10

echo "=== Demo Scenario ==="
echo "1. Show MAC tables on all switches"
echo "2. Add routes between switches"
echo "3. Send test packets"
echo "4. Verify forwarding"

# Function to execute CLI command
exec_cli() {
    local switch=$1
    shift
    echo "Switch $switch: $*"
    python3 cli/cli.py --url http://localhost:808$switch "$@"
    echo ""
}

echo "=== Initial MAC Tables ==="
exec_cli 1 show-mac
exec_cli 2 show-mac
exec_cli 3 show-mac

echo "=== Adding Routes ==="
exec_cli 1 add-route 192.168.2.0/24 172.20.0.12
exec_cli 2 add-route 192.168.1.0/24 172.20.0.11
exec_cli 2 add-route 192.168.3.0/24 172.20.0.13
exec_cli 3 add-route 192.168.2.0/24 172.20.0.12

echo "=== Route Tables After Configuration ==="
exec_cli 1 show-routes
exec_cli 2 show-routes
exec_cli 3 show-routes

echo "=== Sending Test Packets ==="
exec_cli 1 send-packet --src-mac aa:bb:cc:dd:ee:01 --dst-mac ff:ee:dd:cc:bb:02 --src-ip 192.168.1.10 --dst-ip 192.168.2.10 --port 1

echo "=== Demo Complete ==="
echo "Check the switch logs to see packet processing:"
echo "docker logs mini-sonic-switch1"
echo "docker logs mini-sonic-switch2"
echo "docker logs mini-sonic-switch3"

echo ""
echo "To stop the demo:"
echo "docker-compose -f docker/docker-compose.yml down"
