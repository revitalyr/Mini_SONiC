/**
 * @file test_sonic_types.cpp
 * @brief Catch2 v3 Tests for SONiC Types
 * 
 * This file contains unit tests for SONiC basic types and utilities
 * using Catch2 v3 testing framework.
 */

#include <catch2/catch_all.hpp>
#include "../src/sonic_types.h"
#include <cstring>
#include <arpa/inet.h>

// =============================================================================
// MAC ADDRESS TESTS
// =============================================================================

TEST_CASE("MAC Address Operations", "[mac_address]") {
    SECTION("MAC comparison") {
        sonic_mac_t mac1 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        sonic_mac_t mac2 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        sonic_mac_t mac3 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        REQUIRE(SONIC_MAC_EQUAL(mac1, mac2) == true);
        REQUIRE(SONIC_MAC_EQUAL(mac1, mac3) == false);
    }
    
    SECTION("MAC broadcast detection") {
        sonic_mac_t broadcast = SONIC_MAC_BROADCAST;
        sonic_mac_t normal = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        
        REQUIRE(SONIC_MAC_IS_BROADCAST(broadcast) == true);
        REQUIRE(SONIC_MAC_IS_BROADCAST(normal) == false);
    }
    
    SECTION("MAC multicast detection") {
        sonic_mac_t multicast = SONIC_MAC_MULTICAST;
        sonic_mac_t normal = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        
        REQUIRE(SONIC_MAC_IS_MULTICAST(multicast) == true);
        REQUIRE(SONIC_MAC_IS_MULTICAST(normal) == false);
    }
    
    SECTION("MAC zero detection") {
        sonic_mac_t zero = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        sonic_mac_t normal = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        
        REQUIRE(SONIC_MAC_IS_ZERO(zero) == true);
        REQUIRE(SONIC_MAC_IS_ZERO(normal) == false);
    }
    
    SECTION("MAC copy") {
        sonic_mac_t src = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        sonic_mac_t dst;
        
        SONIC_MAC_COPY(dst, src);
        
        REQUIRE(SONIC_MAC_EQUAL(dst, src) == true);
    }
}

// =============================================================================
// IPv4 ADDRESS TESTS
// =============================================================================

TEST_CASE("IPv4 Address Operations", "[ipv4_address]") {
    SECTION("IPv4 comparison") {
        sonic_ipv4_t ip1 = SONIC_IPV4_LOOPBACK;
        sonic_ipv4_t ip2 = {127, 0, 0, 1};
        sonic_ipv4_t ip3 = {192, 168, 1, 1};
        
        REQUIRE(SONIC_IPV4_EQUAL(ip1, ip2) == true);
        REQUIRE(SONIC_IPV4_EQUAL(ip1, ip3) == false);
    }
    
    SECTION("IPv4 broadcast detection") {
        sonic_ipv4_t broadcast = SONIC_IPV4_BROADCAST;
        sonic_ipv4_t normal = {192, 168, 1, 100};
        
        REQUIRE(SONIC_IPV4_IS_BROADCAST(broadcast) == true);
        REQUIRE(SONIC_IPV4_IS_BROADCAST(normal) == false);
    }
    
    SECTION("IPv4 loopback detection") {
        sonic_ipv4_t loopback = SONIC_IPV4_LOOPBACK;
        sonic_ipv4_t normal = {192, 168, 1, 100};
        
        REQUIRE(SONIC_IPV4_IS_LOOPBACK(loopback) == true);
        REQUIRE(SONIC_IPV4_IS_LOOPBACK(normal) == false);
    }
    
    SECTION("IPv4 copy") {
        sonic_ipv4_t src = {192, 168, 1, 100};
        sonic_ipv4_t dst;
        
        SONIC_IPV4_COPY(dst, src);
        
        REQUIRE(SONIC_IPV4_EQUAL(dst, src) == true);
    }
}

// =============================================================================
// STRING UTILITIES TESTS
// =============================================================================

TEST_CASE("String Utilities", "[string_utils]") {
    SECTION("String comparison") {
        const char* str1 = "hello";
        const char* str2 = "hello";
        const char* str3 = "world";
        
        REQUIRE(sonic_string_compare(str1, str2) == 0);
        REQUIRE(sonic_string_compare(str1, str3) != 0);
    }
    
    SECTION("String duplication") {
        const char* original = "test string";
        char* duplicate = sonic_string_duplicate(original);
        
        REQUIRE(duplicate != nullptr);
        REQUIRE(strcmp(duplicate, original) == 0);
        
        sonic_string_free(duplicate);
    }
}

// =============================================================================
// HASH TABLE TESTS
// =============================================================================

TEST_CASE("Hash Table Operations", "[hash_table]") {
    SECTION("Hash table creation and destruction") {
        sonic_hash_table_t* table = sonic_hash_create(16);
        
        REQUIRE(table != nullptr);
        REQUIRE(table->bucket_count == 16);
        REQUIRE(table->entry_count == 0);
        
        sonic_hash_destroy(table);
    }
    
    SECTION("Hash table set and get") {
        sonic_hash_table_t* table = sonic_hash_create(16);
        
        const char* key = "test_key";
        const char* value = "test_value";
        
        int result = sonic_hash_set(table, key, (void*)value);
        REQUIRE(result == SONIC_STATUS_SUCCESS);
        REQUIRE(table->entry_count == 1);
        
        void* retrieved = sonic_hash_get(table, key);
        REQUIRE(retrieved == (void*)value);
        
        sonic_hash_destroy(table);
    }
    
    SECTION("Hash table remove") {
        sonic_hash_table_t* table = sonic_hash_create(16);
        
        const char* key = "test_key";
        const char* value = "test_value";
        
        sonic_hash_set(table, key, (void*)value);
        REQUIRE(table->entry_count == 1);
        
        int result = sonic_hash_remove(table, key);
        REQUIRE(result == SONIC_STATUS_SUCCESS);
        REQUIRE(table->entry_count == 0);
        
        void* retrieved = sonic_hash_get(table, key);
        REQUIRE(retrieved == nullptr);
        
        sonic_hash_destroy(table);
    }
}

// =============================================================================
// LIST UTILITIES TESTS
// =============================================================================

TEST_CASE("List Operations", "[list]") {
    SECTION("List creation and destruction") {
        sonic_list_t* list = sonic_list_create();
        
        REQUIRE(list != nullptr);
        REQUIRE(list->head == nullptr);
        REQUIRE(list->tail == nullptr);
        REQUIRE(list->count == 0);
        
        sonic_list_destroy(list);
    }
    
    SECTION("List add and count") {
        sonic_list_t* list = sonic_list_create();
        
        int value1 = 10;
        int value2 = 20;
        int value3 = 30;
        
        sonic_list_add(list, &value1);
        REQUIRE(list->count == 1);
        
        sonic_list_add(list, &value2);
        REQUIRE(list->count == 2);
        
        sonic_list_add(list, &value3);
        REQUIRE(list->count == 3);
        
        sonic_list_destroy(list);
    }
    
    SECTION("List get by index") {
        sonic_list_t* list = sonic_list_create();
        
        int value1 = 10;
        int value2 = 20;
        int value3 = 30;
        
        sonic_list_add(list, &value1);
        sonic_list_add(list, &value2);
        sonic_list_add(list, &value3);
        
        int* retrieved = (int*)sonic_list_get(list, 1);
        REQUIRE(retrieved != nullptr);
        REQUIRE(*retrieved == 20);
        
        retrieved = (int*)sonic_list_get(list, 5); // Out of bounds
        REQUIRE(retrieved == nullptr);
        
        sonic_list_destroy(list);
    }
}

// =============================================================================
// TIMER TESTS
// =============================================================================

TEST_CASE("Timer Operations", "[timer]") {
    SECTION("Timer start and stop") {
        sonic_timer_t timer;
        
        sonic_timer_start(&timer);
        REQUIRE(timer.running == true);
        REQUIRE(timer.start_time > 0);
        
        // Sleep for a short time
        usleep(10000); // 10ms
        
        uint64_t elapsed = sonic_timer_elapsed_ms(&timer);
        REQUIRE(elapsed >= 10); // At least 10ms
        
        sonic_timer_stop(&timer);
        REQUIRE(timer.running == false);
        REQUIRE(timer.end_time > timer.start_time);
    }
}

// =============================================================================
// TIMESTAMP TESTS
// =============================================================================

TEST_CASE("Timestamp Operations", "[timestamp]") {
    SECTION("Get timestamp") {
        sonic_timestamp_t ts = sonic_get_timestamp();
        
        REQUIRE(ts.seconds > 0);
        REQUIRE(ts.nanoseconds >= 0);
        REQUIRE(ts.nanoseconds < 1000000000); // Less than 1 second
    }
    
    SECTION("Timestamp conversion") {
        sonic_timestamp_t ts = sonic_get_timestamp();
        
        uint64_t ms = sonic_timestamp_to_ms(ts);
        uint64_t us = sonic_timestamp_to_us(ts);
        
        REQUIRE(ms > 0);
        REQUIRE(us > 0);
        REQUIRE(us > ms * 1000); // Microseconds should be > milliseconds * 1000
    }
}

// =============================================================================
// NETWORK UTILITIES TESTS
// =============================================================================

TEST_CASE("Network Utilities", "[network_utils]") {
    SECTION("MAC to string conversion") {
        sonic_mac_t mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        char mac_str[18]; // "00:11:22:33:44:55" + null terminator
        
        sonic_mac_to_string(&mac, mac_str);
        
        REQUIRE(strcmp(mac_str, "00:11:22:33:44:55") == 0);
    }
    
    SECTION("MAC from string conversion") {
        const char* mac_str = "00:11:22:33:44:55";
        sonic_mac_t mac;
        
        sonic_mac_from_string(mac_str, &mac);
        
        REQUIRE(mac.bytes[0] == 0x00);
        REQUIRE(mac.bytes[1] == 0x11);
        REQUIRE(mac.bytes[2] == 0x22);
        REQUIRE(mac.bytes[3] == 0x33);
        REQUIRE(mac.bytes[4] == 0x44);
        REQUIRE(mac.bytes[5] == 0x55);
    }
    
    SECTION("IPv4 to string conversion") {
        sonic_ipv4_t ip = {192, 168, 1, 100};
        char ip_str[16]; // "192.168.1.100" + null terminator
        
        sonic_ipv4_to_string(&ip, ip_str);
        
        REQUIRE(strcmp(ip_str, "192.168.1.100") == 0);
    }
    
    SECTION("IPv4 from string conversion") {
        const char* ip_str = "192.168.1.100";
        sonic_ipv4_t ip;
        
        sonic_ipv4_from_string(ip_str, &ip);
        
        REQUIRE(ip.bytes[0] == 192);
        REQUIRE(ip.bytes[1] == 168);
        REQUIRE(ip.bytes[2] == 1);
        REQUIRE(ip.bytes[3] == 100);
    }
}

// =============================================================================
// CHECKSUM TESTS
// =============================================================================

TEST_CASE("Checksum Calculation", "[checksum]") {
    SECTION("Basic checksum calculation") {
        uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
        uint16_t checksum = sonic_calculate_checksum(data, sizeof(data));
        
        REQUIRE(checksum != 0);
    }
    
    SECTION("Empty data checksum") {
        uint16_t checksum = sonic_calculate_checksum(nullptr, 0);
        REQUIRE(checksum == 0);
    }
}

// =============================================================================
// PERFORMANCE TESTS
// =============================================================================

TEST_CASE("Performance Tests", "[performance]") {
    SECTION("Hash table performance") {
        sonic_hash_table_t* table = sonic_hash_create(1024);
        
        // Add many entries
        const int num_entries = 1000;
        for (int i = 0; i < num_entries; i++) {
            char key[32];
            snprintf(key, sizeof(key), "key_%d", i);
            sonic_hash_set(table, key, (void*)(intptr_t)i);
        }
        
        REQUIRE(table->entry_count == num_entries);
        
        // Test lookup performance
        sonic_timer_t timer;
        sonic_timer_start(&timer);
        
        for (int i = 0; i < num_entries; i++) {
            char key[32];
            snprintf(key, sizeof(key), "key_%d", i);
            void* value = sonic_hash_get(table, key);
            REQUIRE(value == (void*)(intptr_t)i);
        }
        
        uint64_t elapsed = sonic_timer_elapsed_us(&timer);
        REQUIRE(elapsed < 10000); // Should complete in less than 10ms
        
        sonic_hash_destroy(table);
    }
}

// =============================================================================
// BENCHMARK TESTS
// =============================================================================

TEST_CASE("Benchmark Tests", "[benchmark]") {
    BENCHMARK("MAC comparison") {
        sonic_mac_t mac1 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        sonic_mac_t mac2 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        
        return SONIC_MAC_EQUAL(mac1, mac2);
    };
    
    BENCHMARK("String comparison") {
        const char* str1 = "hello world";
        const char* str2 = "hello world";
        
        return sonic_string_compare(str1, str2);
    };
    
    BENCHMARK("Hash table lookup") {
        static sonic_hash_table_t* table = nullptr;
        if (!table) {
            table = sonic_hash_create(1024);
            for (int i = 0; i < 100; i++) {
                char key[32];
                snprintf(key, sizeof(key), "key_%d", i);
                sonic_hash_set(table, key, (void*)(intptr_t)i);
            }
        }
        
        void* value = sonic_hash_get(table, "key_50");
        return value != nullptr;
    };
}
