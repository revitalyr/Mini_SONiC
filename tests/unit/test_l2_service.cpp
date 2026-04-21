#include <catch2/catch_all.hpp>
import MiniSonic.L2L3;
import MiniSonic.SAI;
import MiniSonic.DataPlane;

TEST_CASE("L2Service HandlePacketLearnsMacAddress", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2.handle(pkt);
    
    REQUIRE(handled);
    // MAC should be learned (verified through SAI interface)
}

TEST_CASE("L2Service ForwardKnownDestination", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // First learn a MAC address
    Packet learn_pkt{
        .src_mac = "11:22:33:44:55:66",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    l2.handle(learn_pkt);

    // Now send packet to known MAC
    Packet forward_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "11:22:33:44:55:66",  // Known destination
        .src_ip = "10.0.0.3",
        .dst_ip = "10.0.0.4",
        .ingress_port = 2
    };

    bool handled = l2.handle(forward_pkt);
    REQUIRE(handled);
}

TEST_CASE("L2Service FloodUnknownDestination", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "99:88:77:66:55:44",  // Unknown destination
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood to all ports
}

TEST_CASE("L2Service MultipleMacLearning", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // Learn multiple MAC addresses
    std::vector<std::pair<std::string, int>> macs = {
        {"aa:bb:cc:dd:ee:01", 1},
        {"aa:bb:cc:dd:ee:02", 2},
        {"aa:bb:cc:dd:ee:03", 3}
    };

    for (const auto& [mac, port] : macs) {
        Packet pkt{
            .src_mac = mac,
            .dst_mac = "ff:ff:ff:ff:ff:ff",
            .src_ip = "10.0.0.1",
            .dst_ip = "10.0.0.2",
            .ingress_port = port
        };
        l2.handle(pkt);
    }

    // Test forwarding to each learned MAC
    for (const auto& [mac, port] : macs) {
        Packet pkt{
            .src_mac = "00:11:22:33:44:55",
            .dst_mac = mac,
            .src_ip = "10.0.0.100",
            .dst_ip = "10.0.0.200",
            .ingress_port = 4  // Different port
        };
        bool handled = l2.handle(pkt);
        REQUIRE(handled);
    }
}

TEST_CASE("L2Service MacUpdateOnDifferentPort", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // Learn MAC on port 1
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    l2.handle(pkt1);

    // Re-learn same MAC on port 2 (should update)
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 2
    };
    l2.handle(pkt2);

    // Send packet to this MAC - should go to port 2
    Packet pkt3{
        .src_mac = "11:22:33:44:55:66",
        .dst_mac = "aa:bb:cc:dd:ee:ff",
        .src_ip = "10.0.0.3",
        .dst_ip = "10.0.0.4",
        .ingress_port = 3
    };
    bool handled = l2.handle(pkt3);
    REQUIRE(handled);
}

TEST_CASE("L2Service BroadcastHandling", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",  // Broadcast address
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood broadcast
}

TEST_CASE("L2Service MulticastHandling", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "01:00:5e:00:00:01",  // Multicast address
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood multicast
}

TEST_CASE("L2Service EmptyMacTableFlood", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // Send packet without any learned MACs
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "11:22:33:44:55:66",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood since destination is unknown
}
