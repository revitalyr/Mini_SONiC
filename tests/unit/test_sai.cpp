#include <catch2/catch_all.hpp>
 
import MiniSonic.Core.Types;
import MiniSonic.SAI;

using MiniSonic::SAI::SimulatedSai;
using namespace MiniSonic::Types;

TEST_CASE("SaiInterface CreateAndRemovePort", "[sai]") {
    SimulatedSai sai;
    // Create port
    sai.createPort(1);

    // Remove port
    sai.deletePort(1);

    // Should not crash or throw
}

TEST_CASE("SaiInterface CreateMultiplePorts", "[sai]") {
    SimulatedSai sai;
    // Create multiple ports
    sai.createPort(1);
    sai.createPort(2);
    sai.createPort(3);
    sai.createPort(10);
    sai.createPort(24);

    // Remove all ports
    sai.deletePort(1);
    sai.deletePort(2);
    sai.deletePort(3);
    sai.deletePort(10);
    sai.deletePort(24);
}

TEST_CASE("SaiInterface AddAndRemoveFdbEntry", "[sai]") {
    SimulatedSai sai;
    // Add FDB entry
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:ff"), 1);

    // Remove FDB entry
    sai.removeFdbEntry(macToUint64("aa:bb:cc:dd:ee:ff"));

    // Should not crash or throw
}

TEST_CASE("SaiInterface MultipleFdbEntries", "[sai]") {
    SimulatedSai sai;
    // Add multiple FDB entries
    std::vector<std::pair<std::string, int>> entries = {
        {"aa:bb:cc:dd:ee:01", 1},
        {"aa:bb:cc:dd:ee:02", 2},
        {"aa:bb:cc:dd:ee:03", 3},
        {"11:22:33:44:55:66", 10},
        {"ff:ee:dd:cc:bb:aa", 24}
    };

    // Add all entries
    for (const auto& [mac, port] : entries) {
        sai.addFdbEntry(macToUint64(mac), port);
    }

    // Remove all entries
    for (const auto& [mac, port] : entries) {
        sai.removeFdbEntry(macToUint64(mac));
    }
}

TEST_CASE("SaiInterface AddAndRemoveRoute", "[sai]") {
    SimulatedSai sai;
    // Add route
    sai.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    // Remove route
    sai.removeRoute(ipToUint32("10.0.0.0"));

    // Should not crash or throw
}

TEST_CASE("SaiInterface MultipleRoutes", "[sai]") {
    SimulatedSai sai;
    // Add multiple routes
    std::vector<std::pair<std::string, std::string>> routes = {
        {"10.0.0.0/24", "10.0.0.1"},
        {"192.168.1.0/24", "192.168.1.1"},
        {"172.16.0.0/16", "172.16.0.1"},
        {"0.0.0.0/0", "192.168.1.254"},
        {"10.1.1.0/24", "10.1.1.1"}
    };

    // Add all routes
    for (const auto& [prefix, next_hop] : routes) {
        sai.addRoute(ipToUint32(prefix), 24, ipToUint32(next_hop));
    }

    // Remove all routes
    for (const auto& [prefix, next_hop] : routes) {
        sai.removeRoute(ipToUint32(prefix));
    }
}

TEST_CASE("SaiInterface OverwriteFdbEntry", "[sai]") {
    SimulatedSai sai;
    // Add initial FDB entry
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:ff"), 1);

    // Overwrite with different port
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:ff"), 2);

    // Remove entry
    sai.removeFdbEntry(macToUint64("aa:bb:cc:dd:ee:ff"));
}

TEST_CASE("SaiInterface OverwriteRoute", "[sai]") {
    SimulatedSai sai;
    // Add initial route
    sai.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    // Overwrite with different next-hop
    sai.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2"));

    // Remove route
    sai.removeRoute(ipToUint32("10.0.0.0"));
}

TEST_CASE("SaiInterface RemoveNonExistentEntries", "[sai]") {
    SimulatedSai sai;
    // Try to remove non-existent entries (should not crash)
    sai.deletePort(999);
    sai.removeFdbEntry(macToUint64("11:22:33:44:55:66"));
    sai.removeRoute(ipToUint32("192.168.100.0"));
}

TEST_CASE("SaiInterface ComplexMacAddresses", "[sai]") {
    SimulatedSai sai;
    // Test various MAC address formats
    std::vector<std::string> macs = {
        "aa:bb:cc:dd:ee:ff",
        "00:00:00:00:00:00",
        "ff:ff:ff:ff:ff:ff",
        "01:00:5e:00:00:01",  // Multicast
        "11:22:33:44:55:66",
        "aa:bb:cc:dd:ee:01"
    };

    for (const auto& mac : macs) {
        sai.addFdbEntry(macToUint64(mac), 1);
        sai.removeFdbEntry(macToUint64(mac));
    }
}

TEST_CASE("SaiInterface ComplexRoutePrefixes", "[sai]") {
    SimulatedSai sai;
    // Test various route prefixes
    std::vector<std::string> prefixes = {
        "0.0.0.0/0",           // Default route
        "255.255.255.255/32",  // Host route
        "10.0.0.0/8",         // Class A
        "172.16.0.0/12",      // Private B
        "192.168.0.0/16",     // Private C
        "224.0.0.0/4",        // Multicast
        "10.0.0.0/24",        // Specific /24
        "192.0.2.0/24"        // Documentation
    };

    for (const auto& prefix : prefixes) {
        sai.addRoute(ipToUint32(prefix), 24, ipToUint32("192.168.1.1"));
        sai.removeRoute(ipToUint32(prefix));
    }
}

TEST_CASE("SaiInterface MixedOperations", "[sai]") {
    SimulatedSai sai;
    // Perform mixed operations to test state management

    // Create ports
    sai.createPort(1);
    sai.createPort(2);
    sai.createPort(3);

    // Add FDB entries
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:01"), 1);
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:02"), 2);
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:03"), 3);

    // Add routes
    sai.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));
    sai.addRoute(ipToUint32("192.168.1.0"), 24, ipToUint32("192.168.1.1"));

    // Update some entries
    sai.addFdbEntry(macToUint64("aa:bb:cc:dd:ee:01"), 2);  // Move to different port
    sai.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2"));   // Change next-hop

    // Remove some entries
    sai.removeFdbEntry(macToUint64("aa:bb:cc:dd:ee:03"));
    sai.removeRoute(ipToUint32("192.168.1.0"));

    // Clean up
    sai.removeFdbEntry(macToUint64("aa:bb:cc:dd:ee:01"));
    sai.removeFdbEntry(macToUint64("aa:bb:cc:dd:ee:02"));
    sai.removeRoute(ipToUint32("10.0.0.0"));
    sai.deletePort(1);
    sai.deletePort(2);
    sai.deletePort(3);
}

TEST_CASE("SaiInterface LargeScaleOperations", "[sai]") {
    SimulatedSai sai;
    // Test with larger number of entries

    // Create many ports
    for (int i = 1; i <= 48; ++i) {
        sai.createPort(static_cast<MiniSonic::Types::PortId>(i));
    }

    // Add many FDB entries
    for (int i = 1; i <= 1000; ++i) {
        std::string mac = "aa:bb:cc:dd:ee:" +
                         std::string(i < 16 ? "0" : "") +
                         std::string(i < 256 ? "0" : "") +
                         std::to_string(i % 256);
        sai.addFdbEntry(macToUint64(mac), static_cast<MiniSonic::Types::PortId>(i % 48 + 1));
    }

    // Add many routes
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        std::string next_hop = "10." + std::to_string(i) + ".0.1";
        sai.addRoute(ipToUint32(prefix), static_cast<MiniSonic::Types::PortId>(i), ipToUint32(next_hop));
        sai.addRoute(ipToUint32(prefix), 16, ipToUint32(next_hop));
    }

    // Clean up (simplified - in real test would track all entries)
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        sai.removeRoute(ipToUint32(prefix));
    }

    for (int i = 1; i <= 24; ++i) {
        sai.deletePort(static_cast<MiniSonic::Types::PortId>(i));
    }
}
