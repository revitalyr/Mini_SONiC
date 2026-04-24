#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <atomic>

// Semantic type aliases following modern C++ conventions
namespace MiniSonic::Types {

// Basic string types
using String = std::string;
using StringView = std::string_view;

// Network-related types
using MacAddress = std::uint64_t;
using IpAddress = std::uint32_t;
using Port = std::uint16_t;
using NextHop = String;

// Container types
using Strings = std::vector<String>;
using MacAddresses = std::vector<MacAddress>;
using IpAddresses = std::vector<IpAddress>;

// Smart pointer types
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Time-related types
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;
using TimePoint = std::chrono::steady_clock::time_point;

// Numeric types with semantic meaning
using Count = std::size_t;
using RateLimit = std::size_t;
using Threshold = std::size_t;
using PrefixLength = std::uint8_t;

// Configuration types
using FileName = String;
using ConnectionString = String;
using UserName = String;
using Password = String;
using DbType = String;

// Threading types
using Mutex = std::mutex;
using LockGuard = std::lock_guard<Mutex>;
using AtomicBool = std::atomic<bool>;
template<typename T>
using Atomic = std::atomic<T>;

// Optional types
template<typename T>
using Optional = std::optional<T>;

// Map types
template<typename K, typename V>
using Map = std::unordered_map<K, V>;

using FdbTable = Map<MacAddress, Port>;
using RoutingTable = Map<IpAddress, NextHop>;

// Conversion helpers for tests and logging
inline MacAddress macToUint64(const std::string& mac) {
    std::uint64_t res = 0;
    unsigned int bytes[6];
    if (sscanf_s(mac.c_str(), "%x:%x:%x:%x:%x:%x",
                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) == 6) {
        for (int i = 0; i < 6; ++i) res = (res << 8) | (bytes[i] & 0xFF);
    }
    return res;
}

inline IpAddress ipToUint32(const std::string& ip) {
    unsigned int bytes[4];
    if (sscanf_s(ip.c_str(), "%u.%u.%u.%u", &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }
    return 0;
}

using PortId = std::uint16_t;
using PacketCount = std::uint64_t;

} // namespace MiniSonic::Types
