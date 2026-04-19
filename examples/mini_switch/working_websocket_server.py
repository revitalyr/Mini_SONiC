#!/usr/bin/env python3
"""
Working WebSocket Server for Mini SONiC Demo - Simple Version
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import threading
import time
import random
from datetime import datetime
import json
import websockets
import asyncio

app = Flask(__name__)
app.config['SECRET_KEY'] = 'mini_sonic_secret'

# Enable CORS
CORS(app, resources={r"/*": {"origins": ["*"]}})

# Global state
switches = {
    1: {'name': 'Core Switch', 'status': 'Active', 'processed_packets': 0, 'l2_processed': 0, 'l3_processed': 0, 'arp_processed': 0, 'fdb_table': {}},
    2: {'name': 'Access Switch 1', 'status': 'Active', 'processed_packets': 0, 'l2_processed': 0, 'l3_processed': 0, 'arp_processed': 0, 'fdb_table': {}},
    3: {'name': 'Access Switch 2', 'status': 'Active', 'processed_packets': 0, 'l2_processed': 0, 'l3_processed': 0, 'arp_processed': 0, 'fdb_table': {}}
}

stats = {'packets_processed': 0, 'packets_lost': 0, 'throughput': 0, 'latency': 0, 'start_time': time.time()}
websocket_clients = set()

@app.route('/api/health')
def health_check():
    uptime = int(time.time() - stats['start_time'])
    return jsonify({'status': 'healthy', 'uptime': uptime})

@app.route('/api/packet', methods=['POST'])
def process_packet():
    data = request.get_json()
    packet_id = data.get('packetId')
    src_mac = data.get('srcMac')
    dst_mac = data.get('dstMac')
    packet_type = data.get('type', 'L2')
    ingress_switch = data.get('ingressSwitch')
    
    processing_time = random.randint(10, 50)
    
    try:
        if random.random() < 0.05:
            stats['packets_lost'] += 1
            return jsonify({'success': False, 'error': 'Packet dropped'})
        
        switch_data = switches.get(ingress_switch, {})
        if packet_type in ['L2', 'ARP']:
            switch_data['l2_processed'] += 1
            processing_stage = 'L2'
        elif packet_type in ['L3', 'IP']:
            switch_data['l3_processed'] += 1
            processing_stage = 'L3'
        else:
            processing_stage = 'Unknown'
        
        if packet_type == 'ARP':
            switch_data['arp_processed'] += 1
        
        if src_mac and ingress_switch:
            switch_data['fdb_table'][src_mac] = {'port': data.get('ingressPort', 'unknown'), 'timestamp': time.time()}
        
        if dst_mac in switch_data['fdb_table']:
            egress_port = switch_data['fdb_table'][dst_mac]['port']
        else:
            egress_port = 'flood'
        
        switch_data['processed_packets'] += 1
        stats['packets_processed'] += 1
        stats['throughput'] = stats['packets_processed'] / (time.time() - stats['start_time'])
        stats['latency'] = processing_time
        
        # Broadcast to WebSocket clients
        message = {
            'type': 'packet_processed',
            'packetId': packet_id,
            'egressPort': egress_port,
            'packetType': packet_type,
            'processingTime': processing_time,
            'switchId': ingress_switch
        }
        
        for client in list(websocket_clients):
            try:
                client.send(json.dumps(message))
            except:
                pass
        
        return jsonify({
            'success': True, 'egressPort': egress_port, 'packetType': packet_type,
            'processingStage': processing_stage, 'processingTime': processing_time
        })
        
    except Exception as e:
        stats['packets_lost'] += 1
        return jsonify({'success': False, 'error': str(e)})

async def websocket_handler(websocket, path):
    print('WebSocket client connected!')
    websocket_clients.add(websocket)
    
    try:
        async for message in websocket:
            pass
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        websocket_clients.discard(websocket)
        print('WebSocket client disconnected')

def run_websocket_server():
    print("Starting WebSocket server on ws://localhost:8765")
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    start_server = websockets.serve(websocket_handler, "localhost", 8765)
    loop.run_until_complete(start_server)

if __name__ == '__main__':
    print("Starting Working Mini SONiC WebSocket Server...")
    print("HTTP API: http://localhost:8080")
    print("WebSocket: ws://localhost:8765")
    
    # Start WebSocket server in background thread
    ws_thread = threading.Thread(target=run_websocket_server, daemon=True)
    ws_thread.start()
    
    # Start Flask server
    app.run(host='0.0.0.0', port=8080, debug=False, use_reloader=False)
