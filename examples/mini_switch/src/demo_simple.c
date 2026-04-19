// Simple Mini Switch Demo - Windows Compatible
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#define BUFFER_SIZE 2048
#define MAX_PORTS 8

// Packet structure
typedef struct {
    uint8_t data[BUFFER_SIZE];
    size_t len;
    int port;
    DWORD timestamp;
    char src_mac[18];
    char dst_mac[18];
    char type[8];
} packet_t;

// Switch structure
typedef struct {
    char name[64];
    int active;
    int processed_packets;
    int l2_processed;
    int l3_processed;
    int arp_processed;
} switch_t;

// Global variables
switch_t switches[3];
packet_t packet_buffer[1024];
int packet_count = 0;
HANDLE hConsole;

// Console colors
#define COLOR_RED     12
#define COLOR_GREEN   10
#define COLOR_YELLOW  14
#define COLOR_BLUE    9
#define COLOR_WHITE   15
#define COLOR_CYAN    11

void set_console_color(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void print_header() {
    set_console_color(COLOR_BLUE);
    printf("\n");
    printf("=================================================================\n");
    printf("                    Mini SONiC Switch Demo\n");
    printf("                    Windows-Compatible Version\n");
    printf("=================================================================\n\n");
    set_console_color(COLOR_WHITE);
}

void print_status(const char* message, int color) {
    set_console_color(color);
    printf("[%s] %s\n", message, color == COLOR_GREEN ? "OK" : color == COLOR_RED ? "ERROR" : "INFO");
    set_console_color(COLOR_WHITE);
}

void init_switches() {
    set_console_color(COLOR_YELLOW);
    printf("Initializing Mini SONiC switches...\n");
    
    strcpy(switches[0].name, "Core Switch");
    strcpy(switches[1].name, "Access Switch 1");
    strcpy(switches[2].name, "Access Switch 2");
    
    for (int i = 0; i < 3; i++) {
        switches[i].active = 1;
        switches[i].processed_packets = 0;
        switches[i].l2_processed = 0;
        switches[i].l3_processed = 0;
        switches[i].arp_processed = 0;
        printf("  - %s initialized\n", switches[i].name);
    }
    
    set_console_color(COLOR_WHITE);
    printf("\n");
}

void create_packet(packet_t* packet, int id) {
    const char* packet_types[] = {"L2", "L3", "ARP", "IP"};
    const char* src_macs[] = {"00:11:22:33:44:55", "AA:BB:CC:DD:EE:FF", "11:22:33:44:55:66"};
    const char* dst_macs[] = {"FF:EE:DD:CC:BB:AA", "66:55:44:33:22:11", "BB:AA:99:88:77:66"};
    
    int type_idx = id % 4;
    int src_idx = id % 3;
    int dst_idx = (id + 1) % 3;
    
    packet->len = 64 + (id * 16);
    packet->port = id % 3;
    packet->timestamp = GetTickCount();
    strcpy(packet->src_mac, src_macs[src_idx]);
    strcpy(packet->dst_mac, dst_macs[dst_idx]);
    strcpy(packet->type, packet_types[type_idx]);
    
    snprintf((char*)packet->data, BUFFER_SIZE, 
            "Packet#%d Type:%s SRC:%s DST:%s Size:%zu", 
            id + 1, packet_types[type_idx], src_macs[src_idx], 
            dst_macs[dst_idx], packet->len);
}

void process_packet_on_switch(packet_t* packet, int switch_idx) {
    printf("Processing packet on %s:\n", switches[switch_idx].name);
    printf("  Type: %s\n", packet->type);
    printf("  Source: %s\n", packet->src_mac);
    printf("  Destination: %s\n", packet->dst_mac);
    printf("  Size: %zu bytes\n", packet->len);
    
    // Simulate processing delay
    Sleep(50 + (packet_count * 10));
    
    // Update statistics
    switches[switch_idx].processed_packets++;
    if (strcmp(packet->type, "L2") == 0) switches[switch_idx].l2_processed++;
    else if (strcmp(packet->type, "L3") == 0) switches[switch_idx].l3_processed++;
    else if (strcmp(packet->type, "ARP") == 0) switches[switch_idx].arp_processed++;
    
    set_console_color(COLOR_GREEN);
    printf("  -> Packet processed successfully\n\n");
    set_console_color(COLOR_WHITE);
}

void simulate_packet_processing() {
    set_console_color(COLOR_GREEN);
    printf("Starting Mini SONiC packet processing simulation...\n\n");
    set_console_color(COLOR_WHITE);
    
    // Process 10 packets
    for (int i = 0; i < 10; i++) {
        packet_t packet;
        create_packet(&packet, i);
        packet_buffer[packet_count++] = packet;
        
        int switch_idx = i % 3;
        process_packet_on_switch(&packet, switch_idx);
    }
}

void print_statistics() {
    set_console_color(COLOR_BLUE);
    printf("Mini SONiC Processing Statistics:\n");
    printf("================================\n\n");
    set_console_color(COLOR_WHITE);
    
    int total_packets = 0;
    int total_l2 = 0, total_l3 = 0, total_arp = 0;
    
    for (int i = 0; i < 3; i++) {
        printf("%s:\n", switches[i].name);
        printf("  Total packets: %d\n", switches[i].processed_packets);
        printf("  L2 packets: %d\n", switches[i].l2_processed);
        printf("  L3 packets: %d\n", switches[i].l3_processed);
        printf("  ARP packets: %d\n", switches[i].arp_processed);
        printf("  Status: %s\n", switches[i].active ? "Active" : "Inactive");
        printf("\n");
        
        total_packets += switches[i].processed_packets;
        total_l2 += switches[i].l2_processed;
        total_l3 += switches[i].l3_processed;
        total_arp += switches[i].arp_processed;
    }
    
    printf("Network Totals:\n");
    printf("  Total packets processed: %d\n", total_packets);
    printf("  L2 packets: %d\n", total_l2);
    printf("  L3 packets: %d\n", total_l3);
    printf("  ARP packets: %d\n", total_arp);
    printf("  Packet buffer usage: %d/1024\n", packet_count);
    
    DWORD end_time = GetTickCount();
    printf("  Simulation time: %lu ms\n", end_time - packet_buffer[0].timestamp);
}

void print_architecture_info() {
    set_console_color(COLOR_CYAN);
    printf("Mini SONiC Architecture Components:\n");
    printf("==================================\n\n");
    set_console_color(COLOR_WHITE);
    
    printf("Data Flow Pipeline:\n");
    printf("  1. Input Sources\n");
    printf("     - Packet Generator (Test Traffic)\n");
    printf("     - TCP Link (Inter-switch Traffic)\n");
    printf("     - External Interface (Network Packets)\n\n");
    
    printf("  2. Ingress Processing\n");
    printf("     - SPSC Queue (Lock-free Buffer)\n");
    printf("     - Pipeline Thread (Async Processing)\n");
    printf("     - Packet Parsing (Header Extraction)\n\n");
    
    printf("  3. Control Plane\n");
    printf("     - L2 Service (MAC Learning)\n");
    printf("     - L3 Service (Route Lookup)\n");
    printf("     - Event Loop (Coordination)\n\n");
    
    printf("  4. Decision Logic\n");
    printf("     - MAC Table Lookup (Destination MAC)\n");
    printf("     - LPM Lookup (IP Prefix Match)\n");
    printf("     - Forwarding Decision (Egress Port)\n\n");
    
    printf("  5. Egress Processing\n");
    printf("     - SAI Operations (Hardware Abstraction)\n");
    printf("     - Packet Forwarding (Send to Output)\n");
    printf("     - Statistics Update (Counters & Metrics)\n\n");
    
    printf("  6. Output Destinations\n");
    printf("     - TCP Link Out (To Other Switches)\n");
    printf("     - Local Delivery (Host Processing)\n");
    printf("     - Drop/Discard (Filtered Packets)\n\n");
    
    set_console_color(COLOR_GREEN);
    printf("Architecture simulation complete!\n");
    set_console_color(COLOR_WHITE);
}

int main() {
    // Initialize console
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    print_header();
    
    // Initialize switches
    init_switches();
    print_status("Mini SONiC switches initialized", COLOR_GREEN);
    
    // Show architecture
    print_architecture_info();
    print_status("Architecture components displayed", COLOR_GREEN);
    
    // Simulate packet processing
    simulate_packet_processing();
    print_status("Packet processing simulation complete", COLOR_GREEN);
    
    // Print statistics
    print_statistics();
    
    set_console_color(COLOR_GREEN);
    printf("\nMini SONiC Demo completed successfully!\n");
    printf("=====================================\n");
    set_console_color(COLOR_WHITE);
    
    return 0;
}
