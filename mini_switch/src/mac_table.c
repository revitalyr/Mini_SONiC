#include "common.h"
#include "mac_table.h"
#include "ethernet.h"
#include "visual.h"
#include "cross_platform.h"
#include "net_utils.h"

static mac_entry_t mac_table[MAC_TABLE_SIZE];
static mutex_t mac_table_mutex;

// Helper: check if MAC entry matches given MAC address
static bool mac_match(const void *entry, const void *key) {
    const mac_entry_t *e = entry;
    const uint8_t *mac = key;
    if (!mac) return !e->port; // Empty slot check
    return memcmp(e->mac, mac, 6) == 0;
}

// Helper: check if MAC entry is valid
static bool mac_is_valid(const void *entry, const void *unused) {
    (void)unused;
    const mac_entry_t *e = entry;
    return e->port > 0;
}

// Helper: copy MAC entry data
static void mac_copy(void *dest, const void *src) {
    mac_entry_t *d = dest;
    const mac_entry_t *s = src;
    *d = *s;
}

// Helper: clear MAC entry
static void mac_clear(void *entry) {
    mac_entry_t *e = entry;
    memset(e, 0, sizeof(*e));
}

void mac_table_init(void) {
    table_init(mac_table, sizeof(mac_entry_t), MAC_TABLE_SIZE, mac_clear);
    MUTEX_INIT(mac_table_mutex);
}

void mac_learn(const uint8_t *mac, int port, uint16_t vlan) {
    MUTEX_LOCK(mac_table_mutex);
    
    mac_entry_t new_entry = {
        .port = port,
        .vlan = vlan,
        .last_seen = time(NULL)
    };
    memcpy(new_entry.mac, mac, 6);
    
    int idx = table_find_entry(mac_table, sizeof(mac_entry_t), MAC_TABLE_SIZE, mac, mac_match);
    
    if (idx >= 0) {
        // Update existing
        mac_copy(&mac_table[idx], &new_entry);
    } else {
        // Find slot for new entry
        idx = table_find_oldest(mac_table, sizeof(mac_entry_t), MAC_TABLE_SIZE, mac_match);
        mac_copy(&mac_table[idx], &new_entry);
    }
    
    printf("MAC learned: ");
    print_mac(mac);
    printf(" -> port %d, VLAN %d\n", port, vlan);
    
    MUTEX_UNLOCK(mac_table_mutex);
}

int mac_lookup(const uint8_t *mac, uint16_t vlan) {
    MUTEX_LOCK(mac_table_mutex);
    
    time_t now = time(NULL);
    
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (memcmp(mac_table[i].mac, mac, 6) == 0 && 
            mac_table[i].vlan == vlan && 
            mac_table[i].port > 0 &&
            (now - mac_table[i].last_seen) < MAC_TIMEOUT) {
            
            int port = mac_table[i].port;
            MUTEX_UNLOCK(mac_table_mutex);
            return port;
        }
    }
    
    MUTEX_UNLOCK(mac_table_mutex);
    return -1;
}

void mac_table_cleanup(void) {
    MUTEX_LOCK(mac_table_mutex);
    
    time_t now = time(NULL);
    
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (mac_table[i].port > 0 && (now - mac_table[i].last_seen) > MAC_TIMEOUT) {
            printf("MAC expired: ");
            print_mac(mac_table[i].mac);
            printf("\n");
            mac_clear(&mac_table[i]);
        }
    }
    
    MUTEX_UNLOCK(mac_table_mutex);
}

void mac_table_print(void) {
    MUTEX_LOCK(mac_table_mutex);
    
    printf("\nMAC Table:\n");
    printf("MAC Address         Port  VLAN  Last Seen\n");
    printf("------------------- ---- ---- ------------\n");
    
    time_t now = time(NULL);
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (mac_table[i].port > 0) {
            time_t age = now - mac_table[i].last_seen;
            printf("%02x:%02x:%02x:%02x:%02x:%02x  %-4d %-4d %lld seconds ago\n",
                   mac_table[i].mac[0], mac_table[i].mac[1], mac_table[i].mac[2],
                   mac_table[i].mac[3], mac_table[i].mac[4], mac_table[i].mac[5],
                   mac_table[i].port, mac_table[i].vlan, age);
        }
    }
    printf("\n");
    
    MUTEX_UNLOCK(mac_table_mutex);
}

void mac_table_print_visual(void) {
    MUTEX_LOCK(mac_table_mutex);
    
    uint8_t macs[MAC_TABLE_SIZE][6];
    int ports[MAC_TABLE_SIZE];
    int count = 0;
    
    time_t now = time(NULL);
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (mac_table[i].port > 0 && (now - mac_table[i].last_seen) < 300) {
            memcpy(macs[count], mac_table[i].mac, 6);
            ports[count] = mac_table[i].port;
            count++;
        }
    }
    
    visual_mac_table(macs, ports, count);
    
    MUTEX_UNLOCK(mac_table_mutex);
}

void mac_table_get_entries(uint8_t macs[][6], int ports[], int *count) {
    MUTEX_LOCK(mac_table_mutex);
    
    time_t now = time(NULL);
    *count = 0;
    
    for (int i = 0; i < MAC_TABLE_SIZE; i++) {
        if (mac_table[i].port >= 0 && (now - mac_table[i].last_seen) < 300) {
            memcpy(macs[*count], mac_table[i].mac, 6);
            ports[*count] = mac_table[i].port;
            (*count)++;
        }
    }
    
    MUTEX_UNLOCK(mac_table_mutex);
}
