#include "dashboard.h"
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

static WINDOW *main_win;
static WINDOW *stats_win;
static WINDOW *mac_win;
static WINDOW *arp_win;
static WINDOW *packets_win;
static int max_y, max_x;

// Colors
enum {
    COLOR_HEADER = 1,
    COLOR_BORDER = 2,
    COLOR_MAC = 3,
    COLOR_ARP = 4,
    COLOR_PACKET = 5,
    COLOR_SUCCESS = 6,
    COLOR_FLOOD = 7
};

void dashboard_init(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    start_color();
    
    // Define color pairs
    init_pair(COLOR_HEADER, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_BORDER, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_MAC, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_ARP, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PACKET, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_FLOOD, COLOR_MAGENTA, COLOR_BLACK);
    
    getmaxyx(stdscr, max_y, max_x);
    
    // Create windows
    main_win = newwin(max_y, max_x, 0, 0);
    stats_win = derwin(main_win, 8, 30, 2, 2);
    mac_win = derwin(main_win, 10, 35, 2, 33);
    arp_win = derwin(main_win, 10, 35, 12, 33);
    packets_win = derwin(main_win, max_y - 15, max_x - 4, 2, 69);
    
    keypad(main_win, TRUE);
    nodelay(main_win, TRUE);
    wrefresh(main_win);
}

void dashboard_cleanup(void) {
    delwin(stats_win);
    delwin(mac_win);
    delwin(arp_win);
    delwin(packets_win);
    delwin(main_win);
    endwin();
}

void dashboard_draw_border(WINDOW *win, const char *title) {
    box(win, 0, 0);
    wattron(win, COLOR_PAIR(COLOR_HEADER));
    mvwprintw(win, 0, 2, " %s ", title);
    wattroff(win, COLOR_PAIR(COLOR_HEADER));
}

void dashboard_update_stats(unsigned long rx_count, unsigned long tx_count, 
                      unsigned long mac_entries, unsigned long arp_entries) {
    werase(stats_win);
    dashboard_draw_border(stats_win, "📊 Statistics");
    
    wattron(stats_win, COLOR_PAIR(COLOR_SUCCESS));
    mvwprintw(stats_win, 2, 2, "RX Packets: %lu", rx_count);
    mvwprintw(stats_win, 3, 2, "TX Packets: %lu", tx_count);
    wattroff(stats_win, COLOR_PAIR(COLOR_SUCCESS));
    
    mvwprintw(stats_win, 5, 2, "MAC Entries: %lu", mac_entries);
    mvwprintw(stats_win, 6, 2, "ARP Entries: %lu", arp_entries);
    
    wrefresh(stats_win);
}

void dashboard_update_mac_table(uint8_t macs[][6], int ports[], int count) {
    werase(mac_win);
    dashboard_draw_border(mac_win, "🔗 MAC Table");
    
    wattron(mac_win, COLOR_PAIR(COLOR_MAC));
    mvwprintw(mac_win, 1, 2, "MAC Address        Port");
    mvwhline(mac_win, 2, 1, '-', 33);
    wattroff(mac_win, COLOR_PAIR(COLOR_MAC));
    
    for (int i = 0; i < count && i < 8; i++) {
        mvwprintw(mac_win, 3 + i, 2, "%02X:%02X:%02X:%02X:%02X:%02X  %d",
                 macs[i][0], macs[i][1], macs[i][2],
                 macs[i][3], macs[i][4], macs[i][5], ports[i]);
    }
    
    wrefresh(mac_win);
}

void dashboard_update_arp_table(uint32_t ips[], uint8_t macs[][6], int count) {
    werase(arp_win);
    dashboard_draw_border(arp_win, "🌐 ARP Table");
    
    wattron(arp_win, COLOR_PAIR(COLOR_ARP));
    mvwprintw(arp_win, 1, 2, "IP Address      MAC Address");
    mvwhline(arp_win, 2, 1, '-', 33);
    wattroff(arp_win, COLOR_PAIR(COLOR_ARP));
    
    for (int i = 0; i < count && i < 8; i++) {
        struct in_addr addr;
        addr.s_addr = ips[i];
        mvwprintw(arp_win, 3 + i, 2, "%-15s %02X:%02X:%02X:%02X:%02X:%02X",
                 inet_ntoa(addr),
                 macs[i][0], macs[i][1], macs[i][2],
                 macs[i][3], macs[i][4], macs[i][5]);
    }
    
    wrefresh(arp_win);
}

static char packet_history[20][256];
static int packet_count = 0;
static int packet_start = 0;

void dashboard_add_packet(const char *packet_info) {
    strncpy(packet_history[packet_start], packet_info, 255);
    packet_history[packet_start][255] = '\0';
    packet_start = (packet_start + 1) % 20;
    if (packet_count < 20) packet_count++;
}

void dashboard_update_packets(void) {
    werase(packets_win);
    dashboard_draw_border(packets_win, "📦 Live Packets");
    
    int display_count = packet_count < 20 ? packet_count : 20;
    int start_idx = packet_count < 20 ? 0 : packet_start;
    
    for (int i = 0; i < display_count; i++) {
        int idx = (start_idx + i) % 20;
        char *packet = packet_history[idx];
        
        // Color based on packet type
        if (strstr(packet, "🟢")) {
            wattron(packets_win, COLOR_PAIR(COLOR_SUCCESS));
        } else if (strstr(packet, "🟡")) {
            wattron(packets_win, COLOR_PAIR(COLOR_FLOOD));
        } else if (strstr(packet, "🔵")) {
            wattron(packets_win, COLOR_PAIR(COLOR_ARP));
        } else {
            wattron(packets_win, COLOR_PAIR(COLOR_PACKET));
        }
        
        mvwprintw(packets_win, 1 + i, 2, "%s", packet);
        wattroff(packets_win, COLOR_PAIR(COLOR_SUCCESS) | COLOR_PAIR(COLOR_FLOOD) | 
                                      COLOR_PAIR(COLOR_ARP) | COLOR_PAIR(COLOR_PACKET));
    }
    
    wrefresh(packets_win);
}

void dashboard_refresh(void) {
    // Draw main border
    werase(main_win);
    box(main_win, 0, 0);
    
    // Title
    wattron(main_win, COLOR_PAIR(COLOR_HEADER));
    mvwprintw(main_win, 0, (max_x - 20) / 2, " 🌟 Mini-Switch Dashboard 🌟 ");
    wattroff(main_win, COLOR_PAIR(COLOR_HEADER));
    
    // Status bar
    time_t now = time(NULL);
    mvwprintw(main_win, max_y - 1, 2, " Press 'q' to quit | %s", ctime(&now));
    
    wrefresh(main_win);
}

int dashboard_should_exit(void) {
    int ch = wgetch(main_win);
    return ch == 'q' || ch == 'Q' || ch == 27; // ESC
}

void dashboard_clear_packets(void) {
    packet_count = 0;
    packet_start = 0;
    memset(packet_history, 0, sizeof(packet_history));
}
