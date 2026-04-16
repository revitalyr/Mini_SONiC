#include "common.h"
#include "port.h"

port_t ports[MAX_PORTS];
int port_count = 0;

int port_init(void) {
    memset(ports, 0, sizeof(ports));
    port_count = 0;
    return 0;
}

int port_add(const char *ifname) {
    if (port_count >= MAX_PORTS) {
        fprintf(stderr, "Maximum number of ports reached\n");
        return -1;
    }
    
    port_t *p = &ports[port_count];
    
    strncpy(p->name, ifname, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    
    p->sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (p->sock < 0) {
        perror("socket");
        return -1;
    }
    
    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(p->sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        close(p->sock);
        return -1;
    }
    p->ifindex = ifr.ifr_ifindex;
    
    if (ioctl(p->sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCGIFFLAGS");
        close(p->sock);
        return -1;
    }
    
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(p->sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(p->sock);
        return -1;
    }
    
    struct sockaddr_ll addr = {0};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = p->ifindex;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_pkttype = PACKET_HOST;
    
    if (bind(p->sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(p->sock);
        return -1;
    }
    
    p->vlan = 1;
    p->active = 1;
    
    printf("Added port %d: %s (ifindex: %d)\n", port_count, p->name, p->ifindex);
    port_count++;
    return port_count - 1;
}

int port_find_by_ifindex(int ifindex) {
    for (int i = 0; i < port_count; i++) {
        if (ports[i].ifindex == ifindex && ports[i].active) {
            return i;
        }
    }
    return -1;
}

int port_find_by_name(const char *ifname) {
    for (int i = 0; i < port_count; i++) {
        if (strcmp(ports[i].name, ifname) == 0 && ports[i].active) {
            return i;
        }
    }
    return -1;
}

void port_print(void) {
    printf("\nPort Configuration:\n");
    printf("Port  Interface     VLAN  Status\n");
    printf("----  ---------      ----  ------\n");
    for (int i = 0; i < port_count; i++) {
        printf("%-4d  %-14s %-4d  %s\n", 
               i, ports[i].name, ports[i].vlan, 
               ports[i].active ? "UP" : "DOWN");
    }
    printf("\n");
}

int port_get_sock(int port_idx) {
    if (port_idx < 0 || port_idx >= port_count || !ports[port_idx].active) {
        return -1;
    }
    return ports[port_idx].sock;
}
