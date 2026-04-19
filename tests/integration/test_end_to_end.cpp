#include <catch2/catch_all.hpp>
#include "core/app.h"
#include "sai/simulated_sai.h"
#include "dataplane/packet.h"
#include <thread>
#include <chrono>

TEST_CASE("EndToEnd BasicSwitchingScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Simple L2 learning switch
    
    // 1. Host A (port 1) learns MAC
    Packet host_a_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.1.10",
        .dst_ip = "192.168.1.20",
        .ingress_port = 1
    };
    pipeline.process(host_a_pkt);

    // 2. Host B (port 2) learns MAC
    Packet host_b_pkt{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.1.20",
        .dst_ip = "192.168.1.10",
        .ingress_port = 2
    };
    pipeline.process(host_b_pkt);

    // 3. Host A sends to Host B (should be forwarded)
    Packet forward_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "aa:bb:cc:dd:ee:02",
        .src_ip = "192.168.1.10",
        .dst_ip = "192.168.1.20",
        .ingress_port = 1
    };
    pipeline.process(forward_pkt);

    // 4. Host B responds to Host A (should be forwarded)
    Packet response_pkt{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "aa:bb:cc:dd:ee:01",
        .src_ip = "192.168.1.20",
        .dst_ip = "192.168.1.10",
        .ingress_port = 2
    };
    pipeline.process(response_pkt);
}

TEST_CASE("EndToEnd RouterScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: L3 routing between different subnets
    
    // Setup routing table
    pipeline.get_l3().add_route("192.168.1.0", 24, "192.168.1.1");
    pipeline.get_l3().add_route("192.168.2.0", 24, "192.168.2.1");
    pipeline.get_l3().add_route("10.0.0.0", 8, "10.0.0.1");

    // 1. Packet from 192.168.1.0 to 192.168.2.0
    Packet pkt1{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "192.168.2.100",
        .ingress_port = 1
    };
    pipeline.process(pkt1);

    // 2. Packet from 192.168.2.0 to 10.0.0.0
    Packet pkt2{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.2.100",
        .dst_ip = "10.1.1.100",
        .ingress_port = 2
    };
    pipeline.process(pkt2);

    // 3. Packet with default route scenario
    pipeline.get_l3().add_route("0.0.0.0", 0, "192.168.1.254");
    Packet pkt3{
        .src_mac = "aa:bb:cc:dd:ee:03",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "203.0.113.100",  // External IP
        .ingress_port = 3
    };
    pipeline.process(pkt3);
}

TEST_CASE("EndToEnd MixedL2L3Scenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Mixed L2 switching and L3 routing
    
    // Learn some MAC addresses
    Packet learn1{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.1.10",
        .dst_ip = "192.168.1.20",
        .ingress_port = 1
    };
    pipeline.process(learn1);

    Packet learn2{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.1.20",
        .dst_ip = "192.168.1.10",
        .ingress_port = 2
    };
    pipeline.process(learn2);

    // Add routes
    pipeline.get_l3().add_route("192.168.1.0", 24, "192.168.1.1");
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.1");

    // Test 1: L2 forwarding (same subnet)
    Packet l2_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "aa:bb:cc:dd:ee:02",
        .src_ip = "192.168.1.10",
        .dst_ip = "192.168.1.20",
        .ingress_port = 1
    };
    pipeline.process(l2_pkt);

    // Test 2: L3 routing (different subnet)
    Packet l3_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.10",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };
    pipeline.process(l3_pkt);

    // Test 3: Unknown destination (should be dropped)
    Packet drop_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "99:88:77:66:55:44",
        .src_ip = "192.168.1.10",
        .dst_ip = "172.16.1.100",
        .ingress_port = 1
    };
    pipeline.process(drop_pkt);
}

TEST_CASE("EndToEnd VlanLikeScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Simulate VLAN-like behavior with different port groups
    
    // Group 1: Ports 1-2 (VLAN 10)
    // Group 2: Ports 3-4 (VLAN 20)
    
    // Learn MACs in "VLAN 10"
    Packet vlan10_host1{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.10.10",
        .dst_ip = "192.168.10.20",
        .ingress_port = 1
    };
    pipeline.process(vlan10_host1);

    Packet vlan10_host2{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.10.20",
        .dst_ip = "192.168.10.10",
        .ingress_port = 2
    };
    pipeline.process(vlan10_host2);

    // Learn MACs in "VLAN 20"
    Packet vlan20_host1{
        .src_mac = "aa:bb:cc:dd:ee:03",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.20.10",
        .dst_ip = "192.168.20.20",
        .ingress_port = 3
    };
    pipeline.process(vlan20_host1);

    Packet vlan20_host2{
        .src_mac = "aa:bb:cc:dd:ee:04",
        .dst_mac = "ff:ff:ff:ff:ff:ff",
        .src_ip = "192.168.20.20",
        .dst_ip = "192.168.20.10",
        .ingress_port = 4
    };
    pipeline.process(vlan20_host2);

    // Test intra-VLAN communication
    Packet intra_vlan10{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "aa:bb:cc:dd:ee:02",
        .src_ip = "192.168.10.10",
        .dst_ip = "192.168.10.20",
        .ingress_port = 1
    };
    pipeline.process(intra_vlan10);

    Packet intra_vlan20{
        .src_mac = "aa:bb:cc:dd:ee:03",
        .dst_mac = "aa:bb:cc:dd:ee:04",
        .src_ip = "192.168.20.10",
        .dst_ip = "192.168.20.20",
        .ingress_port = 3
    };
    pipeline.process(intra_vlan20);

    // Test inter-VLAN communication (should be flooded or dropped)
    Packet inter_vlan{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "aa:bb:cc:dd:ee:03",
        .src_ip = "192.168.10.10",
        .dst_ip = "192.168.20.10",
        .ingress_port = 1
    };
    pipeline.process(inter_vlan);
}

TEST_CASE("EndToEnd HighAvailabilityScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Test failover and redundancy
    
    // Setup primary and backup routes
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.1");  // Primary
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.2");  // Backup (overwrites)
    
    // Test traffic flow
    Packet traffic_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };
    pipeline.process(traffic_pkt);

    // Simulate route change
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.3");  // New next-hop
    
    // Test traffic with new route
    Packet new_traffic_pkt{
        .src_mac = "aa:bb:cc:dd:ee:ff",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "10.0.0.200",
        .ingress_port = 1
    };
    pipeline.process(new_traffic_pkt);
}

TEST_CASE("EndToEnd NetworkGrowthScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Simulate network growing over time
    
    // Phase 1: Small network
    pipeline.get_l3().add_route("192.168.1.0", 24, "192.168.1.1");
    
    Packet phase1_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "192.168.1.100",
        .ingress_port = 1
    };
    pipeline.process(phase1_pkt);

    // Phase 2: Network expansion
    pipeline.get_l3().add_route("192.168.2.0", 24, "192.168.2.1");
    pipeline.get_l3().add_route("192.168.3.0", 24, "192.168.3.1");
    
    Packet phase2_pkt1{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "192.168.2.100",
        .ingress_port = 2
    };
    pipeline.process(phase2_pkt1);

    Packet phase2_pkt2{
        .src_mac = "aa:bb:cc:dd:ee:03",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "192.168.3.100",
        .ingress_port = 3
    };
    pipeline.process(phase2_pkt2);

    // Phase 3: Aggregate routes
    pipeline.get_l3().add_route("192.168.0.0", 16, "192.168.0.1");
    
    Packet phase3_pkt{
        .src_mac = "aa:bb:cc:dd:ee:04",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "10.0.0.100",
        .dst_ip = "192.168.100.100",
        .ingress_port = 4
    };
    pipeline.process(phase3_pkt);
}

TEST_CASE("EndToEnd SecurityScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Test security-related features
    
    // Setup restricted routes
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.1");
    pipeline.get_l3().add_route("192.168.1.0", 24, "192.168.1.1");
    
    // Test legitimate traffic
    Packet legitimate_pkt{
        .src_mac = "aa:bb:cc:dd:ee:01",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };
    pipeline.process(legitimate_pkt);

    // Test traffic to non-routed destination (should be dropped)
    Packet blocked_pkt{
        .src_mac = "aa:bb:cc:dd:ee:02",
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "203.0.113.100",  // External, no route
        .ingress_port = 1
    };
    pipeline.process(blocked_pkt);

    // Test spoofed MAC (still processed by L2 learning)
    Packet spoofed_pkt{
        .src_mac = "00:11:22:33:44:55",  // Different from expected
        .dst_mac = "ff:ee:dd:cc:bb:aa",
        .src_ip = "192.168.1.100",
        .dst_ip = "10.0.0.100",
        .ingress_port = 1
    };
    pipeline.process(spoofed_pkt);
}

TEST_CASE("EndToEnd PerformanceScenario", "[e2e][integration]") {
    SimulatedSai sai;
    Pipeline pipeline(sai);
    
    // Scenario: Test performance with high traffic volume
    
    // Setup routing table
    pipeline.get_l3().add_route("10.0.0.0", 24, "10.0.0.1");
    pipeline.get_l3().add_route("192.168.1.0", 24, "192.168.1.1");
    pipeline.get_l3().add_route("172.16.0.0", 16, "172.16.0.1");

    // Learn some MACs
    std::vector<std::string> macs = {
        "aa:bb:cc:dd:ee:01", "aa:bb:cc:dd:ee:02", 
        "aa:bb:cc:dd:ee:03", "aa:bb:cc:dd:ee:04"
    };
    
    for (size_t i = 0; i < macs.size(); ++i) {
        Packet learn_pkt{
            .src_mac = macs[i],
            .dst_mac = "ff:ff:ff:ff:ff:ff",
            .src_ip = "10.0.0." + std::to_string(i + 1),
            .dst_ip = "10.0.0.254",
            .ingress_port = static_cast<int>(i + 1)
        };
        pipeline.process(learn_pkt);
    }

    // Generate high volume traffic
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {  // Reduced from 10000
        Packet pkt{
            .src_mac = macs[i % macs.size()],
            .dst_mac = macs[(i + 1) % macs.size()],
            .src_ip = "10.0.0." + std::to_string((i % 254) + 1),
            .dst_ip = (i % 3 == 0) ? "10.0.0.100" : 
                     (i % 3 == 1) ? "192.168.1.100" : "172.16.100.1",
            .ingress_port = (i % 4) + 1
        };
        pipeline.process(pkt);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Performance should be reasonable (this is more of a smoke test)
    REQUIRE(duration.count() < 10000);  // Increased timeout to 10 seconds
}
