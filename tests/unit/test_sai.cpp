#include <catch2/catch_all.hpp>
#include "sai/sai_interface.h"
#include "sai/simulated_sai.h"

TEST_CASE("SaiInterface CreateAndRemovePort", "[sai]") {
    SimulatedSai sai;
    // Create port
    sai.create_port(1);

    // Remove port
    sai.remove_port(1);

    // Should not crash or throw
}

TEST_CASE("SaiInterface CreateMultiplePorts", "[sai]") {
    SimulatedSai sai;
    // Create multiple ports
    sai.create_port(1);
    sai.create_port(2);
    sai.create_port(3);
    sai.create_port(10);
    sai.create_port(24);

    // Remove all ports
    sai.remove_port(1);
    sai.remove_port(2);
    sai.remove_port(3);
    sai.remove_port(10);
    sai.remove_port(24);
}

TEST_CASE("SaiInterface AddAndRemoveFdbEntry", "[sai]") {
    SimulatedSai sai;
    // Add FDB entry
    sai.add_fdb_entry("aa:bb:cc:dd:ee:ff", 1);

    // Remove FDB entry
    sai.remove_fdb_entry("aa:bb:cc:dd:ee:ff");

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
        sai.add_fdb_entry(mac, port);
    }

    // Remove all entries
    for (const auto& [mac, port] : entries) {
        sai.remove_fdb_entry(mac);
    }
}

TEST_CASE("SaiInterface AddAndRemoveRoute", "[sai]") {
    SimulatedSai sai;
    // Add route
    sai.add_route("10.0.0.0/24", "10.0.0.1");

    // Remove route
    sai.remove_route("10.0.0.0/24");

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
        sai.add_route(prefix, next_hop);
    }

    // Remove all routes
    for (const auto& [prefix, next_hop] : routes) {
        sai.remove_route(prefix);
    }
}

TEST_CASE("SaiInterface OverwriteFdbEntry", "[sai]") {
    SimulatedSai sai;
    // Add initial FDB entry
    sai.add_fdb_entry("aa:bb:cc:dd:ee:ff", 1);

    // Overwrite with different port
    sai.add_fdb_entry("aa:bb:cc:dd:ee:ff", 2);

    // Remove entry
    sai.remove_fdb_entry("aa:bb:cc:dd:ee:ff");
}

TEST_CASE("SaiInterface OverwriteRoute", "[sai]") {
    SimulatedSai sai;
    // Add initial route
    sai.add_route("10.0.0.0/24", "10.0.0.1");

    // Overwrite with different next-hop
    sai.add_route("10.0.0.0/24", "10.0.0.2");

    // Remove route
    sai.remove_route("10.0.0.0/24");
}

TEST_CASE("SaiInterface RemoveNonExistentEntries", "[sai]") {
    SimulatedSai sai;
    // Try to remove non-existent entries (should not crash)
    sai.remove_port(999);
    sai.remove_fdb_entry("11:22:33:44:55:66");
    sai.remove_route("192.168.100.0/24");
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
        sai.add_fdb_entry(mac, 1);
        sai.remove_fdb_entry(mac);
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
        sai.add_route(prefix, "192.168.1.1");
        sai.remove_route(prefix);
    }
}

TEST_CASE("SaiInterface MixedOperations", "[sai]") {
    SimulatedSai sai;
    // Perform mixed operations to test state management

    // Create ports
    sai.create_port(1);
    sai.create_port(2);
    sai.create_port(3);

    // Add FDB entries
    sai.add_fdb_entry("aa:bb:cc:dd:ee:01", 1);
    sai.add_fdb_entry("aa:bb:cc:dd:ee:02", 2);
    sai.add_fdb_entry("aa:bb:cc:dd:ee:03", 3);

    // Add routes
    sai.add_route("10.0.0.0/24", "10.0.0.1");
    sai.add_route("192.168.1.0/24", "192.168.1.1");

    // Update some entries
    sai.add_fdb_entry("aa:bb:cc:dd:ee:01", 2);  // Move to different port
    sai.add_route("10.0.0.0/24", "10.0.0.2");   // Change next-hop

    // Remove some entries
    sai.remove_fdb_entry("aa:bb:cc:dd:ee:03");
    sai.remove_route("192.168.1.0/24");

    // Clean up
    sai.remove_fdb_entry("aa:bb:cc:dd:ee:01");
    sai.remove_fdb_entry("aa:bb:cc:dd:ee:02");
    sai.remove_route("10.0.0.0/24");
    sai.remove_port(1);
    sai.remove_port(2);
    sai.remove_port(3);
}

TEST_CASE("SaiInterface LargeScaleOperations", "[sai]") {
    SimulatedSai sai;
    // Test with larger number of entries

    // Create many ports
    for (int i = 1; i <= 48; ++i) {
        sai.create_port(i);
    }

    // Add many FDB entries
    for (int i = 1; i <= 1000; ++i) {
        std::string mac = "aa:bb:cc:dd:ee:" +
                         std::string(i < 16 ? "0" : "") +
                         std::string(i < 256 ? "0" : "") +
                         std::to_string(i % 256);
        sai.add_fdb_entry(mac, i % 48 + 1);
    }

    // Add many routes
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        std::string next_hop = "10." + std::to_string(i) + ".0.1";
        sai.add_route(prefix, next_hop);
    }

    // Clean up (simplified - in real test would track all entries)
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        sai.remove_route(prefix);
    }

    for (int i = 1; i <= 48; ++i) {
        sai.remove_port(i);
    }
}
