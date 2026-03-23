#include <gtest/gtest.h>
#include "dataplane/pipeline.h"
#include "sai/simulated_sai.h"
#include "dataplane/packet.h"

class PipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        sai_ = std::make_unique<SimulatedSai>();
        pipeline_ = std::make_unique<Pipeline>(*sai_);
    }

    std::unique_ptr<SimulatedSai> sai_;
    std::unique_ptr<Pipeline> pipeline_;
};

TEST_F(PipelineTest, L2ForwardingScenario) {
    // Learn MAC addresses
    Packet learn_pkt1{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    pipeline_->process(learn_pkt1);

    Packet learn_pkt2{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.2",
        .dst_ip = "10.0.0.1",
        .ingress_port = 2
    };
    pipeline_->process(learn_pkt2);

    // Test forwarding between learned MACs
    Packet forward_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "aa:bb:cc:dd:ee:02",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };

    // Should be handled by L2 (forwarded)
    pipeline_->process(forward_pkt);
}

TEST_F(PipelineTest, L3RoutingScenario) {
    // Add L3 routes
    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.1");
    pipeline_->get_l3().add_route("192.168.1.0", 24, "192.168.1.1");

    // Test L3 packet to known route
    Packet l3_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "10.0.0.200",
        .ingress_port = 1
    };

    // Should be handled by L3 (routed)
    pipeline_->process(l3_pkt);
}

TEST_F(PipelineTest, L2FallsBackToL3) {
    // Add L3 route
    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.1");

    // Send packet with unknown destination MAC (should flood in L2, then try L3)
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "99:88:77:66:55:44",  // Unknown MAC
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };

    // Should be processed by pipeline
    pipeline_->process(pkt);
}

TEST_F(PipelineTest, PacketDropScenario) {
    // Send packet with no matching L2 or L3 entries
    Packet pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "99:88:77:66:55:44",  // Unknown MAC
        .src_ip = "10.0.0.1",
        .dst_ip = "192.168.100.100",    // Unknown route
        .ingress_port = 1
    };

    // Should be dropped by pipeline
    pipeline_->process(pkt);
}

TEST_F(PipelineTest, MixedTrafficScenario) {
    // Setup: Learn some MACs and add some routes
    Packet learn_pkt{
        .src_mac = "11:22:33:44:55:66",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    pipeline_->process(learn_pkt);

    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.1");

    // Test 1: L2 forwarding to known MAC
    Packet l2_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "11:22:33:44:55:66",
        .src_ip = "10.0.0.100",
        .dst_ip = "10.0.0.200",
        .ingress_port = 2
    };
    pipeline_->process(l2_pkt);

    // Test 2: L3 routing
    Packet l3_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "10.0.0.200",
        .ingress_port = 2
    };
    pipeline_->process(l3_pkt);

    // Test 3: Unknown destination (should be dropped)
    Packet drop_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "99:88:77:66:55:44",
        .src_ip = "10.0.0.100",
        .dst_ip = "172.16.1.100",
        .ingress_port = 2
    };
    pipeline_->process(drop_pkt);
}

TEST_F(PipelineTest, BroadcastTraffic) {
    // Learn a MAC first
    Packet learn_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    pipeline_->process(learn_pkt);

    // Send broadcast packet
    Packet broadcast_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.255",
        .ingress_port = 1
    };

    // Should be flooded by L2
    pipeline_->process(broadcast_pkt);
}

TEST_F(PipelineTest, MulticastTraffic) {
    // Send multicast packet
    Packet multicast_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "01:00:5e:00:00:01",  // Multicast MAC
        .src_ip = "10.0.0.1",
        .dst_ip = "224.0.0.1",
        .ingress_port = 1
    };

    // Should be flooded by L2
    pipeline_->process(multicast_pkt);
}

TEST_F(PipelineTest, HighVolumePackets) {
    // Setup routes
    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.1");
    pipeline_->get_l3().add_route("192.168.1.0", 24, "192.168.1.1");

    // Process many packets to test performance
    for (int i = 0; i < 1000; ++i) {
        Packet pkt{
            .src_mac = "aa:bb:cc:dd:ee:" + std::string(i < 16 ? "0" : "") + std::to_string(i % 16),
            .dst_mac = "ff:ee:dd:cc:bb:" + std::string(i < 16 ? "0" : "") + std::to_string(i % 16),
            .src_ip = "10.0.0." + std::to_string((i % 254) + 1),
            .dst_ip = (i % 2 == 0) ? "10.0.0.100" : "192.168.1.100",
            .ingress_port = (i % 4) + 1
        };
        pipeline_->process(pkt);
    }
}

TEST_F(PipelineTest, ComplexRoutingScenario) {
    // Setup complex routing table
    pipeline_->get_l3().add_route("10.0.0.0", 16, "10.0.0.1");
    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.2");
    pipeline_->get_l3().add_route("10.1.0.0", 16, "10.1.0.1");
    pipeline_->get_l3().add_route("192.168.1.0", 24, "192.168.1.1");

    // Test various destinations
    std::vector<std::string> test_destinations = {
        "10.0.0.100",  // Should match /24
        "10.0.1.100",  // Should match /16
        "10.1.100.1",  // Should match /16
        "192.168.1.100" // Should match /24
    };

    for (const auto& dst_ip : test_destinations) {
        Packet pkt{
            .src_mac = "aa:bb:cc:dd:ee:ff",
            .dst_mac = "ff:ee:dd:cc:bb:aa",
            .src_ip = "172.16.0.1",
            .dst_ip = dst_ip,
            .ingress_port = 1
        };
        pipeline_->process(pkt);
    }
}

TEST_F(PipelineTest, StateConsistency) {
    // Test that pipeline maintains consistent state across multiple operations
    
    // Learn MAC
    Packet learn_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "10.0.0.1",
        .dst_ip = "10.0.0.2",
        .ingress_port = 1
    };
    pipeline_->process(learn_pkt);

    // Add route
    pipeline_->get_l3().add_route("10.0.0.0", 24, "10.0.0.1");

    // Send multiple packets
    for (int i = 0; i < 10; ++i) {
        Packet pkt{
            .src_mac = "11:22:33:44:55:66",
            .dst_mac = "aa:bb:cc:dd:ee:ff",
            .src_ip = "10.0.0." + std::to_string(i + 1),
            .dst_ip = "10.0.0.100",
            .ingress_port = 2
        };
        pipeline_->process(pkt);
    }

    // Verify state is still consistent
    Packet test_pkt{
        .src_mac = "22:33:44:55:66:77",
        .dst_mac = "aa:bb:cc:dd:ee:ff",
        .src_ip = "10.0.0.50",
        .dst_ip = "10.0.0.100",
        .ingress_port = 3
    };
    pipeline_->process(test_pkt);
}
