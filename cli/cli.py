#!/usr/bin/env python3
"""
Mini SONiC CLI Interface
Provides command-line interface for managing the network OS
"""

import requests
import argparse
import sys
from typing import Dict, Any

class MiniSonicCLI:
    def __init__(self, api_url: str = "http://localhost:8080"):
        self.api_url = api_url
    
    def show_mac(self) -> None:
        """Display MAC table"""
        try:
            response = requests.get(f"{self.api_url}/api/mac")
            if response.status_code == 200:
                data = response.json()
                print("MAC Address Table:")
                print("{:<18} {:<10}".format("MAC Address", "Port"))
                print("-" * 30)
                for mac, port in data.get('mac_table', {}).items():
                    print("{:<18} {:<10}".format(mac, port))
            else:
                print(f"Error: {response.status_code}")
        except requests.exceptions.ConnectionError:
            print("Error: Cannot connect to Mini SONiC daemon")
    
    def show_routes(self) -> None:
        """Display routing table"""
        try:
            response = requests.get(f"{self.api_url}/api/routes")
            if response.status_code == 200:
                data = response.json()
                print("Routing Table:")
                print("{:<18} {:<15}".format("Prefix", "Next Hop"))
                print("-" * 35)
                for prefix, nexthop in data.get('routes', {}).items():
                    print("{:<18} {:<15}".format(prefix, nexthop))
            else:
                print(f"Error: {response.status_code}")
        except requests.exceptions.ConnectionError:
            print("Error: Cannot connect to Mini SONiC daemon")
    
    def add_route(self, prefix: str, nexthop: str) -> None:
        """Add a route"""
        try:
            data = {"prefix": prefix, "next_hop": nexthop}
            response = requests.post(f"{self.api_url}/api/routes", json=data)
            if response.status_code == 200:
                print(f"Route {prefix} -> {nexthop} added successfully")
            else:
                print(f"Error: {response.status_code}")
        except requests.exceptions.ConnectionError:
            print("Error: Cannot connect to Mini SONiC daemon")
    
    def remove_route(self, prefix: str) -> None:
        """Remove a route"""
        try:
            response = requests.delete(f"{self.api_url}/api/routes/{prefix}")
            if response.status_code == 200:
                print(f"Route {prefix} removed successfully")
            else:
                print(f"Error: {response.status_code}")
        except requests.exceptions.ConnectionError:
            print("Error: Cannot connect to Mini SONiC daemon")
    
    def send_packet(self, src_mac: str, dst_mac: str, src_ip: str, dst_ip: str, port: int) -> None:
        """Send a test packet"""
        try:
            data = {
                "src_mac": src_mac,
                "dst_mac": dst_mac,
                "src_ip": src_ip,
                "dst_ip": dst_ip,
                "ingress_port": port
            }
            response = requests.post(f"{self.api_url}/api/packet", json=data)
            if response.status_code == 200:
                print("Packet sent successfully")
            else:
                print(f"Error: {response.status_code}")
        except requests.exceptions.ConnectionError:
            print("Error: Cannot connect to Mini SONiC daemon")

def main():
    parser = argparse.ArgumentParser(description="Mini SONiC CLI")
    parser.add_argument("--url", default="http://localhost:8080", 
                       help="API URL (default: http://localhost:8080)")
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Show commands
    subparsers.add_parser('show-mac', help='Show MAC table')
    subparsers.add_parser('show-routes', help='Show routing table')
    
    # Route commands
    route_parser = subparsers.add_parser('add-route', help='Add a route')
    route_parser.add_argument('prefix', help='IP prefix (e.g., 10.0.0.0/24)')
    route_parser.add_argument('nexthop', help='Next hop IP address')
    
    remove_route_parser = subparsers.add_parser('remove-route', help='Remove a route')
    remove_route_parser.add_argument('prefix', help='IP prefix to remove')
    
    # Packet command
    packet_parser = subparsers.add_parser('send-packet', help='Send a test packet')
    packet_parser.add_argument('--src-mac', required=True, help='Source MAC address')
    packet_parser.add_argument('--dst-mac', required=True, help='Destination MAC address')
    packet_parser.add_argument('--src-ip', required=True, help='Source IP address')
    packet_parser.add_argument('--dst-ip', required=True, help='Destination IP address')
    packet_parser.add_argument('--port', type=int, required=True, help='Ingress port')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    cli = MiniSonicCLI(args.url)
    
    if args.command == 'show-mac':
        cli.show_mac()
    elif args.command == 'show-routes':
        cli.show_routes()
    elif args.command == 'add-route':
        cli.add_route(args.prefix, args.nexthop)
    elif args.command == 'remove-route':
        cli.remove_route(args.prefix)
    elif args.command == 'send-packet':
        cli.send_packet(args.src_mac, args.dst_mac, args.src_ip, args.dst_ip, args.port)

if __name__ == "__main__":
    main()
