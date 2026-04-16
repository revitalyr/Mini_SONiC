/**
 * @file realtime_demo.c
 * @brief Real-time Data Flow Demo for SONiC Mini Switch
 * 
 * This demo shows real packet processing with actual data flow
 * between switches, demonstrating how data moves through
 * the system in real-time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "cross_platform.h"
#include "../include/constants.h"

// Simple packet structure
typedef struct {
    uint8_t src_mac[6];
    uint8_t dst_mac[6];
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t type;
    uint32_t id;
    uint32_t size;
    uint64_t timestamp;
} packet_t;

// Simple switch structure
typedef struct {
    int id;
    char name[32];
    char mac_address[18];
    int ingress_ports[8];
    int egress_ports[8];
    int port_count;
    int active;
    uint64_t packets_processed;
    uint64_t packets_dropped;
    uint64_t bytes_processed;
    double last_activity;
} switch_t;

// Global variables
static switch_t switches[4];
static packet_t packet_buffer[1000];
static int packet_count = 0;
static volatile int running = 1;

// Function prototypes
void init_switches(void);
void generate_packet(packet_t *packet, switch_t *src_switch, switch_t *dst_switch);
void process_packet(switch_t *sw, packet_t *packet);
void print_packet_info(const packet_t *packet);
void print_statistics(void);
void cleanup_switches(void);

// Initialize switches
void init_switches(void) {
    // Switch 1 - Core Switch
    strcpy(switches[0].name, SWITCH_NAME_CORE);
    sprintf(switches[0].mac_address, "00:11:22:33:44:55");
    switches[0].id = 1;
    switches[0].port_count = 2;
    switches[0].ingress_ports[0] = 1;
    switches[0].egress_ports[0] = 2;
    switches[0].active = 0;
    switches[0].packets_processed = 0;
    switches[0].packets_dropped = 0;
    switches[0].bytes_processed = 0;
    switches[0].last_activity = time(NULL);
    
    // Switch 2 - Aggregation Switch
    strcpy(switches[1].name, SWITCH_NAME_AGGREGATION);
    sprintf(switches[1].mac_address, "00:11:22:33:44:66");
    switches[1].id = 2;
    switches[1].port_count = 2;
    switches[1].ingress_ports[0] = 3;
    switches[1].ingress_ports[1] = 4;
    switches[1].egress_ports[0] = 1;
    switches[1].egress_ports[1] = 2;
    switches[1].active = 0;
    switches[1].packets_processed = 0;
    switches[1].packets_dropped = 0;
    switches[1].bytes_processed = 0;
    switches[1].last_activity = time(NULL);
    
    // Switch 3 - Edge Switch
    strcpy(switches[2].name, SWITCH_NAME_EDGE);
    sprintf(switches[2].mac_address, "00:11:22:33:44:77");
    switches[2].id = 3;
    switches[2].port_count = 2;
    switches[2].ingress_ports[0] = 5;
    switches[2].ingress_ports[1] = 6;
    switches[2].egress_ports[0] = 3;
    switches[2].egress_ports[1] = 4;
    switches[2].active = 0;
    switches[2].packets_processed = 0;
    switches[2].packets_dropped = 0;
    switches[2].bytes_processed = 0;
    switches[2].last_activity = time(NULL);
    
    // Switch 4 - Access Switch
    strcpy(switches[3].name, SWITCH_NAME_ACCESS);
    sprintf(switches[3].mac_address, "00:11:22:33:44:88");
    switches[3].id = 4;
    switches[3].port_count = 2;
    switches[3].ingress_ports[0] = 7;
    switches[3].ingress_ports[1] = 8;
    switches[3].egress_ports[0] = 5;
    switches[3].egress_ports[1] = 6;
    switches[3].active = 0;
    switches[3].packets_processed = 0;
    switches[3].packets_dropped = 0;
    switches[3].bytes_processed = 0;
    switches[3].last_activity = time(NULL);
}

// Generate random MAC address
void random_mac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = rand() % 256;
    }
}

// Generate random packet
void generate_packet(packet_t *packet, switch_t *src_switch, switch_t *dst_switch) {
    static uint32_t packet_id = 0;
    
    // Set source and destination
    random_mac(packet->src_mac);
    random_mac(packet->dst_mac);
    
    // Random source and destination ports
    packet->src_port = src_switch->ingress_ports[rand() % src_switch->port_count];
    packet->dst_port = dst_switch->egress_ports[rand() % dst_switch->port_count];
    
    // Random packet type (70% L2, 20% L3, 10% ARP)
    int type_rand = rand() % 100;
    if (type_rand < 70) {
        packet->type = 0; // L2
    } else if (type_rand < 90) {
        packet->type = 1; // L3
    } else {
        packet->type = 2; // ARP
    }
    
    // Random packet size (64-1518 bytes)
    packet->size = 64 + (rand() % (1518 - 64));
    
    // Set timestamp
    packet->timestamp = time(NULL);
    packet->id = ++packet_id;
}

// Process packet through switch
void process_packet(switch_t *sw, packet_t *packet) {
    // Update statistics
    sw->packets_processed++;
    sw->bytes_processed += packet->size;
    sw->last_activity = time(NULL);
    
    // Simple processing: learn source MAC and forward
    // In real SONiC, this would involve FDB updates, route lookups, etc.
    // For demo, we just log the packet
    
    const char* type_str = (packet->type == 0) ? PACKET_TYPE_L2 : (packet->type == 1 ? PACKET_TYPE_L3 : PACKET_TYPE_ARP);
    printf("Switch %d: %s packet %u from port %d to port %d (type: %s, size: %u bytes)\n",
           sw->id, sw->name, packet->id, packet->src_port, packet->dst_port,
           type_str, packet->size);
    
    print_packet_info(packet);
}

// Print packet information
void print_packet_info(const packet_t *packet) {
    printf("  Src MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           packet->src_mac[0], packet->src_mac[1], packet->src_mac[2],
           packet->src_mac[3], packet->src_mac[4], packet->src_mac[5]);

    printf("  Dst MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           packet->dst_mac[0], packet->dst_mac[1], packet->dst_mac[2],
           packet->dst_mac[3], packet->dst_mac[4], packet->dst_mac[5]);

    const char* type_str = (packet->type == 0) ? PACKET_TYPE_L2 : (packet->type == 1 ? PACKET_TYPE_L3 : PACKET_TYPE_ARP);
    printf("  Type: %s\n", type_str);
    printf("  Size: %u bytes\n", packet->size);
    printf("  ID: %u\n", packet->id);
    printf("  Time: %llu\n", (unsigned long long)packet->timestamp);
}

// Print system statistics
void print_statistics(void) {
    printf("\n=== System Statistics ===\n");
    printf("Total packets processed: %lu\n", (unsigned long)packet_count);
    
    uint64_t total_processed = 0;
    uint64_t total_dropped = 0;
    
    for (int i = 0; i < 4; i++) {
        total_processed += switches[i].packets_processed;
        total_dropped += switches[i].packets_dropped;
    }
    
    printf("Total packets dropped: %lu\n", total_dropped);
    printf("Total bytes processed: %lu\n", total_processed * 100); // Assume average 100 bytes
    printf("Packet loss rate: %.2f%%\n", total_dropped * 100.0 / total_processed);
    
    printf("\nPer Switch:\n");
    for (int i = 0; i < 4; i++) {
        printf("  %s: %lu processed, %lu dropped (%.1f%% loss)\n",
               switches[i].name, switches[i].packets_processed, switches[i].packets_dropped,
               switches[i].packets_dropped * 100.0 / switches[i].packets_processed);
    }
}

// Cleanup resources
void cleanup_switches(void) {
    for (int i = 0; i < 4; i++) {
        switches[i].active = 0;
    }
    running = 0;
}

int main() {
    printf("=== SONiC Mini Switch - Real-time Data Flow Demo ===\n");
    printf("Starting real-time packet processing simulation...\n\n");
    
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize switches
    init_switches();
    
    // Activate all switches
    for (int i = 0; i < 4; i++) {
        switches[i].active = 1;
        printf("Switch %d (%s) activated\n", switches[i].id, switches[i].name);
    }
    
    printf("\nPress Ctrl+C to stop...\n\n");
    
    // Main processing loop
    while (running) {
        // Generate packets for each switch
        for (int i = 0; i < 4; i++) {
            if (!switches[i].active) continue;
            
            // Generate 1-3 packets per cycle
            int packets_to_generate = 1 + (rand() % 3);
            for (int j = 0; j < packets_to_generate; j++) {
                generate_packet(&packet_buffer[packet_count++ % 1000], &switches[i], &switches[(i + 1) % 4]);
                
                // Process packet through source switch
                process_packet(&switches[i], &packet_buffer[packet_count - 1]);
                
                // Process packet through destination switch
                process_packet(&switches[(i + 1) % 4], &packet_buffer[packet_count - 1]);
            }
        }
        
        // Print statistics every 1000 packets
        if (packet_count % 1000 == 0) {
            print_statistics();
        }
        
        // Small delay to make output readable
        SLEEP_MS(100);
        
        // Check for exit condition (Ctrl+C)
        #ifdef _WIN32
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            printf("\n\nStopping...\n");
            running = 0;
        }
        #else
        if (getchar() == 27) { // ESC key
            printf("\n\nStopping...\n");
            running = 0;
        }
        #endif
    }
    
    printf("\n=== Simulation Complete ===\n");
    print_statistics();
    
    cleanup_switches();
    
    return 0;
}
