#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// MAC address utilities
void print_mac(const uint8_t *mac);
void mac_to_string(const uint8_t *mac, char *buf, size_t buf_size);
bool is_broadcast_mac(const uint8_t *mac);
bool is_multicast_mac(const uint8_t *mac);
bool is_valid_mac(const uint8_t *mac);

// IP address utilities
void print_ip(uint32_t ip);
void ip_to_string(uint32_t ip, char *buf, size_t buf_size);
bool is_same_subnet(uint32_t ip1, uint32_t ip2, uint32_t mask);

// Common table entry with timestamp
typedef struct {
    time_t last_seen;
    bool valid;
} table_entry_t;

// Generic table operations
typedef bool (*entry_match_fn)(const void *entry, const void *key);
typedef void (*entry_copy_fn)(void *dest, const void *src);
typedef void (*entry_clear_fn)(void *entry);

// Generic table management
void table_init(void *table, size_t entry_size, size_t count,
                entry_clear_fn clear_fn);
int table_find_entry(void *table, size_t entry_size, size_t count,
                     const void *key, entry_match_fn match_fn);
int table_insert_or_update(void *table, size_t entry_size, size_t count,
                           const void *key, const void *data,
                           entry_match_fn match_fn, entry_copy_fn copy_fn);
int table_find_oldest(void *table, size_t entry_size, size_t count,
                      entry_match_fn is_empty_fn);
void table_cleanup_by_age(void *table, size_t entry_size, size_t count,
                          int max_age_seconds, entry_clear_fn clear_fn);
int table_get_valid_count(void *table, size_t entry_size, size_t count,
                          entry_match_fn is_valid_fn);
