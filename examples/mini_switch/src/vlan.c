#include "common.h"
#include "vlan.h"
#include "cross_platform.h"

typedef struct {
    uint16_t vlan_id;
    int ports[MAX_PORTS];
    int port_count;
} vlan_entry_t;

static vlan_entry_t vlan_table[VLAN_TABLE_SIZE];
static int vlan_count = 0;
static mutex_t vlan_mutex;

void vlan_init(void) {
    memset(vlan_table, 0, sizeof(vlan_table));
    vlan_count = 0;
    MUTEX_INIT(vlan_mutex);
}


void vlan_add_port(int port, uint16_t vlan_id) {
    MUTEX_LOCK(vlan_mutex);
    
    vlan_entry_t *vlan = NULL;
    
    for (int i = 0; i < vlan_count; i++) {
        if (vlan_table[i].vlan_id == vlan_id) {
            vlan = &vlan_table[i];
            break;
        }
    }
    
    if (!vlan) {
        if (vlan_count >= VLAN_TABLE_SIZE) {
            MUTEX_UNLOCK(vlan_mutex);
            return;
        }
        vlan = &vlan_table[vlan_count];
        vlan->vlan_id = vlan_id;
        vlan->port_count = 0;
        vlan_count++;
    }
    
    for (int i = 0; i < vlan->port_count; i++) {
        if (vlan->ports[i] == port) {
            MUTEX_UNLOCK(vlan_mutex);
            return;
        }
    }
    
    if (vlan->port_count < MAX_PORTS) {
        vlan->ports[vlan->port_count++] = port;
        printf("Added port %d to VLAN %d\n", port, vlan_id);
    }
    
    MUTEX_UNLOCK(vlan_mutex);
}

int port_vlan_allowed(int port, uint16_t vlan_id) {
    // VLAN 0 (untagged) is always allowed on all ports
    if (vlan_id == 0) {
        return 1;
    }
    
    MUTEX_LOCK(vlan_mutex);
    
    for (int i = 0; i < vlan_count; i++) {
        if (vlan_table[i].vlan_id == vlan_id) {
            for (int j = 0; j < vlan_table[i].port_count; j++) {
                if (vlan_table[i].ports[j] == port) {
                    MUTEX_UNLOCK(vlan_mutex);
                    return 1;
                }
            }
        }
    }
    
    MUTEX_UNLOCK(vlan_mutex);
    return 0;
}
