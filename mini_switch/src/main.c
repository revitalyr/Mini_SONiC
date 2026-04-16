#include "common.h"
#include "port.h"
#include "ring_buffer.h"
#include "forwarding.h"
#include "mac_table.h"
#include "arp.h"
#include "vlan.h"
#include "visual.h"
#include <signal.h>
#include <stdbool.h>

static volatile int running = 1;

// Global packet counters
volatile unsigned long global_rx_count = 0;
volatile unsigned long global_tx_count = 0;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

void* rx_thread(void *arg) {
    int port_idx = *(int*)arg;
    uint8_t buffer[BUFFER_SIZE];
    packet_t *pkt;
    
    printf("RX thread started for port %d (%s)\n", port_idx, ports[port_idx].name);
    
    while (running) {
        ssize_t len = recvfrom(ports[port_idx].sock, buffer, sizeof(buffer), 0, NULL, NULL);
        if (len <= 0) continue;
        
        pkt = malloc(sizeof(packet_t));
        if (!pkt) continue;
        
        memcpy(pkt->data, buffer, len);
        pkt->len = len;
        pkt->port = port_idx;
        clock_gettime(CLOCK_MONOTONIC, &pkt->timestamp);
        
        __sync_fetch_and_add(&global_rx_count, 1);
        
        if (ring_push(&global_queues.rx_queue, pkt) < 0) {
            free(pkt);
        }
    }
    
    return NULL;
}

void* worker_thread(void *arg) {
    packet_t *pkt;
    
    printf("Worker thread started\n");
    
    while (running) {
        pkt = (packet_t*)ring_pop(&global_queues.rx_queue);
        if (pkt != NULL) {
            printf("📦 [WORKER] Processing packet: %zu bytes from port %d\n", pkt->len, pkt->port);
            
            // Debug: print first few bytes
            printf("📦 [WORKER] Packet data: ");
            for (int i = 0; i < (pkt->len < 16 ? pkt->len : 16); i++) {
                printf("%02x ", pkt->data[i]);
            }
            printf("\n");
            
            forward_frame(pkt, pkt->port);
            free(pkt);
        }
    }
    
    return NULL;
}

void* cli_thread(void *arg) {
    char cmd[64];
    
    while (running) {
        printf("mini-switch> ");
        fflush(stdout);
        
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            // stdin closed or interrupted
            if (feof(stdin)) {
                printf("\nEOF received, exiting CLI...\n");
                running = 0;
                break;
            }
            continue;
        }
        
        // Remove newline
        cmd[strcspn(cmd, "\n")] = 0;
        
        if (strncmp(cmd, "show mac", 8) == 0) {
            mac_table_print_visual();
        } else if (strncmp(cmd, "show arp", 8) == 0) {
            arp_table_print_visual();
        } else if (strncmp(cmd, "help", 4) == 0) {
            printf("Commands:\n");
            printf("  show mac    - Show MAC address table\n");
            printf("  show arp    - Show ARP table\n");
            printf("  help        - Show this help\n");
            printf("  exit        - Exit mini-switch\n");
        } else if (strncmp(cmd, "exit", 4) == 0) {
            running = 0;
        } else if (strlen(cmd) > 0) {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
    printf("CLI thread exiting...\n");
    return NULL;
}

void* visual_thread(void *arg) {
    while (running) {
        sleep(5); // Show tables every 5 seconds for more frequent updates
        if (!running) break; // Check after sleep
        printf("\n=== MAC Table ===\n");
        mac_table_print_visual();
        printf("================\n\n");
        
        printf("=== ARP Table ===\n");
        arp_table_print_visual();
        printf("================\n\n");
        
        printf("=== Statistics ===\n");
        printf("RX packets: %lu\n", (unsigned long)global_rx_count);
        printf("TX packets: %lu\n", (unsigned long)global_tx_count);
        printf("==============\n\n");
    }
    printf("Visual thread exiting...\n");
    return NULL;
}

void usage(const char *prog) {
    printf("Usage: %s <interface1> [interface2] ...\n", prog);
    printf("Example: %s eth0 eth1 eth2\n", prog);
}

int main(int argc, char **argv) {
    pthread_t rx_threads[MAX_PORTS];
    pthread_t worker_thread_handle;
    pthread_t cli_thread_handle;
    pthread_t visual_thread_handle;
    int thread_args[MAX_PORTS];
    int i;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    
    // Check for visual-only mode
    bool visual_only = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--visual-only") == 0) {
            visual_only = true;
            break;
        }
    }
    
    printf("Mini Switch - L2/L3 Software Switch\n");
    printf("=====================================\n");
    
    port_init();
    mac_table_init();
    arp_init();
    vlan_init();
    forward_init();
    ring_init(&global_queues.rx_queue);
    ring_init(&global_queues.tx_queue);
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--visual-only") != 0 && port_add(argv[i]) < 0) {
            fprintf(stderr, "Failed to add interface %s\n", argv[i]);
            return 1;
        }
    }
    
    port_print();
    
    for (int i = 0; i < port_count; i++) {
        thread_args[i] = i;
        if (pthread_create(&rx_threads[i], NULL, rx_thread, &thread_args[i]) != 0) {
            perror("pthread_create rx_thread");
            return 1;
        }
    }
    
    if (pthread_create(&worker_thread_handle, NULL, worker_thread, NULL) != 0) {
        perror("pthread_create worker_thread");
        return 1;
    }
    
    // Only create CLI thread if not in visual-only mode
    if (!visual_only) {
        if (pthread_create(&cli_thread_handle, NULL, cli_thread, NULL) != 0) {
            perror("pthread_create cli_thread");
            return 1;
        }
        printf("Switch running... Press Ctrl+C to stop\n");
        printf("Type 'help' for available commands\n");
    } else {
        printf("Switch running in visual-only mode... Press Ctrl+C to stop\n");
    }
    
    if (pthread_create(&visual_thread_handle, NULL, visual_thread, NULL) != 0) {
        perror("pthread_create visual_thread");
        return 1;
    }
    
    for (int i = 0; i < port_count; i++) {
        pthread_join(rx_threads[i], NULL);
    }
    pthread_join(worker_thread_handle, NULL);
    
    // Only join CLI thread if it was created
    if (!visual_only) {
        pthread_join(cli_thread_handle, NULL);
    }
    
    pthread_join(visual_thread_handle, NULL);
    
    printf("Switch shutdown complete\n");
    return 0;
}
