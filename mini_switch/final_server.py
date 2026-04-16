#!/usr/bin/env python3
"""
Final Working Mini SONiC Server
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import threading
import time
import random
from datetime import datetime

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
        
        return jsonify({
            'success': True, 'egressPort': egress_port, 'packetType': packet_type,
            'processingStage': processing_stage, 'processingTime': processing_time
        })
        
    except Exception as e:
        stats['packets_lost'] += 1
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/switch/<int:switch_id>/status')
def get_switch_status(switch_id):
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    return jsonify(switch_data)

@app.route('/api/switch/<int:switch_id>/fdb')
def get_fdb_table(switch_id):
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    return jsonify(list(switch_data['fdb_table'].items()))

@app.route('/api/switch/<int:switch_id>/routing')
def get_routing_table(switch_id):
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    return jsonify({})

@app.route('/api/stats')
def get_stats():
    return jsonify(stats)

if __name__ == '__main__':
    print("Starting Final Mini SONiC Server...")
    print("HTTP API: http://localhost:8080")
    print("WebSocket: DISABLED (using HTTP polling fallback)")
    print()
    app.run(host='0.0.0.0', port=8080, debug=False)
