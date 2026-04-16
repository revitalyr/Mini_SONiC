/**
 * @file sonic_types.c
 * @brief SONiC Basic Types Implementation
 * 
 * This file implements the basic types and utilities for SONiC
 */

#include "sonic_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <sys/time.h>
#endif

// =============================================================================
// STRING UTILITIES
// =============================================================================

int sonic_string_compare(const char *str1, const char *str2) {
    if (!str1 || !str2) return -1;
    return strcmp(str1, str2);
}

char *sonic_string_duplicate(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (!dup) return NULL;
    
    strcpy(dup, str);
    return dup;
}

void sonic_string_free(char *str) {
    if (str) {
        free(str);
    }
}

// =============================================================================
// LIST UTILITIES
// =============================================================================

sonic_list_t *sonic_list_create(void) {
    sonic_list_t *list = malloc(sizeof(sonic_list_t));
    if (!list) return NULL;
    
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    
    return list;
}

void sonic_list_destroy(sonic_list_t *list) {
    if (!list) return;
    
    sonic_list_node_t *current = list->head;
    while (current) {
        sonic_list_node_t *next = current->next;
        free(current);
        current = next;
    }
    
    free(list);
}

int sonic_list_add(sonic_list_t *list, void *data) {
    if (!list || !data) return SONIC_STATUS_INVALID;
    
    sonic_list_node_t *node = malloc(sizeof(sonic_list_node_t));
    if (!node) return SONIC_STATUS_NO_MEMORY;
    
    node->data = data;
    node->next = NULL;
    
    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    
    list->tail = node;
    list->count++;
    
    return SONIC_STATUS_SUCCESS;
}

void *sonic_list_get(sonic_list_t *list, uint16_t index) {
    if (!list || index >= list->count) return NULL;
    
    sonic_list_node_t *current = list->head;
    for (uint16_t i = 0; i < index && current; i++) {
        current = current->next;
    }
    
    return current ? current->data : NULL;
}

uint16_t sonic_list_count(sonic_list_t *list) {
    return list ? list->count : 0;
}

// =============================================================================
// HASH TABLE UTILITIES
// =============================================================================

static uint32_t sonic_hash_string(const char *str) {
    uint32_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

sonic_hash_table_t *sonic_hash_create(uint16_t bucket_count) {
    if (bucket_count == 0) return NULL;
    
    sonic_hash_table_t *table = malloc(sizeof(sonic_hash_table_t));
    if (!table) return NULL;
    
    table->buckets = calloc(bucket_count, sizeof(sonic_hash_entry_t*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    table->bucket_count = bucket_count;
    table->entry_count = 0;
    
    return table;
}

void sonic_hash_destroy(sonic_hash_table_t *table) {
    if (!table) return;
    
    for (uint16_t i = 0; i < table->bucket_count; i++) {
        sonic_hash_entry_t *entry = table->buckets[i];
        while (entry) {
            sonic_hash_entry_t *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    
    free(table->buckets);
    free(table);
}

int sonic_hash_set(sonic_hash_table_t *table, const char *key, void *value) {
    if (!table || !key || !value) return SONIC_STATUS_INVALID;
    
    uint32_t hash = sonic_hash_string(key);
    uint16_t index = hash % table->bucket_count;
    
    // Check if key already exists
    sonic_hash_entry_t *entry = table->buckets[index];
    while (entry) {
        if (sonic_string_compare(entry->key, key) == 0) {
            entry->value = value;
            return SONIC_STATUS_SUCCESS;
        }
        entry = entry->next;
    }
    
    // Create new entry
    entry = malloc(sizeof(sonic_hash_entry_t));
    if (!entry) return SONIC_STATUS_NO_MEMORY;
    
    entry->key = sonic_string_duplicate(key);
    entry->value = value;
    entry->next = table->buckets[index];
    
    table->buckets[index] = entry;
    table->entry_count++;
    
    return SONIC_STATUS_SUCCESS;
}

void *sonic_hash_get(sonic_hash_table_t *table, const char *key) {
    if (!table || !key) return NULL;
    
    uint32_t hash = sonic_hash_string(key);
    uint16_t index = hash % table->bucket_count;
    
    sonic_hash_entry_t *entry = table->buckets[index];
    while (entry) {
        if (sonic_string_compare(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

int sonic_hash_remove(sonic_hash_table_t *table, const char *key) {
    if (!table || !key) return SONIC_STATUS_INVALID;
    
    uint32_t hash = sonic_hash_string(key);
    uint16_t index = hash % table->bucket_count;
    
    sonic_hash_entry_t *prev = NULL;
    sonic_hash_entry_t *entry = table->buckets[index];
    
    while (entry) {
        if (sonic_string_compare(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[index] = entry->next;
            }
            
            free(entry->key);
            free(entry);
            table->entry_count--;
            
            return SONIC_STATUS_SUCCESS;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
    return SONIC_STATUS_NOT_FOUND;
}

// =============================================================================
// TIMER UTILITIES
// =============================================================================

void sonic_timer_start(sonic_timer_t *timer) {
    if (!timer) return;
    
    timer->start_time = sonic_get_timestamp().seconds * 1000 + 
                       sonic_get_timestamp().nanoseconds / 1000000;
    timer->running = true;
}

void sonic_timer_stop(sonic_timer_t *timer) {
    if (!timer) return;
    
    timer->end_time = sonic_get_timestamp().seconds * 1000 + 
                     sonic_get_timestamp().nanoseconds / 1000000;
    timer->running = false;
}

uint64_t sonic_timer_elapsed_ms(sonic_timer_t *timer) {
    if (!timer) return 0;
    
    if (timer->running) {
        uint64_t current = sonic_get_timestamp().seconds * 1000 + 
                              sonic_get_timestamp().nanoseconds / 1000000;
        return current - timer->start_time;
    } else {
        return timer->end_time - timer->start_time;
    }
}

uint64_t sonic_timer_elapsed_us(sonic_timer_t *timer) {
    return sonic_timer_elapsed_ms(timer) * 1000;
}

// =============================================================================
// TIMESTAMP UTILITIES
// =============================================================================

sonic_timestamp_t sonic_get_timestamp(void) {
    sonic_timestamp_t ts;
    
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    ts.seconds = time(NULL);
    ts.nanoseconds = st.wMilliseconds * 1000000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    ts.seconds = tv.tv_sec;
    ts.nanoseconds = tv.tv_usec * 1000;
#endif
    
    return ts;
}

uint64_t sonic_timestamp_to_ms(sonic_timestamp_t ts) {
    return ts.seconds * 1000 + ts.nanoseconds / 1000000;
}

uint64_t sonic_timestamp_to_us(sonic_timestamp_t ts) {
    return ts.seconds * 1000000 + ts.nanoseconds / 1000;
}

// =============================================================================
// NETWORK UTILITIES
// =============================================================================

void sonic_mac_to_string(const sonic_mac_t *mac, char *str) {
    if (!mac || !str) return;
    
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac->bytes[0], mac->bytes[1], mac->bytes[2],
            mac->bytes[3], mac->bytes[4], mac->bytes[5]);
}

void sonic_mac_from_string(const char *str, sonic_mac_t *mac) {
    if (!str || !mac) return;
    
    unsigned int bytes[6];
    if (sscanf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
               &bytes[0], &bytes[1], &bytes[2],
               &bytes[3], &bytes[4], &bytes[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac->bytes[i] = (uint8_t)bytes[i];
        }
    }
}

void sonic_ipv4_to_string(const sonic_ipv4_t *ip, char *str) {
    if (!ip || !str) return;
    
    sprintf(str, "%u.%u.%u.%u",
            ip->bytes[0], ip->bytes[1], ip->bytes[2], ip->bytes[3]);
}

void sonic_ipv4_from_string(const char *str, sonic_ipv4_t *ip) {
    if (!str || !ip) return;
    
    unsigned int bytes[4];
    if (sscanf(str, "%u.%u.%u.%u",
               &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
        for (int i = 0; i < 4; i++) {
            ip->bytes[i] = (uint8_t)bytes[i];
        }
    }
}

uint16_t sonic_calculate_checksum(const void *data, uint16_t length) {
    if (!data || length == 0) return 0;
    
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;
    
    for (uint16_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

// =============================================================================
// LOGGING UTILITIES
// =============================================================================

static sonic_log_level_t g_log_level = LOG_INFO;

void sonic_log_init(sonic_log_level_t level) {
    g_log_level = level;
}

void sonic_log_cleanup(void) {
    // Nothing to cleanup for now
}

void sonic_log(sonic_log_level_t level, const char *format, ...) {
    if (level > g_log_level) return;
    
    const char *level_strings[] = {"ERROR", "WARN", "INFO", "DEBUG"};
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    printf("[%04d-%02d-%02d %02d:%02d:%02d] [%s] SONiC: ",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
           level_strings[level]);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}
