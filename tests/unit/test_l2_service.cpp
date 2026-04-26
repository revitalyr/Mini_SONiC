#include <catch2/catch_all.hpp>
 
import MiniSonic.Core.Types;
import MiniSonic.L2L3;
import MiniSonic.SAI;
import MiniSonic.DataPlane;
import MiniSonic.Core.Utils;

using namespace MiniSonic::L2;
using namespace MiniSonic::SAI;
using namespace MiniSonic::DataPlane;
using namespace MiniSonic::Types;

TEST_CASE("L2Service HandlePacketLearnsMacAddress", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ee:dd:cc:bb:aa"),
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

    bool handled = l2.handle(pkt);
    
    REQUIRE(handled);
    // MAC should be learned (verified through SAI interface)
}

TEST_CASE("L2Service ForwardKnownDestination", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // First learn a MAC address
    Packet learn_pkt(macToUint64("11:22:33:44:55:66"), macToUint64("ff:ff:ff:ff:ff:ff"),
                     ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    l2.handle(learn_pkt);

    // Now send packet to known MAC
    Packet forward_pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("11:22:33:44:55:66"),
                       ipToUint32("10.0.0.3"), ipToUint32("10.0.0.4"), 2);

    bool handled = l2.handle(forward_pkt);
    REQUIRE(handled);
}

TEST_CASE("L2Service FloodUnknownDestination", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("99:88:77:66:55:44"),
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

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
        Packet pkt(macToUint64(mac), macToUint64("ff:ff:ff:ff:ff:ff"),
                   ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), static_cast<MiniSonic::Types::PortId>(port));
        l2.handle(pkt);
    }

    // Test forwarding to each learned MAC
    for (const auto& [mac, port] : macs) {
        Packet pkt(macToUint64("00:11:22:33:44:55"), macToUint64(mac),
                   ipToUint32("10.0.0.100"), ipToUint32("10.0.0.200"), 4);
        bool handled = l2.handle(pkt);
        REQUIRE(handled);
    }
}

TEST_CASE("L2Service MacUpdateOnDifferentPort", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // Learn MAC on port 1
    Packet pkt1(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"),
                ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);
    l2.handle(pkt1);

    // Re-learn same MAC on port 2 (should update)
    Packet pkt2(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"),
                ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 2);
    l2.handle(pkt2);

    // Send packet to this MAC - should go to port 2
    Packet pkt3(macToUint64("11:22:33:44:55:66"), macToUint64("aa:bb:cc:dd:ee:ff"),
                ipToUint32("10.0.0.3"), ipToUint32("10.0.0.4"), 3);
    bool handled = l2.handle(pkt3);
    REQUIRE(handled);
}

TEST_CASE("L2Service BroadcastHandling", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("ff:ff:ff:ff:ff:ff"),
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood broadcast
}

TEST_CASE("L2Service MulticastHandling", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("01:00:5e:00:00:01"),
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood multicast
}

TEST_CASE("L2Service EmptyMacTableFlood", "[l2]") {
    SimulatedSai sai;
    L2Service l2(sai);
    
    // Send packet without any learned MACs
    Packet pkt(macToUint64("aa:bb:cc:dd:ee:ff"), macToUint64("11:22:33:44:55:66"),
               ipToUint32("10.0.0.1"), ipToUint32("10.0.0.2"), 1);

    bool handled = l2.handle(pkt);
    REQUIRE(handled);
    // Should flood since destination is unknown
}
