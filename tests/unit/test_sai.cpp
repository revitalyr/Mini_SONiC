#include <gtest/gtest.h>
#include "sai/sai_interface.h"
#include "sai/simulated_sai.h"

class SaiInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        sai_ = std::make_unique<SimulatedSai>();
    }

    std::unique_ptr<SimulatedSai> sai_;
};

TEST_F(SaiInterfaceTest, CreateAndRemovePort) {
    // Create port
    sai_->create_port(1);
    
    // Remove port
    sai_->remove_port(1);
    
    // Should not crash or throw
}

TEST_F(SaiInterfaceTest, CreateMultiplePorts) {
    // Create multiple ports
    sai_->create_port(1);
    sai_->create_port(2);
    sai_->create_port(3);
    sai_->create_port(10);
    sai_->create_port(24);
    
    // Remove all ports
    sai_->remove_port(1);
    sai_->remove_port(2);
    sai_->remove_port(3);
    sai_->remove_port(10);
    sai_->remove_port(24);
}

TEST_F(SaiInterfaceTest, AddAndRemoveFdbEntry) {
    // Add FDB entry
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:ff", 1);
    
    // Remove FDB entry
    sai_->remove_fdb_entry("aa:bb:cc:dd:ee:ff");
    
    // Should not crash or throw
}

TEST_F(SaiInterfaceTest, MultipleFdbEntries) {
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
        sai_->add_fdb_entry(mac, port);
    }
    
    // Remove all entries
    for (const auto& [mac, port] : entries) {
        sai_->remove_fdb_entry(mac);
    }
}

TEST_F(SaiInterfaceTest, AddAndRemoveRoute) {
    // Add route
    sai_->add_route("10.0.0.0/24", "10.0.0.1");
    
    // Remove route
    sai_->remove_route("10.0.0.0/24");
    
    // Should not crash or throw
}

TEST_F(SaiInterfaceTest, MultipleRoutes) {
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
        sai_->add_route(prefix, next_hop);
    }
    
    // Remove all routes
    for (const auto& [prefix, next_hop] : routes) {
        sai_->remove_route(prefix);
    }
}

TEST_F(SaiInterfaceTest, OverwriteFdbEntry) {
    // Add initial FDB entry
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:ff", 1);
    
    // Overwrite with different port
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:ff", 2);
    
    // Remove entry
    sai_->remove_fdb_entry("aa:bb:cc:dd:ee:ff");
}

TEST_F(SaiInterfaceTest, OverwriteRoute) {
    // Add initial route
    sai_->add_route("10.0.0.0/24", "10.0.0.1");
    
    // Overwrite with different next-hop
    sai_->add_route("10.0.0.0/24", "10.0.0.2");
    
    // Remove route
    sai_->remove_route("10.0.0.0/24");
}

TEST_F(SaiInterfaceTest, RemoveNonExistentEntries) {
    // Try to remove non-existent entries (should not crash)
    sai_->remove_port(999);
    sai_->remove_fdb_entry("11:22:33:44:55:66");
    sai_->remove_route("192.168.100.0/24");
}

TEST_F(SaiInterfaceTest, ComplexMacAddresses) {
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
        sai_->add_fdb_entry(mac, 1);
        sai_->remove_fdb_entry(mac);
    }
}

TEST_F(SaiInterfaceTest, ComplexRoutePrefixes) {
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
        sai_->add_route(prefix, "192.168.1.1");
        sai_->remove_route(prefix);
    }
}

TEST_F(SaiInterfaceTest, MixedOperations) {
    // Perform mixed operations to test state management
    
    // Create ports
    sai_->create_port(1);
    sai_->create_port(2);
    sai_->create_port(3);
    
    // Add FDB entries
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:01", 1);
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:02", 2);
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:03", 3);
    
    // Add routes
    sai_->add_route("10.0.0.0/24", "10.0.0.1");
    sai_->add_route("192.168.1.0/24", "192.168.1.1");
    
    // Update some entries
    sai_->add_fdb_entry("aa:bb:cc:dd:ee:01", 2);  // Move to different port
    sai_->add_route("10.0.0.0/24", "10.0.0.2");   // Change next-hop
    
    // Remove some entries
    sai_->remove_fdb_entry("aa:bb:cc:dd:ee:03");
    sai_->remove_route("192.168.1.0/24");
    
    // Clean up
    sai_->remove_fdb_entry("aa:bb:cc:dd:ee:01");
    sai_->remove_fdb_entry("aa:bb:cc:dd:ee:02");
    sai_->remove_route("10.0.0.0/24");
    sai_->remove_port(1);
    sai_->remove_port(2);
    sai_->remove_port(3);
}

TEST_F(SaiInterfaceTest, LargeScaleOperations) {
    // Test with larger number of entries
    
    // Create many ports
    for (int i = 1; i <= 48; ++i) {
        sai_->create_port(i);
    }
    
    // Add many FDB entries
    for (int i = 1; i <= 1000; ++i) {
        std::string mac = "aa:bb:cc:dd:ee:" + 
                         std::string(i < 16 ? "0" : "") + 
                         std::string(i < 256 ? "0" : "") + 
                         std::to_string(i % 256);
        sai_->add_fdb_entry(mac, i % 48 + 1);
    }
    
    // Add many routes
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        std::string next_hop = "10." + std::to_string(i) + ".0.1";
        sai_->add_route(prefix, next_hop);
    }
    
    // Clean up (simplified - in real test would track all entries)
    for (int i = 0; i < 100; ++i) {
        std::string prefix = "10." + std::to_string(i) + ".0.0/16";
        sai_->remove_route(prefix);
    }
    
    for (int i = 1; i <= 48; ++i) {
        sai_->remove_port(i);
    }
}
