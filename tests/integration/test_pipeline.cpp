#include <catch2/catch_all.hpp>
#include "core/common/types.hpp" // Corrected include path
 
import MiniSonic.SAI;
import MiniSonic.DataPlane;

using namespace MiniSonic::SAI;
using namespace MiniSonic::DataPlane;
using namespace MiniSonic::Types;

TEST_CASE("Pipeline L2ForwardingScenario", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Learn MAC addresses
    Packet learn_pkt1(macToUint64("aa:bb:cc:dd:ee:01"), macToUint64("ff:ff:ff:ff:ff:ff"), 
                      ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    pipeline.process(learn_pkt1);

    Packet learn_pkt2(macToUint64("aa:bb:cc:dd:ee:02"), macToUint64("ff:ff:ff:ff:ff:ff"), 
                      ipToUint32("10.0.0.2"), ipToUint32("10.0.0.1"), 2);
    pipeline.process(learn_pkt2);

    // Test forwarding between learned MACs
    Packet forward_pkt(macToUint64("aa:bb:cc:dd:ee:01"), macToUint64("aa:bb:cc:dd:ee:02"), 
                       ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

    // Should be handled by L2 (forwarded)
    pipeline.process(forward_pkt);
}

TEST_CASE("Pipeline L3RoutingScenario", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Add L3 routes
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop
    pipeline.getL3().addRoute(ipToUint32("192.168.1.0"), 24, ipToUint32("192.168.1.1")); // Use ipToUint32 for next_hop

    // Test L3 packet to known route
    Packet l3_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"), 
                  ipToUint32("10.0.0.100"), ipToUint32("10.0.0.200"), 1);

    // Should be handled by L3 (routed)
    pipeline.process(l3_pkt);
}

TEST_CASE("Pipeline L2FallsBackToL3", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Add L3 route
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop

    // Send packet with unknown destination MAC (should flood in L2, then try L3)
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("99:88:77:66:55:44"), 
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.100"), 1);

    // Should be processed by pipeline
    pipeline.process(pkt);
}

TEST_CASE("Pipeline PacketDropScenario", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Send packet with no matching L2 or L3 entries
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("99:88:77:66:55:44"), 
               ipToUint32("10.0.0.1"), ipToUint32("192.168.100.100"), 1);

    // Should be dropped by pipeline
    pipeline.process(pkt);
}

TEST_CASE("Pipeline MixedTrafficScenario", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Setup: Learn some MACs and add some routes
    Packet learn_pkt(macToUint64("11:22:33:44:55:66"), macToUint64("ff:ff:ff:ff:ff:ff"), 
                     ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    pipeline.process(learn_pkt);

    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop

    // Test 1: L2 forwarding to known MAC
    Packet l2_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("11:22:33:44:55:66"), 
                  ipToUint32("10.0.0.100"), ipToUint32("10.0.0.200"), 2);
    pipeline.process(l2_pkt);

    // Test 2: L3 routing
    Packet l3_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"), 
                  ipToUint32("10.0.0.100"), ipToUint32("10.0.0.200"), 2);
    pipeline.process(l3_pkt);

    // Test 3: Unknown destination (should be dropped)
    Packet drop_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("99:88:77:66:55:44"), 
                    ipToUint32("10.0.0.100"), ipToUint32("172.16.1.100"), 2);
    pipeline.process(drop_pkt);
}

TEST_CASE("Pipeline BroadcastTraffic", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Learn a MAC first
    Packet learn_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"), 
                     ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    pipeline.process(learn_pkt);

    // Send broadcast packet
    Packet broadcast_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"), 
                         ipToUint32("10.0.0.1"), ipToUint32("10.0.0.255"), 1);

    // Should be flooded by L2
    pipeline.process(broadcast_pkt);
}

TEST_CASE("Pipeline MulticastTraffic", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Send multicast packet
    Packet multicast_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("01:00:5e:00:00:01"), 
                         ipToUint32("10.0.0.1"), ipToUint32("224.0.0.1"), 1);

    // Should be flooded by L2
    pipeline.process(multicast_pkt);
}

TEST_CASE("Pipeline HighVolumePackets", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Setup routes
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop
    pipeline.getL3().addRoute(ipToUint32("192.168.1.0"), 24, ipToUint32("192.168.1.1")); // Use ipToUint32 for next_hop

    // Process many packets to test performance
    for (int i = 0; i < 1000; ++i) {
        Packet pkt(macToUint64("aa:bb:cc:dd:ee:" + std::string(i < 16 ? "0" : "") + std::to_string(i % 16)),
                   macToUint64("ff:ee:dd:cc:bb:" + std::string(i < 16 ? "0" : "") + std::to_string(i % 16)),
                   ipToUint32("10.0.0." + std::to_string((i % 254) + 1)),
                   ipToUint32((i % 2 == 0) ? "10.0.0.100" : "192.168.1.100"), static_cast<PortId>((i % 4) + 1));
        pipeline.process(pkt);
    }
}

TEST_CASE("Pipeline ComplexRoutingScenario", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Setup complex routing table
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 16, ipToUint32("10.0.0.1")); // Use ipToUint32 for next_hop
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2")); // Use ipToUint32 for next_hop
    pipeline.getL3().addRoute(ipToUint32("10.1.0.0"), 16, ipToUint32("10.1.0.1")); // Use ipToUint32 for next_hop
    pipeline.getL3().addRoute(ipToUint32("192.168.1.0"), 24, ipToUint32("192.168.1.1")); // Use ipToUint32 for next_hop

    // Test various destinations
    std::vector<std::string> test_destinations = {
        "10.0.0.100",  // Should match /24
        "10.0.1.100",  // Should match /16
        "10.1.100.1",  // Should match /16
        "192.168.1.100" // Should match /24
    };

    for (const auto& dst_ip : test_destinations) {
        Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"), 
                   ipToUint32("172.16.0.1"), ipToUint32(dst_ip), 1);
        pipeline.process(pkt);
    }
}

TEST_CASE("Pipeline StateConsistency", "[pipeline][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Test that pipeline maintains consistent state across multiple operations
    
    // Learn MAC
    Packet learn_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"),
                     ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    pipeline.process(learn_pkt);

    // Add route
    pipeline.getL3().addRoute(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    // Send multiple packets
    for (int i = 0; i < 10; ++i) {
        Packet pkt(macToUint64("11:22:33:44:55:66"), macToUint64("aa:bb:cc:dd:ee:ff"),
                   ipToUint32("10.0.0." + std::to_string(i + 1)), ipToUint32("10.0.0.100"), 2);
        pipeline.process(pkt);
    }

    // Verify state is still consistent
    Packet test_pkt(macToUint64("22:33:44:55:66:77"), macToUint64("aa:bb:cc:dd:ee:ff"),
                    ipToUint32("10.0.0.50"), ipToUint32("10.0.0.100"), 3);
    pipeline.process(test_pkt);
}
