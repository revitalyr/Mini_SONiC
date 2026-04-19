#!/usr/bin/env python3
"""
Mini SONiC Server Mock
Implements the server API for the switch animation demo
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
from flask_socketio import SocketIO, emit
import threading
import time
import random
from datetime import datetime

app = Flask(__name__)
app.config['SECRET_KEY'] = 'mini_sonic_secret'

# Enable CORS for all routes
CORS(app, resources={
    r"/*": {
        "origins": ["*"],
        "methods": ["GET", "POST", "OPTIONS"],
        "allow_headers": ["Content-Type", "Authorization"]
    }
})

socketio = SocketIO(app, cors_allowed_origins="*", logger=True, engineio_logger=True)

# Global state
switches = {
    1: {
        'name': 'Core Switch',
        'status': 'Active',
        'processed_packets': 0,
        'l2_processed': 0,
        'l3_processed': 0,
        'arp_processed': 0,
        'fdb_table': {},
        'routing_table': {
            '10.0.1.0/24': {'next_hop': '10.0.1.1', 'port': 'right-0'},
            '10.0.2.0/24': {'next_hop': '10.0.2.1', 'port': 'right-1'}
        }
    },
    2: {
        'name': 'Access Switch 1',
        'status': 'Active',
        'processed_packets': 0,
        'l2_processed': 0,
        'l3_processed': 0,
        'arp_processed': 0,
        'fdb_table': {},
        'routing_table': {}
    },
    3: {
        'name': 'Access Switch 2',
        'status': 'Active',
        'processed_packets': 0,
        'l2_processed': 0,
        'l3_processed': 0,
        'arp_processed': 0,
        'fdb_table': {},
        'routing_table': {}
    }
}

# Statistics
stats = {
    'packets_processed': 0,
    'packets_lost': 0,
    'throughput': 0,
    'latency': 0,
    'start_time': time.time()
}

@app.route('/api/health')
def health_check():
    """Health check endpoint"""
    uptime = int(time.time() - stats['start_time'])
    return jsonify({
        'status': 'healthy',
        'uptime': uptime,
        'timestamp': datetime.now().isoformat()
    })

@app.route('/api/packet', methods=['POST'])
def process_packet():
    """Process a packet through Mini SONiC pipeline"""
    data = request.get_json()
    
    packet_id = data.get('packetId')
    src_mac = data.get('srcMac')
    dst_mac = data.get('dstMac')
    packet_type = data.get('type', 'L2')
    ingress_switch = data.get('ingressSwitch')
    
    # Simulate processing delay
    processing_time = random.randint(10, 50)
    time.sleep(processing_time / 1000.0)
    
    # Simulate packet processing pipeline
    try:
        # Step 1: Ingress processing
        if random.random() < 0.05:  # 5% packet loss simulation
            stats['packets_lost'] += 1
            return jsonify({
                'success': False,
                'error': 'Packet dropped in ingress processing'
            })
        
        # Step 2: L2/L3 processing
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
        
        # Step 3: Update FDB table
        if src_mac and ingress_switch:
            switch_data['fdb_table'][src_mac] = {
                'port': data.get('ingressPort', 'unknown'),
                'timestamp': time.time()
            }
        
        # Step 4: Forwarding decision
        if dst_mac in switch_data['fdb_table']:
            egress_port = switch_data['fdb_table'][dst_mac]['port']
        else:
            egress_port = 'flood'
        
        # Step 5: Update statistics
        switch_data['processed_packets'] += 1
        stats['packets_processed'] += 1
        stats['throughput'] = stats['packets_processed'] / (time.time() - stats['start_time'])
        stats['latency'] = processing_time
        
        # Emit real-time update
        socketio.emit('packet_processed', {
            'packetId': packet_id,
            'egressPort': egress_port,
            'packetType': packet_type,
            'processingTime': processing_time,
            'switchId': ingress_switch
        })
        
        return jsonify({
            'success': True,
            'egressPort': egress_port,
            'packetType': packet_type,
            'processingStage': processing_stage,
            'processingTime': processing_time,
            'fdbUpdate': {
                'mac': src_mac,
                'entry': switch_data['fdb_table'].get(src_mac) if src_mac else None
            }
        })
        
    except Exception as e:
        stats['packets_lost'] += 1
        return jsonify({
            'success': False,
            'error': str(e)
        })

@app.route('/api/switch/<int:switch_id>/status')
def get_switch_status(switch_id):
    """Get switch status"""
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    
    return jsonify(switch_data)

@app.route('/api/switch/<int:switch_id>/fdb')
def get_fdb_table(switch_id):
    """Get FDB table for switch"""
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    
    fdb_list = []
    for mac, entry in switch_data['fdb_table'].items():
        fdb_list.append({
            'mac': mac,
            'port': entry['port'],
            'timestamp': entry['timestamp']
        })
    
    return jsonify(fdb_list)

@app.route('/api/switch/<int:switch_id>/routing')
def get_routing_table(switch_id):
    """Get routing table for switch"""
    switch_data = switches.get(switch_id)
    if not switch_data:
        return jsonify({'error': 'Switch not found'}), 404
    
    routing_list = []
    for prefix, route in switch_data['routing_table'].items():
        routing_list.append({
            'prefix': prefix,
            'next_hop': route['next_hop'],
            'port': route['port']
        })
    
    return jsonify(routing_list)

@app.route('/api/stats')
def get_stats():
    """Get global statistics"""
    return jsonify(stats)

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print('Client connected to Mini SONiC server')
    emit('connected', {'message': 'Connected to Mini SONiC server'})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print('Client disconnected from Mini SONiC server')

def background_stats_updater():
    """Background thread to update statistics"""
    while True:
        time.sleep(5)  # Update every 5 seconds
        
        # Calculate current throughput
        current_time = time.time()
        elapsed = current_time - stats['start_time']
        if elapsed > 0:
            stats['throughput'] = stats['packets_processed'] / elapsed
            stats['latency'] = random.randint(5, 25)  # Simulated latency
        
        # Emit stats update
        socketio.emit('stats_update', stats)
        
        # Emit switch status updates
        for switch_id, switch_data in switches.items():
            socketio.emit('switch_status', {
                'switchId': switch_id,
                'status': switch_data['status'],
                'processed_packets': switch_data['processed_packets'],
                'l2_processed': switch_data['l2_processed'],
                'l3_processed': switch_data['l3_processed'],
                'arp_processed': switch_data['arp_processed']
            })

if __name__ == '__main__':
    print("Starting Mini SONiC Server Mock...")
    print("API Endpoints:")
    print("  GET  /api/health")
    print("  POST /api/packet")
    print("  GET  /api/switch/<id>/status")
    print("  GET  /api/switch/<id>/fdb")
    print("  GET  /api/switch/<id>/routing")
    print("  GET  /api/stats")
    print("WebSocket: ws://localhost:8080/ws")
    print()
    
    # Start background thread
    stats_thread = threading.Thread(target=background_stats_updater, daemon=True)
    stats_thread.start()
    
    # Run server
    socketio.run(app, host='0.0.0.0', port=8080, debug=False)
