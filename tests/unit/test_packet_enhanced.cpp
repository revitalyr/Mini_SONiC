/**
 * @file test_packet_enhanced.cpp
 * @brief Unit tests for MiniSonic.DataPlane.PacketEnhanced module
 */

#include <catch2/catch_all.hpp>
#include <thread>
#include <chrono>
#include "common/types.hpp"

import MiniSonic.DataPlane.PacketEnhanced;
import MiniSonic.Core.Utils;

using namespace MiniSonic::DataPlane;
using namespace MiniSonic::Types;

TEST_CASE("EnhancedPacket construction", "[packet][construction]") {
    SECTION("Default constructor") {
        EnhancedPacket pkt;

        REQUIRE(pkt.sequence_number == 0);
        REQUIRE(pkt.protocol() == ProtocolType::UNKNOWN);
        REQUIRE(pkt.ttl() == 64);
        REQUIRE(pkt.ingressPort() == 0);
        REQUIRE(pkt.current_stage == PipelineStage::INGRESS);
        REQUIRE(!pkt.is_dropped);
    }

    SECTION("Parameterized constructor") {
        uint64_t src_mac = 0xAABBCCDDEE01ULL;
        uint64_t dst_mac = 0xAABBCCDDEE02ULL;
        uint32_t src_ip = 0x0A000001;  // 10.0.0.1
        uint32_t dst_ip = 0x0A000002;  // 10.0.0.2
        uint16_t ingress = 1;

        EnhancedPacket pkt(src_mac, dst_mac, src_ip, dst_ip, ingress, ProtocolType::TCP);

        REQUIRE(pkt.srcMac() == src_mac);
        REQUIRE(pkt.dstMac() == dst_mac);
        REQUIRE(pkt.srcIp() == src_ip);
        REQUIRE(pkt.dstIp() == dst_ip);
        REQUIRE(pkt.ingressPort() == ingress);
        REQUIRE(pkt.protocol() == ProtocolType::TCP);
    }
}

TEST_CASE("EnhancedPacket metadata", "[packet][metadata]") {
    SECTION("VLAN tagging") {
        EnhancedPacket pkt;

        REQUIRE(!pkt.vlanId().has_value());

        pkt.setVlanId(100);
        REQUIRE(pkt.vlanId().has_value());
        REQUIRE(pkt.vlanId().value() == 100);

        pkt.setVlanId(0); // Or use a clearVlan() if implemented
        REQUIRE(!pkt.vlanId().has_value());
    }
    
    SECTION("DSCP/TOS handling") {
        EnhancedPacket pkt;
        
        // DSCP is upper 6 bits of TOS
        pkt.setDscp(46);  // Expedited Forwarding
        REQUIRE(pkt.dscp() == 46);
        
        pkt.setDscp(0);   // Best Effort
        REQUIRE(pkt.dscp() == 0);
    }
    
    SECTION("TTL decrement") {
        EnhancedPacket pkt;
        pkt.setTtl(64);
        
        REQUIRE(pkt.ttl() == 64);
        
        pkt.setTtl(pkt.ttl() - 1);
        REQUIRE(pkt.ttl() == 63);
    }
    
    SECTION("L4 ports") {
        EnhancedPacket pkt;
        
        REQUIRE(!pkt.srcPort().has_value());
        REQUIRE(!pkt.dstPort().has_value());
        
        pkt.setSrcPort(12345);
        pkt.setDstPort(80);
        
        REQUIRE(pkt.srcPort().has_value());
        REQUIRE(*pkt.srcPort() == 12345);
        REQUIRE(pkt.dstPort().has_value());
        REQUIRE(*pkt.dstPort() == 80);
    }
}

TEST_CASE("EnhancedPacket pipeline stage tracking", "[packet][pipeline]") {
    SECTION("Stage transitions") {
        EnhancedPacket pkt;
        
        REQUIRE(pkt.current_stage == PipelineStage::INGRESS);
        REQUIRE(pkt.stage_history.size() == 0);
        
        pkt.recordStage(PipelineStage::PARSER);
        REQUIRE(pkt.current_stage == PipelineStage::PARSER);
        REQUIRE(pkt.stage_history.size() == 1);
        
        pkt.recordStage(PipelineStage::L2_LOOKUP);
        REQUIRE(pkt.current_stage == PipelineStage::L2_LOOKUP);
        REQUIRE(pkt.stage_history.size() == 2);
        
        pkt.recordStage(PipelineStage::L3_LOOKUP);
        REQUIRE(pkt.current_stage == PipelineStage::L3_LOOKUP);
        REQUIRE(pkt.stage_history.size() == 3);
    }
    
    SECTION("Egress port assignment") {
        EnhancedPacket pkt;
        
        REQUIRE(!pkt.egressPort().has_value());
        
        pkt.setEgressPort(5);
        REQUIRE(pkt.egressPort().has_value());
        REQUIRE(*pkt.egressPort() == 5);
    }
}

TEST_CASE("EnhancedPacket drop handling", "[packet][drop]") {
    SECTION("Mark as dropped") {
        EnhancedPacket pkt;
        
        REQUIRE(!pkt.is_dropped);
        
        pkt.markDropped("ACL denied");
        
        REQUIRE(pkt.is_dropped);
        REQUIRE(pkt.drop_reason == "ACL denied");
        REQUIRE(pkt.current_stage == PipelineStage::DROPPED);
        REQUIRE(pkt.stage_history.size() == 1);
    }
    
    SECTION("TTL exceeded drop") {
        EnhancedPacket pkt;
        pkt.setTtl(1);
        pkt.setTtl(pkt.ttl() - 1);
        
        REQUIRE(pkt.ttl() == 0);
        
        pkt.markDropped("TTL exceeded");
        REQUIRE(pkt.is_dropped);
    }
}

TEST_CASE("EnhancedPacket latency measurement", "[packet][latency]") {
    SECTION("Latency calculation") {
        EnhancedPacket pkt;
        
        // Give a small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        pkt.markCompleted();
        
        auto latency = pkt.getLatencyUsec();
        REQUIRE(latency.count() >= 10000);  // At least 10ms in microseconds
        REQUIRE(latency.count() < 1000000); // Less than 1 second
    }
}

TEST_CASE("EnhancedPacket sequence number assignment", "[packet][sequence]") {
    SECTION("Sequence numbers are unique and increasing") {
        EnhancedPacket pkt1;
        EnhancedPacket pkt2;
        EnhancedPacket pkt3;
        
        assignSequenceNumber(pkt1);
        assignSequenceNumber(pkt2);
        assignSequenceNumber(pkt3);
        
        REQUIRE(pkt1.sequence_number > 0);
        REQUIRE(pkt2.sequence_number > pkt1.sequence_number);
        REQUIRE(pkt3.sequence_number > pkt2.sequence_number);
    }
    
    SECTION("PacketSequenceCounter singleton") {
        auto& counter1 = PacketSequenceCounter::instance();
        auto& counter2 = PacketSequenceCounter::instance();
        
        REQUIRE(&counter1 == &counter2);  // Same instance
        
        uint64_t seq1 = counter1.next();
        uint64_t seq2 = counter1.next();
        
        REQUIRE(seq2 == seq1 + 1);
    }
}

TEST_CASE("EnhancedPacket string representation", "[packet][debug]") {
    SECTION("toString contains key information") {
        EnhancedPacket pkt(
            0xAABBCCDDEE01ULL,
            0xAABBCCDDEE02ULL,
            0x0A000001,
            0x0A000002,
            1,
            ProtocolType::TCP
        );
        assignSequenceNumber(pkt);
        
        std::string str = pkt.toString();
        
        REQUIRE(str.find("Packet[") != std::string::npos);
        REQUIRE(str.find("seq=") != std::string::npos);
        REQUIRE(str.find("aa:bb:cc:dd:ee:01") != std::string::npos);
        REQUIRE(str.find("10.0.0.1") != std::string::npos);
    }
    
    SECTION("Dropped packet in toString") {
        EnhancedPacket pkt;
        pkt.markDropped("Test drop reason");
        
        std::string str = pkt.toString();
        
        REQUIRE(str.find("[DROPPED:") != std::string::npos);
        REQUIRE(str.find("Test drop reason") != std::string::npos);
    }
}
