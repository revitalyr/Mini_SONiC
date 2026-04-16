#include "common.h"
#include "port.h"
#include "mac_table.h"
#include "arp.h"
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

static volatile int dashboard_running = 1;

void* dashboard_thread(void *arg) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLUE);
    
    while (dashboard_running) {
        clear();
        
        // Title
        attron(COLOR_PAIR(4));
        mvprintw(0, 2, "🌟 Mini-Switch Real-Time Dashboard 🌟");
        attroff(COLOR_PAIR(4));
        
        // Statistics
        mvprintw(2, 2, "📊 Statistics:");
        attron(COLOR_PAIR(1));
        mvprintw(3, 4, "RX: %lu  TX: %lu", global_rx_count, global_tx_count);
        attroff(COLOR_PAIR(1));
        
        // MAC table
        mvprintw(5, 2, "🔗 MAC Table:");
        uint8_t macs[16][6];
        int ports[16];
        int mac_count = 0;
        mac_table_get_entries(macs, ports, &mac_count);
        
        attron(COLOR_PAIR(2));
        for (int i = 0; i < mac_count && i < 8; i++) {
            mvprintw(6 + i, 4, "Port %d: %02X:%02X:%02X:%02X:%02X:%02X", 
                     ports[i], macs[i][0], macs[i][1], macs[i][2],
                     macs[i][3], macs[i][4], macs[i][5]);
        }
        attroff(COLOR_PAIR(2));
        
        // ARP table
        mvprintw(15, 2, "🌐 ARP Table:");
        uint32_t arp_ips[16];
        uint8_t arp_macs[16][6];
        int arp_count = 0;
        arp_table_get_entries(arp_ips, arp_macs, &arp_count);
        
        attron(COLOR_PAIR(3));
        for (int i = 0; i < arp_count && i < 8; i++) {
            struct in_addr addr;
            addr.s_addr = arp_ips[i];
            mvprintw(16 + i, 4, "%-15s -> %02X:%02X:%02X:%02X:%02X:%02X", 
                     inet_ntoa(addr), arp_macs[i][0], arp_macs[i][1], arp_macs[i][2],
                     arp_macs[i][3], arp_macs[i][4], arp_macs[i][5]);
        }
        attroff(COLOR_PAIR(3));
        
        // Instructions
        mvprintw(LINES-1, 2, "Press 'q' to quit | Updates every 100ms");
        
        refresh();
        
        // Check for quit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            dashboard_running = 0;
            break;
        }
        
        usleep(100000); // 100ms
    }
    
    endwin();
    return NULL;
}

// Initialize simple dashboard mode
void visual_init_simple_dashboard(void) {
    pthread_t dashboard_thread_handle;
    
    if (pthread_create(&dashboard_thread_handle, NULL, dashboard_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create dashboard thread\n");
        return;
    }
    
    printf("Simple dashboard initialized. Press 'q' in dashboard to quit.\n");
    pthread_join(dashboard_thread_handle, NULL);
    printf("Dashboard closed.\n");
}
