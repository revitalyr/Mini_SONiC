/**
 * @file test_simple.c
 * @brief Simple Test without Catch2
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/sonic_types.h"

int test_mac_operations(void) {
    printf("Testing MAC operations...\n");
    
    sonic_mac_t mac1 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    sonic_mac_t mac2 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    sonic_mac_t mac3 = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Test MAC comparison
    assert(SONIC_MAC_EQUAL(mac1, mac2) == 1);
    assert(SONIC_MAC_EQUAL(mac1, mac3) == 0);
    
    // Test MAC broadcast detection
    assert(SONIC_MAC_IS_BROADCAST(mac3) == 1);
    assert(SONIC_MAC_IS_BROADCAST(mac1) == 0);
    
    printf("MAC operations test passed!\n");
    return 0;
}

int test_string_operations(void) {
    printf("Testing string operations...\n");
    
    const char* str1 = "hello";
    const char* str2 = "hello";
    const char* str3 = "world";
    
    // Test string comparison
    assert(sonic_string_compare(str1, str2) == 0);
    assert(sonic_string_compare(str1, str3) != 0);
    
    // Test string duplication
    char* dup = sonic_string_duplicate(str1);
    assert(dup != NULL);
    assert(sonic_string_compare(dup, str1) == 0);
    sonic_string_free(dup);
    
    printf("String operations test passed!\n");
    return 0;
}

int test_hash_table(void) {
    printf("Testing hash table operations...\n");
    
    sonic_hash_table_t* table = sonic_hash_create(16);
    assert(table != NULL);
    assert(table->bucket_count == 16);
    assert(table->entry_count == 0);
    
    // Test hash set and get
    const char* key = "test_key";
    const char* value = "test_value";
    
    int result = sonic_hash_set(table, key, (void*)value);
    assert(result == SONIC_STATUS_SUCCESS);
    assert(table->entry_count == 1);
    
    void* retrieved = sonic_hash_get(table, key);
    assert(retrieved == (void*)value);
    
    // Test hash remove
    result = sonic_hash_remove(table, key);
    assert(result == SONIC_STATUS_SUCCESS);
    assert(table->entry_count == 0);
    
    retrieved = sonic_hash_get(table, key);
    assert(retrieved == NULL);
    
    sonic_hash_destroy(table);
    printf("Hash table test passed!\n");
    return 0;
}

int main(void) {
    printf("Running SONiC Mini Switch Tests...\n\n");
    
    int result = 0;
    
    result += test_mac_operations();
    result += test_string_operations();
    result += test_hash_table();
    
    if (result == 0) {
        printf("\n✅ All tests passed!\n");
    } else {
        printf("\n❌ Some tests failed!\n");
    }
    
    return result;
}
