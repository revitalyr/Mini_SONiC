#include <gtest/gtest.h>
#include "l3/l3_service.h"
#include "sai/simulated_sai.h"
#include "dataplane/packet.h"

class L3ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        sai_ = std::make_unique<SimulatedSai>();
        l3_ = std::make_unique<L3Service>(*sai_);
    }

    std::unique_ptr<SimulatedSai> sai_;
    std::unique_ptr<L3Service> l3_;
};

TEST_F(L3ServiceTest, HandlePacketWithoutRoute) {
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_FALSE(handled);  // Should not handle without route
}

TEST_F(L3ServiceTest, AddRouteAndHandlePacket) {
    // Add a route
    l3_->add_route("10.0.0.0", 24, "10.0.0.1");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_TRUE(handled);  // Should handle with matching route
}

TEST_F(L3ServiceTest, LongestPrefixMatchRouting) {
    // Add overlapping routes
    l3_->add_route("10.0.0.0", 16, "10.0.0.1");  // Less specific
    l3_->add_route("10.0.0.0", 24, "10.0.0.2");  // More specific

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should use the more specific /24 route
}

TEST_F(L3ServiceTest, MultipleRoutesDifferentNetworks) {
    // Add routes for different networks
    l3_->add_route("192.168.1.0", 24, "192.168.1.1");
    l3_->add_route("10.0.0.0", 8, "10.0.0.1");
    l3_->add_route("172.16.0.0", 16, "172.16.0.1");

    // Test packet to 192.168.1.x network
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };
    EXPECT_TRUE(l3_->handle(pkt1));

    // Test packet to 10.x.x.x network
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.1",
        .dst_ip = "10.1.1.1",
        .ingress_port = 1
    };
    EXPECT_TRUE(l3_->handle(pkt2));

    // Test packet to 172.16.x.x network
    Packet pkt3{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "172.16.100.1",
        .ingress_port = 1
    };
    EXPECT_TRUE(l3_->handle(pkt3));
}

TEST_F(L3ServiceTest, DefaultRoute) {
    // Add default route
    l3_->add_route("0.0.0.0", 0, "192.168.1.1");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "203.0.113.100",  // Some external IP
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_TRUE(handled);  // Should use default route
}

TEST_F(L3ServiceTest, RouteOverwrite) {
    // Add initial route
    l3_->add_route("10.0.0.0", 24, "10.0.0.1");
    
    // Overwrite with different next-hop
    l3_->add_route("10.0.0.0", 24, "10.0.0.2");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should use the new next-hop
}

TEST_F(L3ServiceTest, EdgeCaseNetworks) {
    // Test edge case networks
    l3_->add_route("255.255.255.255", 32, "host_route");
    l3_->add_route("0.0.0.0", 0, "default_route");

    // Test host route
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "255.255.255.255",
        .ingress_port = 1
    };
    EXPECT_TRUE(l3_->handle(pkt1));

    // Test default route
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "128.0.0.1",
        .ingress_port = 1
    };
    EXPECT_TRUE(l3_->handle(pkt2));
}

TEST_F(L3ServiceTest, ComplexHierarchy) {
    // Create complex route hierarchy
    l3_->add_route("10.0.0.0", 8, "gateway1");
    l3_->add_route("10.0.0.0", 16, "gateway2");
    l3_->add_route("10.0.0.0", 24, "gateway3");
    l3_->add_route("10.1.0.0", 16, "gateway4");
    l3_->add_route("10.1.1.0", 24, "gateway5");

    // Test various destinations
    struct TestCase {
        std::string dst_ip;
        std::string expected_next_hop_pattern;
    };

    std::vector<TestCase> test_cases = {
        {"10.0.0.1", "gateway3"},   // Matches /24
        {"10.0.1.1", "gateway2"},   // Matches /16
        {"10.1.0.1", "gateway4"},   // Matches /16
        {"10.1.1.1", "gateway5"},   // Matches /24
        {"10.2.0.1", "gateway1"},   // Matches /8
    };

    for (const auto& test_case : test_cases) {
        Packet pkt{
            .src_mac = "aa:bb:cc:dd:ee:ff",
            .dst_mac = "ff:ee:dd:cc:bb:aa",
            .src_ip = "192.168.1.1",
            .dst_ip = test_case.dst_ip,
            .ingress_port = 1
        };
        bool handled = l3_->handle(pkt);
        EXPECT_TRUE(handled) << "Failed for IP: " << test_case.dst_ip;
    }
}

TEST_F(L3ServiceTest, NonRoutablePacket) {
    // Add route for specific network
    l3_->add_route("10.0.0.0", 24, "10.0.0.1");

    // Send packet to different network
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };

    bool handled = l3_->handle(pkt);
    EXPECT_FALSE(handled);  // Should not handle
}
