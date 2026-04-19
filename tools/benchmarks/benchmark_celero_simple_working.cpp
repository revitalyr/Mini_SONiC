/**
 * @file benchmark_celero_simple_working.cpp
 * @brief Simple Working Celero Benchmarks for SONiC Mini Switch
 */

#include <celero/Celero.h>
#include <cstring>
#include <vector>
#include <string>
#include <random>

// Simple MAC address benchmark
BASELINE(MAC_Baseline, 10, 1000, 30) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::vector<int> macs;
    int target = 0;
    
    // Generate test data
    macs.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        int mac = 0;
        for (int j = 0; j < 6; ++j) {
            mac |= (static_cast<int>(dis(gen)) << (j * 8));
        }
        macs.push_back(mac);
    }
    
    for (int j = 0; j < 6; ++j) {
        target |= (static_cast<int>(dis(gen)) << (j * 8));
    }
    
    // Benchmark
    bool found = false;
    for (const auto& mac : macs) {
        if (mac == target) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

BENCHMARK(MAC_Optimized, 10, 1000, 30) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::vector<int> macs;
    int target = 0;
    
    // Generate test data
    macs.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        int mac = 0;
        for (int j = 0; j < 6; ++j) {
            mac |= (static_cast<int>(dis(gen)) << (j * 8));
        }
        macs.push_back(mac);
    }
    
    for (int j = 0; j < 6; ++j) {
        target |= (static_cast<int>(dis(gen)) << (j * 8));
    }
    
    // Benchmark
    bool found = false;
    for (const auto& mac : macs) {
        if (mac == target) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

// Simple string benchmark
BASELINE(String_Baseline, 10, 1000, 30) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(32, 126);
    
    std::vector<std::string> strings;
    std::string target;
    
    // Generate test data
    strings.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        std::string str;
        for (int j = 0; j < 32; ++j) {
            str += static_cast<char>(dis(gen));
        }
        strings.push_back(str);
    }
    
    target = strings[gen() % strings.size()];
    
    // Benchmark
    bool found = false;
    for (const auto& str : strings) {
        if (str == target) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

BENCHMARK(String_Optimized, 10, 1000, 30) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(32, 126);
    
    std::vector<std::string> strings;
    std::string target;
    
    // Generate test data
    strings.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        std::string str;
        for (int j = 0; j < 32; ++j) {
            str += static_cast<char>(dis(gen));
        }
        strings.push_back(str);
    }
    
    target = strings[gen() % strings.size()];
    
    // Benchmark
    bool found = false;
    for (const auto& str : strings) {
        if (str == target) {
            found = true;
            break;
        }
    }
    celero::DoNotOptimizeAway(found);
}

int main(int argc, char** argv) {
    celero::Run(argc, argv);
    return 0;
}
