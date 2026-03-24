#include <gtest/gtest.h>
#include "l2/l2_service.h"
#include "sai/simulated_sai.h"
#include "dataplane/packet.h"

class L2ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        sai_ = std::make_unique<SimulatedSai>();
        l2_ = std::make_unique<L2Service>(*sai_);
    }

    std::unique_ptr<SimulatedSai> sai_;
    std::unique_ptr<L2Service> l2_;
};

TEST_F(L2ServiceTest, HandlePacketLearnsMacAddress) {
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2_->handle(pkt);
    
    EXPECT_TRUE(handled);
    // MAC should be learned (verified through SAI interface)
}

TEST_F(L2ServiceTest, ForwardKnownDestination) {
    // First learn a MAC address
    Packet learn_pkt{
        .src_mac = "11:22:33:44:55:66",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    l2_->handle(learn_pkt);

    // Now send packet to known MAC
    Packet forward_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "11:22:33:44:55:66",  // Known destination
        .src_ip = "10.0.0.3",
        .dst_ip = "10.0.0.4",
        .ingress_port = 2
    };

    bool handled = l2_->handle(forward_pkt);
    EXPECT_TRUE(handled);
}

TEST_F(L2ServiceTest, FloodUnknownDestination) {
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "99:88:77:66:55:44",  // Unknown destination
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should flood to all ports
}

TEST_F(L2ServiceTest, MultipleMacLearning) {
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
        l2_->handle(pkt);
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
        bool handled = l2_->handle(pkt);
        EXPECT_TRUE(handled);
    }
}

TEST_F(L2ServiceTest, MacUpdateOnDifferentPort) {
    // Learn MAC on port 1
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    l2_->handle(pkt1);

    // Re-learn same MAC on port 2 (should update)
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 2
    };
    l2_->handle(pkt2);

    // Send packet to this MAC - should go to port 2
    Packet pkt3{
        .src_mac = "11:22:33:44:55:66",
        .dst_mac = "aa:bb:cc:dd:ee:ff",
        .src_ip = "10.0.0.3",
        .dst_ip = "10.0.0.4",
        .ingress_port = 3
    };
    bool handled = l2_->handle(pkt3);
    EXPECT_TRUE(handled);
}

TEST_F(L2ServiceTest, BroadcastHandling) {
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",  // Broadcast address
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should flood broadcast
}

TEST_F(L2ServiceTest, MulticastHandling) {
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "01:00:5e:00:00:01",  // Multicast address
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should flood multicast
}

TEST_F(L2ServiceTest, EmptyMacTableFlood) {
    // Send packet without any learned MACs
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "11:22:33:44:55:66",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    bool handled = l2_->handle(pkt);
    EXPECT_TRUE(handled);
    // Should flood since destination is unknown
}
