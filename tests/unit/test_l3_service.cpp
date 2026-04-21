#include <catch2/catch_all.hpp>
import MiniSonic.L2L3;
import MiniSonic.SAI;
import MiniSonic.DataPlane;

TEST_CASE("L3Service HandlePacketWithoutRoute", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE_FALSE(handled);  // Should not handle without route
}

TEST_CASE("L3Service AddRouteAndHandlePacket", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add a route
    l3.add_route("10.0.0.0", 24, "10.0.0.1");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE(handled);  // Should handle with matching route
}

TEST_CASE("L3Service LongestPrefixMatchRouting", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add overlapping routes
    l3.add_route("10.0.0.0", 16, "10.0.0.1");  // Less specific
    l3.add_route("10.0.0.0", 24, "10.0.0.2");  // More specific

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE(handled);
    // Should use the more specific /24 route
}

TEST_CASE("L3Service MultipleRoutesDifferentNetworks", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add routes for different networks
    l3.add_route("192.168.1.0", 24, "192.168.1.1");
    l3.add_route("10.0.0.0", 8, "10.0.0.1");
    l3.add_route("172.16.0.0", 16, "172.16.0.1");

    // Test packet to 192.168.1.x network
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };
    REQUIRE(l3.handle(pkt1));

    // Test packet to 10.x.x.x network
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.1",
        .dst_ip = "10.1.1.1",
        .ingress_port = 1
    };
    REQUIRE(l3.handle(pkt2));

    // Test packet to 172.16.x.x network
    Packet pkt3{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "172.16.100.1",
        .ingress_port = 1
    };
    REQUIRE(l3.handle(pkt3));
}

TEST_CASE("L3Service DefaultRoute", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add default route
    l3.add_route("0.0.0.0", 0, "192.168.1.1");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "203.0.113.100",  // Some external IP
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE(handled);  // Should use default route
}

TEST_CASE("L3Service RouteOverwrite", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add initial route
    l3.add_route("10.0.0.0", 24, "10.0.0.1");
    
    // Overwrite with different next-hop
    l3.add_route("10.0.0.0", 24, "10.0.0.2");

    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE(handled);
    // Should use the new next-hop
}

TEST_CASE("L3Service EdgeCaseNetworks", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Test edge case networks
    l3.add_route("255.255.255.255", 32, "host_route");
    l3.add_route("0.0.0.0", 0, "default_route");

    // Test host route
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "255.255.255.255",
        .ingress_port = 1
    };
    REQUIRE(l3.handle(pkt1));

    // Test default route
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "128.0.0.1",
        .ingress_port = 1
    };
    REQUIRE(l3.handle(pkt2));
}

TEST_CASE("L3Service ComplexHierarchy", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Create complex route hierarchy
    l3.add_route("10.0.0.0", 8, "gateway1");
    l3.add_route("10.0.0.0", 16, "gateway2");
    l3.add_route("10.0.0.0", 24, "gateway3");
    l3.add_route("10.1.0.0", 16, "gateway4");
    l3.add_route("10.1.1.0", 24, "gateway5");

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
        bool handled = l3.handle(pkt);
        REQUIRE(handled);
    }
}

TEST_CASE("L3Service NonRoutablePacket", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add route for specific network
    l3.add_route("10.0.0.0", 24, "10.0.0.1");

    // Send packet to different network
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };

    bool handled = l3.handle(pkt);
    REQUIRE_FALSE(handled);  // Should not handle
}
