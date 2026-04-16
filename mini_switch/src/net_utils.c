#include "net_utils.h"
#include "cross_platform.h"

void print_mac(const uint8_t *mac) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void mac_to_string(const uint8_t *mac, char *buf, size_t buf_size) {
    if (buf && buf_size >= 18) {
        snprintf(buf, buf_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

bool is_broadcast_mac(const uint8_t *mac) {
    static const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return memcmp(mac, broadcast, 6) == 0;
}

bool is_multicast_mac(const uint8_t *mac) {
    return (mac[0] & 0x01) != 0;
}

bool is_valid_mac(const uint8_t *mac) {
    static const uint8_t zero_mac[6] = {0};
    return memcmp(mac, zero_mac, 6) != 0;
}

void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
           (ip >> 8) & 0xFF, ip & 0xFF);
}

void ip_to_string(uint32_t ip, char *buf, size_t buf_size) {
    if (buf && buf_size >= 16) {
        snprintf(buf, buf_size, "%d.%d.%d.%d",
                 (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                 (ip >> 8) & 0xFF, ip & 0xFF);
    }
}

bool is_same_subnet(uint32_t ip1, uint32_t ip2, uint32_t mask) {
    return (ip1 & mask) == (ip2 & mask);
}

// Generic table operations implementation
void table_init(void *table, size_t entry_size, size_t count,
                entry_clear_fn clear_fn) {
    if (!table || !clear_fn) return;
    
    uint8_t *entries = table;
    for (size_t i = 0; i < count; i++) {
        clear_fn(&entries[i * entry_size]);
    }
}

int table_find_entry(void *table, size_t entry_size, size_t count,
                     const void *key, entry_match_fn match_fn) {
    if (!table || !key || !match_fn) return -1;
    
    uint8_t *entries = table;
    for (size_t i = 0; i < count; i++) {
        if (match_fn(&entries[i * entry_size], key)) {
            return (int)i;
        }
    }
    return -1;
}

int table_insert_or_update(void *table, size_t entry_size, size_t count,
                           const void *key, const void *data,
                           entry_match_fn match_fn, entry_copy_fn copy_fn) {
    if (!table || !key || !data || !match_fn || !copy_fn) return -1;
    
    // Try to find existing entry
    int idx = table_find_entry(table, entry_size, count, key, match_fn);
    
    if (idx >= 0) {
        // Update existing
        copy_fn((uint8_t *)table + idx * entry_size, data);
        return idx;
    }
    
    // Find empty slot for new entry
    idx = table_find_entry(table, entry_size, count, NULL, match_fn);
    if (idx >= 0) {
        copy_fn((uint8_t *)table + idx * entry_size, data);
        return idx;
    }
    
    return -1; // Table full
}

int table_find_oldest(void *table, size_t entry_size, size_t count,
                      entry_match_fn is_empty_fn) {
    if (!table || !is_empty_fn) return 0;
    
    uint8_t *entries = table;
    time_t oldest_time = time(NULL);
    int oldest_idx = 0;
    
    for (size_t i = 0; i < count; i++) {
        void *entry = &entries[i * entry_size];
        if (is_empty_fn(entry, NULL)) {
            return (int)i; // Found empty slot
        }
        
        table_entry_t *te = (table_entry_t *)entry;
        if (te->last_seen < oldest_time) {
            oldest_time = te->last_seen;
            oldest_idx = (int)i;
        }
    }
    
    return oldest_idx;
}

void table_cleanup_by_age(void *table, size_t entry_size, size_t count,
                          int max_age_seconds, entry_clear_fn clear_fn) {
    if (!table || !clear_fn || max_age_seconds <= 0) return;
    
    uint8_t *entries = table;
    time_t now = time(NULL);
    
    for (size_t i = 0; i < count; i++) {
        table_entry_t *entry = (table_entry_t *)&entries[i * entry_size];
        if (entry->valid && (now - entry->last_seen) > max_age_seconds) {
            clear_fn(entry);
        }
    }
}

int table_get_valid_count(void *table, size_t entry_size, size_t count,
                          entry_match_fn is_valid_fn) {
    if (!table || !is_valid_fn) return 0;
    
    int count_valid = 0;
    uint8_t *entries = table;
    
    for (size_t i = 0; i < count; i++) {
        if (is_valid_fn(&entries[i * entry_size], NULL)) {
            count_valid++;
        }
    }
    return count_valid;
}
