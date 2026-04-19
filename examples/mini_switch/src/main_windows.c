// Windows-compatible Mini Switch Demo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 2048
#define MAX_PORTS 8

// Simplified packet structure for Windows demo
typedef struct {
    uint8_t data[BUFFER_SIZE];
    size_t len;
    int port;
    DWORD timestamp;
} packet_t;

// Simplified switch structure
typedef struct {
    char name[64];
    int active;
    int processed_packets;
} switch_t;

// Global variables
switch_t switches[MAX_PORTS];
packet_t packet_buffer[1024];
int packet_count = 0;
HANDLE hConsole;

// ANSI color codes for Windows console
#define COLOR_RED     12
#define COLOR_GREEN   10
#define COLOR_YELLOW  14
#define COLOR_BLUE    9
#define COLOR_WHITE   15
#define COLOR_GRAY    8

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
        printf("  - %s initialized\n", switches[i].name);
    }
    
    set_console_color(COLOR_WHITE);
    printf("\n");
}

void simulate_packet_processing() {
    set_console_color(COLOR_GREEN);
    printf("Starting packet processing simulation...\n\n");
    set_console_color(COLOR_WHITE);
    
    // Simulate different packet types
    const char* packet_types[] = {"L2", "L3", "ARP", "IP"};
    const char* src_macs[] = {"00:11:22:33:44:55", "AA:BB:CC:DD:EE:FF", "11:22:33:44:55:66"};
    const char* dst_macs[] = {"FF:EE:DD:CC:BB:AA", "66:55:44:33:22:11", "BB:AA:99:88:77:66"};
    
    for (int i = 0; i < 10; i++) {
        int switch_idx = i % 3;
        int packet_type_idx = i % 4;
        int src_mac_idx = i % 3;
        int dst_mac_idx = (i + 1) % 3;
        
        // Create packet
        packet_t packet;
        packet.len = 64 + (i * 16); // Variable packet size
        packet.port = switch_idx;
        packet.timestamp = GetTickCount();
        
        // Store packet info in data buffer (simplified)
        snprintf((char*)packet.data, BUFFER_SIZE, 
                "Type:%s SRC:%s DST:%s Size:%zu", 
                packet_types[packet_type_idx],
                src_macs[src_mac_idx], 
                dst_macs[dst_mac_idx],
                packet.len);
        
        packet_buffer[packet_count++] = packet;
        
        // Process packet
        printf("Processing packet %d on %s:\n", i + 1, switches[switch_idx].name);
        printf("  Type: %s\n", packet_types[packet_type_idx]);
        printf("  Source: %s\n", src_macs[src_mac_idx]);
        printf("  Destination: %s\n", dst_macs[dst_mac_idx]);
        printf("  Size: %zu bytes\n", packet.len);
        
        // Simulate processing delay
        Sleep(50 + (i * 10));
        
        switches[switch_idx].processed_packets++;
        
        set_console_color(COLOR_GREEN);
        printf("  -> Packet processed successfully\n\n");
        set_console_color(COLOR_WHITE);
    }
}

void print_statistics() {
    set_console_color(COLOR_BLUE);
    printf("Processing Statistics:\n");
    printf("======================\n\n");
    set_console_color(COLOR_WHITE);
    
    int total_packets = 0;
    for (int i = 0; i < 3; i++) {
        printf("%s:\n", switches[i].name);
        printf("  Packets processed: %d\n", switches[i].processed_packets);
        printf("  Status: %s\n", switches[i].active ? "Active" : "Inactive");
        printf("\n");
        total_packets += switches[i].processed_packets;
    }
    
    printf("Total packets processed: %d\n", total_packets);
    printf("Packet buffer usage: %d/1024\n", packet_count);
    
    DWORD end_time = GetTickCount();
    printf("Simulation time: %lu ms\n", end_time - packet_buffer[0].timestamp);
}

void simulate_architecture_flow() {
    set_console_color(COLOR_YELLOW);
    printf("Mini SONiC Architecture Flow:\n");
    printf("=============================\n\n");
    set_console_color(COLOR_WHITE);
    
    printf("Packet Processing Pipeline:\n");
    printf("  1. Input Sources (Packet Generator)\n");
    printf("  2. Ingress Processing (SPSC Queue)\n");
    printf("  3. Pipeline Thread (Async Processing)\n");
    printf("  4. Packet Parsing (Header Extraction)\n");
    printf("  5. Control Plane (L2/L3 Services)\n");
    printf("  6. Decision Logic (MAC/LPM Lookup)\n");
    printf("  7. Egress Processing (SAI Operations)\n");
    printf("  8. Output Destinations\n\n");
    
    set_console_color(COLOR_GREEN);
    printf("Pipeline simulation complete!\n");
    set_console_color(COLOR_WHITE);
}

int main() {
    // Initialize console
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    print_header();
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        print_status("WSAStartup failed", COLOR_RED);
        return 1;
    }
    
    print_status("Winsock initialized", COLOR_GREEN);
    
    // Initialize switches
    init_switches();
    print_status("Switches initialized", COLOR_GREEN);
    
    // Simulate architecture flow
    simulate_architecture_flow();
    
    // Simulate packet processing
    simulate_packet_processing();
    print_status("Packet processing complete", COLOR_GREEN);
    
    // Print statistics
    print_statistics();
    
    // Cleanup
    WSACleanup();
    
    set_console_color(COLOR_GREEN);
    printf("\nDemo completed successfully!\n");
    set_console_color(COLOR_WHITE);
    
    return 0;
}
