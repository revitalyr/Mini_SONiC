#include "common.h"
#include "ethernet.h"
#include "net_utils.h"

// is_broadcast and is_multicast are now available from net_utils.h as inline functions
// Kept here for backward compatibility
int is_broadcast(const uint8_t *mac) {
    return is_broadcast_mac(mac);
}

int is_multicast(const uint8_t *mac) {
    return is_multicast_mac(mac);
}
