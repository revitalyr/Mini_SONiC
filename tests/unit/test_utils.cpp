/**
 * @file test_utils.cpp
 * @brief Unit tests for MiniSonic.Utils module
 */

#include <catch2/catch_all.hpp>

// Import the module
import MiniSonic.Core.Utils;

using namespace MiniSonic::Utils;
using namespace Types;

TEST_CASE("Utils Metrics CounterOperations", "[utils][metrics]") {
    SECTION("Increment counter") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.inc("test_counter");
        REQUIRE(metrics.getCounter("test_counter") == 1);
        
        metrics.inc("test_counter", 5);
        REQUIRE(metrics.getCounter("test_counter") == 6);
    }
    
    SECTION("Decrement counter") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.set("test_counter", 10);
        metrics.dec("test_counter");
        REQUIRE(metrics.getCounter("test_counter") == 9);
    }
    
    SECTION("Set counter") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.set("test_counter", 42);
        REQUIRE(metrics.getCounter("test_counter") == 42);
    }
}

TEST_CASE("Utils Metrics GaugeOperations", "[utils][metrics]") {
    SECTION("Set gauge") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.setGauge("test_gauge", 3.14);
        REQUIRE(metrics.getGauge("test_gauge") == 3.14);
    }
    
    SECTION("Update gauge") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.setGauge("test_gauge", 1.0);
        metrics.setGauge("test_gauge", 2.5);
        REQUIRE(metrics.getGauge("test_gauge") == 2.5);
    }
}

TEST_CASE("Utils Metrics LatencyTracking", "[utils][metrics]") {
    SECTION("Add latency samples") {
        auto& metrics = Metrics::instance();
        metrics.reset();
        
        metrics.addLatency(100);
        metrics.addLatency(200);
        metrics.addLatency(300);
        
        std::string stats = metrics.getLatencyStats();
        REQUIRE(!stats.empty());
    }
}

TEST_CASE("Utils Metrics Reset", "[utils][metrics]") {
    SECTION("Reset all metrics") {
        auto& metrics = Metrics::instance();
        
        metrics.inc("test_counter", 10);
        metrics.setGauge("test_gauge", 5.0);
        
        metrics.reset();
        
        REQUIRE(metrics.getCounter("test_counter") == 0);
        REQUIRE(metrics.getGauge("test_gauge") == 0.0);
    }
}

TEST_CASE("Utils StringUtils Trim", "[utils][string]") {
    SECTION("Trim whitespace") {
        REQUIRE(StringUtils::trim("  hello  ") == "hello");
        REQUIRE(StringUtils::trim("\tworld\t") == "world");
        REQUIRE(StringUtils::trim("\n  test  \n") == "test");
    }
    
    SECTION("Trim empty string") {
        REQUIRE(StringUtils::trim("") == "");
        REQUIRE(StringUtils::trim("   ") == "");
    }
}

TEST_CASE("Utils StringUtils Split", "[utils][string]") {
    SECTION("Split by delimiter") {
        auto parts = StringUtils::split("a,b,c", ',');
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "b");
        REQUIRE(parts[2] == "c");
    }
    
    SECTION("Split with no delimiter") {
        auto parts = StringUtils::split("hello", ',');
        REQUIRE(parts.size() == 1);
        REQUIRE(parts[0] == "hello");
    }
}

TEST_CASE("Utils StringUtils Join", "[utils][string]") {
    SECTION("Join with delimiter") {
        std::vector<std::string> parts = {"a", "b", "c"};
        REQUIRE(StringUtils::join(parts, ",") == "a,b,c");
    }
    
    SECTION("Join empty vector") {
        std::vector<std::string> parts;
        REQUIRE(StringUtils::join(parts, ",") == "");
    }
}

TEST_CASE("Utils StringUtils CaseConversion", "[utils][string]") {
    SECTION("To lower case") {
        REQUIRE(StringUtils::toLower("HELLO") == "hello");
        REQUIRE(StringUtils::toLower("HeLLo") == "hello");
    }
    
    SECTION("To upper case") {
        REQUIRE(StringUtils::toUpper("hello") == "HELLO");
        REQUIRE(StringUtils::toUpper("HeLLo") == "HELLO");
    }
}

TEST_CASE("Utils StringUtils StartEndWith", "[utils][string]") {
    SECTION("Starts with") {
        REQUIRE(StringUtils::startsWith("hello world", "hello") == true);
        REQUIRE(StringUtils::startsWith("hello world", "world") == false);
    }
    
    SECTION("Ends with") {
        REQUIRE(StringUtils::endsWith("hello world", "world") == true);
        REQUIRE(StringUtils::endsWith("hello world", "hello") == false);
    }
}

TEST_CASE("Utils TimeUtils FormatDuration", "[utils][time]") {
    SECTION("Format nanoseconds") {
        auto duration = std::chrono::nanoseconds(1000);
        std::string formatted = TimeUtils::formatDuration(duration);
        REQUIRE(!formatted.empty());
    }
    
    SECTION("Format milliseconds") {
        auto duration = std::chrono::milliseconds(100);
        std::string formatted = TimeUtils::formatDuration(duration);
        REQUIRE(!formatted.empty());
    }
}

TEST_CASE("Utils TimeUtils Now", "[utils][time]") {
    SECTION("Get current time") {
        auto now = TimeUtils::now();
        auto timestamp = TimeUtils::timestampToMicroseconds(now);
        REQUIRE(timestamp > 0);
    }
}

TEST_CASE("Utils NetworkUtils MacValidation", "[utils][network]") {
    SECTION("Valid MAC addresses") {
        REQUIRE(NetworkUtils::isValidMacAddress("aa:bb:cc:dd:ee:ff") == true);
        REQUIRE(NetworkUtils::isValidMacAddress("00:00:00:00:00:00") == true);
        REQUIRE(NetworkUtils::isValidMacAddress("FF:FF:FF:FF:FF:FF") == true);
    }
    
    SECTION("Invalid MAC addresses") {
        REQUIRE(NetworkUtils::isValidMacAddress("invalid") == false);
        REQUIRE(NetworkUtils::isValidMacAddress("aa:bb:cc") == false);
        REQUIRE(NetworkUtils::isValidMacAddress("") == false);
    }
}

TEST_CASE("Utils NetworkUtils IpValidation", "[utils][network]") {
    SECTION("Valid IP addresses") {
        REQUIRE(NetworkUtils::isValidIpAddress("192.168.1.1") == true);
        REQUIRE(NetworkUtils::isValidIpAddress("10.0.0.1") == true);
        REQUIRE(NetworkUtils::isValidIpAddress("127.0.0.1") == true);
        REQUIRE(NetworkUtils::isValidIpAddress("0.0.0.0") == true);
    }
    
    SECTION("Invalid IP addresses") {
        REQUIRE(NetworkUtils::isValidIpAddress("invalid") == false);
        REQUIRE(NetworkUtils::isValidIpAddress("256.1.1.1") == false);
        REQUIRE(NetworkUtils::isValidIpAddress("") == false);
    }
}

TEST_CASE("Utils NetworkUtils FormatMac", "[utils][network]") {
    SECTION("Format MAC address") {
        std::string mac = "aabbccddeeff";
        std::string formatted = NetworkUtils::formatMacAddress(mac);
        REQUIRE(!formatted.empty());
    }
}

TEST_CASE("Utils NetworkUtils ByteOrderConversion", "[utils][network]") {
    SECTION("htons and ntohs") {
        uint16_t host = 0x1234;
        uint16_t net = NetworkUtils::htons(host);
        uint16_t back = NetworkUtils::ntohs(net);
        REQUIRE(host == back);
    }
    
    SECTION("htonl and ntohl") {
        uint32_t host = 0x12345678;
        uint32_t net = NetworkUtils::htonl(host);
        uint32_t back = NetworkUtils::ntohl(net);
        REQUIRE(host == back);
    }
}
