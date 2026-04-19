/**
 * @file benchmark_celero.cpp
 * @brief Celero Benchmarks for SONiC Mini Switch
 * 
 * This file contains performance benchmarks using Celero framework
 * instead of Google Benchmark.
 */

#include <cstdint>
#include <celero/Celero.h>
#include <cstring>
#include <chrono>
#include <vector>
#include <string_view>
#include <string>
#include <random>

// Mock SONiC types for benchmarking
typedef struct {
    uint8_t bytes[6];
} sonic_mac_t;

typedef struct {
    uint8_t bytes[4];
} sonic_ipv4_t;

// Mock FDB entry
typedef struct {
    sonic_mac_t mac;
    uint16_t port;
    uint32_t timestamp;
} fdb_entry_t;

// =============================================================================
// MAC ADDRESS BENCHMARKS
// =============================================================================

class MacAddressBenchmark : public celero::TestFixture {
public:
    void setUp(const celero::TestFixture::ExperimentValue& value) override {
        // Generate random MAC addresses for testing
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        test_macs.clear();
        test_macs.reserve(1000);
        
        for (int i = 0; i < 1000; ++i) {
            sonic_mac_t mac;
            for (int j = 0; j < 6; ++j) {
                mac.bytes[j] = static_cast<uint8_t>(dis(gen));
            }
            test_macs.push_back(mac);
        }
        
        // Create target MAC for comparison
        for (int j = 0; j < 6; ++j) {
            target_mac.bytes[j] = static_cast<uint8_t>(dis(gen));
        }
    }
    
    std::vector<sonic_mac_t> test_macs;
    sonic_mac_t target_mac;
};

BASELINE_F(MacAddressBenchmark, BaselineMACComparison, 1000, 10000) {
    bool found = false;
    for (const auto& mac : test_macs) {
        if (memcmp(&mac, &target_mac, sizeof(sonic_mac_t)) == 0) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

BENCHMARK_F(MacAddressBenchmark, OptimizedMACComparison, 1000, 10000) {
    bool found = false;
    for (const auto& mac : test_macs) {
        // Optimized comparison using 64-bit integer
        const uint64_t* mac1 = reinterpret_cast<const uint64_t*>(&mac);
        const uint64_t* mac2 = reinterpret_cast<const uint64_t*>(&target_mac);
        
        if (*mac1 == *mac2) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

// =============================================================================
// FDB LOOKUP BENCHMARKS
// =============================================================================

class FDBBenchmark : public celero::TestFixture {
public:
    void setUp(const celero::TestFixture::ExperimentValue& value) override {
        // Create FDB entries
        fdb_entries.clear();
        fdb_entries.reserve(4096);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> mac_dis(0, 255);
        std::uniform_int_distribution<> port_dis(1, 24);
        
        for (int i = 0; i < 4096; ++i) {
            fdb_entry_t entry;
            for (int j = 0; j < 6; ++j) {
                entry.mac.bytes[j] = static_cast<uint8_t>(mac_dis(gen));
            }
            entry.port = static_cast<uint16_t>(port_dis(gen));
            entry.timestamp = static_cast<uint32_t>(i);
            fdb_entries.push_back(entry);
        }
        
        // Select random target for lookup
        target_index = gen() % fdb_entries.size();
        target_mac = fdb_entries[target_index].mac;
    }
    
    std::vector<fdb_entry_t> fdb_entries;
    size_t target_index;
    sonic_mac_t target_mac;
};

BASELINE_F(FDBBenchmark, LinearFDBLookup, 1000, 10000) {
    fdb_entry_t* found = nullptr;
    
    for (const auto& entry : fdb_entries) {
        if (memcmp(&entry.mac, &target_mac, sizeof(sonic_mac_t)) == 0) {
            found = const_cast<fdb_entry_t*>(&entry);
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

BENCHMARK_F(FDBBenchmark, OptimizedFDBLookup, 1000, 10000) {
    fdb_entry_t* found = nullptr;
    
    // Optimized lookup using 64-bit comparison
    const uint64_t* target_mac64 = reinterpret_cast<const uint64_t*>(&target_mac);
    
    for (const auto& entry : fdb_entries) {
        const uint64_t* entry_mac64 = reinterpret_cast<const uint64_t*>(&entry.mac);
        
        if (*entry_mac64 == *target_mac64) {
            found = const_cast<fdb_entry_t*>(&entry);
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

// =============================================================================
// HASH TABLE BENCHMARKS
// =============================================================================

class HashTableBenchmark : public celero::TestFixture {
public:
    void setUp(const celero::TestFixture::ExperimentValue& value) override {
        // Create hash table entries
        hash_data.clear();
        hash_data.reserve(10000);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 999999);
        
        for (int i = 0; i < 10000; ++i) {
            uint32_t key = dis(gen);
            uint32_t value = dis(gen);
            hash_data.push_back({key, value});
        }
        
        // Select target for lookup
        target_key = hash_data[gen() % hash_data.size()].key;
    }
    
    struct HashEntry {
        uint32_t key;
        uint32_t value;
    };
    
    std::vector<HashEntry> hash_data;
    uint32_t target_key;
};

BASELINE_F(HashTableBenchmark, SimpleHashLookup, 1000, 10000) {
    uint32_t found_value = 0;
    
    for (const auto& entry : hash_data) {
        if (entry.key == target_key) {
            found_value = entry.value;
            break;
        }
    }
    celero::DoNotOptimizeAway(found_value);
}

BENCHMARK_F(HashTableBenchmark, OptimizedHashLookup, 1000, 10000) {
    uint32_t found_value = 0;
    
    // Optimized lookup with better memory access patterns
    size_t hash_size = hash_data.size();
    const HashEntry* data_ptr = hash_data.data();
    
    for (size_t i = 0; i < hash_size; ++i) {
        if (data_ptr[i].key == target_key) {
            found_value = data_ptr[i].value;
            break;
        }
    }
    celero::DoNotOptimizeAway(found_value);
}

// =============================================================================
// STRING PROCESSING BENCHMARKS
// =============================================================================

class StringBenchmark : public celero::TestFixture {
public:
    void setUp(const celero::TestFixture::ExperimentValue& value) override {
        // Generate test strings
        test_strings.clear();
        test_strings.reserve(1000);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(32, 126); // ASCII printable
        
        for (int i = 0; i < 1000; ++i) {
            std::string str;
            str.reserve(32);
            for (int j = 0; j < 32; ++j) {
                str += static_cast<char>(dis(gen));
            }
            test_strings.push_back(str);
        }
        
        target_string = test_strings[gen() % test_strings.size()];
    }
    
    std::vector<std::string> test_strings;
    std::string target_string;
};

BASELINE_F(StringBenchmark, StdStringComparison, 1000, 10000) {
    bool found = false;
    
    for (const auto& str : test_strings) {
        if (str == target_string) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

BENCHMARK_F(StringBenchmark, OptimizedStringComparison, 1000, 10000) {
    bool found = false;
    
    // Optimized comparison using string_view (C++17)
    std::string_view target_view(target_string);
    
    for (const auto& str : test_strings) {
        std::string_view str_view(str);
        if (str_view == target_view) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

// =============================================================================
// MAIN BENCHMARK FUNCTION
// =============================================================================

int main(int argc, char** argv) {
    celero::Run(argc, argv);
    return 0;
}
