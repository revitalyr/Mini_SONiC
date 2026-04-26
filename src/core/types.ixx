/**
 * @file types.ixx
 * @brief Global type aliases and conversion helpers (C++20 module)
 *
 * Semantic type aliases following modern C++ conventions.
 * Replaces core/common/types.hpp with a proper C++20 module.
 */

module;

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <atomic>

export module MiniSonic.Core.Types;

export namespace MiniSonic::Types {

// Basic string types
using String = std::string;
using StringView = std::string_view;

// Network-related types
using MacAddress = std::uint64_t;
using IpAddress = std::uint32_t;
using Port = std::uint16_t;
using NextHop = IpAddress;

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
using Timestamp = std::time_t;

// Numeric types with semantic meaning
using Count = std::size_t;
using SequenceNumber = std::size_t;
using RateLimit = std::size_t;
using Threshold = std::size_t;
using PrefixLength = std::uint8_t;
using BufferLength = std::size_t;

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
using RoutingTable = Map<IpAddress, IpAddress>;

// Additional semantic aliases
using PortId = std::uint16_t;
using PacketCount = std::uint64_t;
using VlanId = std::uint16_t;

// Conversion helpers for tests and logging
inline MacAddress macToUint64(const std::string& mac) {
    std::uint64_t res = 0;
    unsigned int bytes[6];
#ifdef _MSC_VER
    if (sscanf_s(mac.c_str(), "%x:%x:%x:%x:%x:%x",
                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) == 6) {
#else
    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x",
                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) == 6) {
#endif
        for (int i = 0; i < 6; ++i) res = (res << 8) | (bytes[i] & 0xFF);
    }
    return res;
}

inline IpAddress ipToUint32(const std::string& ip) {
    unsigned int bytes[4];
#ifdef _MSC_VER
    if (sscanf_s(ip.c_str(), "%u.%u.%u.%u", &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
#else
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &bytes[0], &bytes[1], &bytes[2], &bytes[3]) == 4) {
#endif
        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }
    return 0;
}

inline std::string formatMac(MacAddress mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             static_cast<unsigned int>((mac >> 40) & 0xFF),
             static_cast<unsigned int>((mac >> 32) & 0xFF),
             static_cast<unsigned int>((mac >> 24) & 0xFF),
             static_cast<unsigned int>((mac >> 16) & 0xFF),
             static_cast<unsigned int>((mac >> 8) & 0xFF),
             static_cast<unsigned int>(mac & 0xFF));
    return std::string(buf);
}

inline std::string formatIp(IpAddress ip) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             static_cast<unsigned int>((ip >> 24) & 0xFF),
             static_cast<unsigned int>((ip >> 16) & 0xFF),
             static_cast<unsigned int>((ip >> 8) & 0xFF),
             static_cast<unsigned int>(ip & 0xFF));
    return std::string(buf);
}

} // namespace MiniSonic::Types
