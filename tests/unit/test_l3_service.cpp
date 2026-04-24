#include <catch2/catch_all.hpp>
#include "core/common/types.hpp" // Corrected include path
 
import MiniSonic.L2L3;
import MiniSonic.SAI;
import MiniSonic.DataPlane;

using MiniSonic::L3::L3Service;
using MiniSonic::DataPlane::Packet;
using MiniSonic::SAI::SimulatedSai;
using namespace MiniSonic::Types;

TEST_CASE("L3Service HandlePacketWithoutRoute", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.1"), ipToUint32("192.168.1.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE_FALSE(handled);  // Should not handle without route
}

TEST_CASE("L3Service AddRouteAndHandlePacket", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add a route
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop

    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.2"), ipToUint32("10.0.0.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE(handled);  // Should handle with matching route
}

TEST_CASE("L3Service LongestPrefixMatchRouting", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add overlapping routes
    l3.addRoute(ipToUint32("10.0.0.0"), 16, ipToUint32("10.0.0.1"));  // Less specific
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2"));  // More specific

    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.2"), ipToUint32("10.0.0.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE(handled);
    // Should use the more specific /24 route
}

TEST_CASE("L3Service MultipleRoutesDifferentNetworks", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add routes for different networks
    l3.addRoute(ipToUint32("192.168.1.0"), 24, ipToUint32("192.168.1.1"));
    l3.addRoute(ipToUint32("10.0.0.0"), 8, ipToUint32("10.0.0.1"));
    l3.addRoute(ipToUint32("172.16.0.0"), 16, ipToUint32("172.16.0.1"));

    // Test packet to 192.168.1.x network
    Packet pkt1(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                ipToUint32("10.0.0.1"), ipToUint32("192.168.1.100"), 1);
    REQUIRE(l3.handle(pkt1));

    // Test packet to 10.x.x.x network
    Packet pkt2(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                ipToUint32("192.168.1.1"), ipToUint32("10.1.1.1"), 1);
    REQUIRE(l3.handle(pkt2));

    // Test packet to 172.16.x.x network
    Packet pkt3(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                ipToUint32("10.0.0.1"), ipToUint32("172.16.100.1"), 1);
    REQUIRE(l3.handle(pkt3));
}

TEST_CASE("L3Service DefaultRoute", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add default route
    l3.addRoute(ipToUint32("0.0.0.0"), 0, ipToUint32("192.168.1.1"));

    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.1"), ipToUint32("203.0.113.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE(handled);  // Should use default route
}

TEST_CASE("L3Service RouteOverwrite", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add initial route
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));
    
    // Overwrite with different next-hop
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2"));

    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.2"), ipToUint32("10.0.0.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE(handled);
    // Should use the new next-hop
}

TEST_CASE("L3Service EdgeCaseNetworks", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Test edge case networks
    l3.addRoute(ipToUint32("255.255.255.255"), 32, ipToUint32("1.1.1.1"));
    l3.addRoute(ipToUint32("0.0.0.0"), 0, ipToUint32("0.0.0.0"));

    // Test host route
    Packet pkt1(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                ipToUint32("10.0.0.1"), ipToUint32("255.255.255.255"), 1);
    REQUIRE(l3.handle(pkt1));

    // Test default route
    Packet pkt2(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                ipToUint32("10.0.0.1"), ipToUint32("128.0.0.1"), 1);
    REQUIRE(l3.handle(pkt2));
}

TEST_CASE("L3Service ComplexHierarchy", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Create complex route hierarchy
    l3.addRoute(ipToUint32("10.0.0.0"), 8, ipToUint32("1.1.1.1"));
    l3.addRoute(ipToUint32("10.0.0.0"), 16, ipToUint32("2.2.2.2"));
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("3.3.3.3"));
    l3.addRoute(ipToUint32("10.1.0.0"), 16, ipToUint32("4.4.4.4"));
    l3.addRoute(ipToUint32("10.1.1.0"), 24, ipToUint32("5.5.5.5"));

    // Test various destinations
    struct TestCase {
        std::string dst_ip;
        uint32_t expected_next_hop;
    };

    std::vector<TestCase> test_cases = {
        {"10.0.0.1", ipToUint32("3.3.3.3")},   // Matches /24
        {"10.0.1.1", ipToUint32("2.2.2.2")},   // Matches /16
        {"10.1.0.1", ipToUint32("4.4.4.4")},   // Matches /16
        {"10.1.1.1", ipToUint32("5.5.5.5")},   // Matches /24
        {"10.2.0.1", ipToUint32("1.1.1.1")},   // Matches /8
    };

    for (const auto& test_case : test_cases) {
        Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
                   ipToUint32("192.168.1.1"), ipToUint32(test_case.dst_ip), 1);
        bool handled = l3.handle(pkt);
        REQUIRE(handled);
    }
}

TEST_CASE("L3Service NonRoutablePacket", "[l3]") {
    SimulatedSai sai;
    L3Service l3(sai);
    
    // Add route for specific network
    l3.addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    // Send packet to different network
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.1"), ipToUint32("192.168.1.100"), 1);

    bool handled = l3.handle(pkt);
    REQUIRE_FALSE(handled);  // Should not handle
}
