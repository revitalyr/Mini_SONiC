/**
 * @file test_arp_pending_queue.cpp
 * @brief Unit tests for MiniSonic.L3.ArpPendingQueue module
 */

#include <catch2/catch_all.hpp>

import MiniSonic.Core.Utils;
import MiniSonic.L3.ArpPendingQueue;
import MiniSonic.DataPlane.PacketEnhanced;
import MiniSonic.Boost.Wrappers;

using namespace MiniSonic::L3;
using namespace MiniSonic::DataPlane;
using namespace MiniSonic::BoostWrapper;
using namespace MiniSonic::Types;

// Helper to create a test packet
std::shared_ptr<EnhancedPacket> createTestPacket(const std::string& src_ip, const std::string& dst_ip) {
    // Use macToUint64 and ipToUint32 for conversion
    auto pkt = std::make_shared<EnhancedPacket>(
        macToUint64("aa:bb:cc:dd:ee:01"),
        macToUint64("aa:bb:cc:dd:ee:02"),
        ipToUint32(src_ip),
        ipToUint32(dst_ip),
        1,
        ProtocolType::TCP
    );
    assignSequenceNumber(*pkt);
    return pkt;
}

TEST_CASE("ArpPendingQueue basic enqueue", "[arp][pending_queue]") {
    ArpPendingQueue queue;
    
    SECTION("Enqueue single packet") {
        auto pkt = createTestPacket("10.0.0.1", "10.0.0.2");
        uint32_t pending_ip = 0x0A000002;  // 10.0.0.2
        
        bool success = queue.enqueue(pkt, pending_ip);
        
        REQUIRE(success);
        
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 1);
        REQUIRE(stats.unique_ips == 1);
    }
    
    SECTION("Enqueue multiple packets for same IP") {
        auto pkt1 = createTestPacket("10.0.0.1", "10.0.0.2");
        auto pkt2 = createTestPacket("10.0.0.3", "10.0.0.2");
        uint32_t pending_ip = 0x0A000002;
        
        REQUIRE(queue.enqueue(pkt1, pending_ip));
        REQUIRE(queue.enqueue(pkt2, pending_ip));
        
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 2);
        REQUIRE(stats.unique_ips == 1);  // Same IP
    }
    
    SECTION("Enqueue packets for different IPs") {
        auto pkt1 = createTestPacket("10.0.0.1", "10.0.0.2");
        auto pkt2 = createTestPacket("10.0.0.1", "10.0.0.3");
        
        REQUIRE(queue.enqueue(pkt1, 0x0A000002));
        REQUIRE(queue.enqueue(pkt2, 0x0A000003));
        
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 2);
        REQUIRE(stats.unique_ips == 2);
    }
}

TEST_CASE("ArpPendingQueue ARP resolution", "[arp][resolution]") {
    ArpPendingQueue queue;
    
    SECTION("Forward packets on ARP resolved") {
        auto pkt1 = createTestPacket("10.0.0.1", "10.0.0.2");
        auto pkt2 = createTestPacket("10.0.0.3", "10.0.0.2");
        uint32_t pending_ip = 0x0A000002;
        
        queue.enqueue(pkt1, pending_ip);
        queue.enqueue(pkt2, pending_ip);
        
        size_t forwarded = 0;
        size_t forwarded_count = queue.onArpResolved(pending_ip, 
            [&forwarded](const EnhancedPacket& pkt) {
                forwarded++;
            });
        
        REQUIRE(forwarded_count == 2);
        REQUIRE(forwarded == 2);
        
        // Queue should be empty after resolution
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 0);
        REQUIRE(stats.unique_ips == 0);
    }
    
    SECTION("No forwarding for unknown IP") {
        auto pkt = createTestPacket("10.0.0.1", "10.0.0.2");
        queue.enqueue(pkt, 0x0A000002);
        
        size_t forwarded = queue.onArpResolved(0x0A000003,  // Different IP
            [](const EnhancedPacket&) { });
        
        REQUIRE(forwarded == 0);
        
        // Original packet still pending
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 1);
    }
}

TEST_CASE("ArpPendingQueue get pending IPs", "[arp][pending_ips]") {
    ArpPendingQueue queue;
    
    SECTION("Get list of pending IPs") {
        auto pkt1 = createTestPacket("10.0.0.1", "10.0.0.2");
        auto pkt2 = createTestPacket("10.0.0.1", "10.0.0.3");
        auto pkt3 = createTestPacket("10.0.0.1", "10.0.0.4");
        
        queue.enqueue(pkt1, 0x0A000002);
        queue.enqueue(pkt2, 0x0A000003);
        queue.enqueue(pkt3, 0x0A000004);
        
        auto pending_ips = queue.getPendingIps();
        
        REQUIRE(pending_ips.size() == 3);
        REQUIRE(std::find(pending_ips.begin(), pending_ips.end(), 0x0A000002) != pending_ips.end());
        REQUIRE(std::find(pending_ips.begin(), pending_ips.end(), 0x0A000003) != pending_ips.end());
        REQUIRE(std::find(pending_ips.begin(), pending_ips.end(), 0x0A000004) != pending_ips.end());
    }
}

TEST_CASE("ArpPendingQueue cleanup and timeout", "[arp][cleanup]") {
    ArpPendingQueue queue;
    
    SECTION("Cleanup doesn't drop fresh packets") {
        auto pkt = createTestPacket("10.0.0.1", "10.0.0.2");
        queue.enqueue(pkt, 0x0A000002);
        
        size_t dropped = queue.cleanup([](const EnhancedPacket&) { });
        
        REQUIRE(dropped == 0);
        
        auto stats = queue.getStats();
        REQUIRE(stats.pending_count == 1);
    }
}

TEST_CASE("ArpPendingQueue stats", "[arp][stats]") {
    ArpPendingQueue queue;
    
    SECTION("Empty queue stats") {
        auto stats = queue.getStats();
        
        REQUIRE(stats.pending_count == 0);
        REQUIRE(stats.unique_ips == 0);
    }
    
    SECTION("Stats after enqueue") {
        auto pkt1 = createTestPacket("10.0.0.1", "10.0.0.2");
        auto pkt2 = createTestPacket("10.0.0.3", "10.0.0.2");
        auto pkt3 = createTestPacket("10.0.0.1", "10.0.0.4");
        
        queue.enqueue(pkt1, 0x0A000002);
        queue.enqueue(pkt2, 0x0A000002);
        queue.enqueue(pkt3, 0x0A000004);
        
        auto stats = queue.getStats();
        
        REQUIRE(stats.pending_count == 3);
        REQUIRE(stats.unique_ips == 2);
    }
}

TEST_CASE("ArpPendingSPSCQueue basic operations", "[arp][spsc]") {
    ArpPendingSPSCQueue<1024> queue;
    
    SECTION("Enqueue and dequeue") {
        auto pkt = createTestPacket("10.0.0.1", "10.0.0.2");
        uint32_t pending_ip = 0x0A000002;
        
        bool enqueued = queue.tryEnqueue(pkt, pending_ip);
        REQUIRE(enqueued);
        REQUIRE(queue.size() == 1);
        
        std::shared_ptr<EnhancedPacket> dequeued_pkt;
        uint32_t dequeued_ip = 0;
        bool dequeued = queue.tryDequeue(dequeued_pkt, dequeued_ip);
        
        REQUIRE(dequeued);
        REQUIRE(dequeued_pkt != nullptr);
        REQUIRE(dequeued_ip == pending_ip);
        REQUIRE(queue.size() == 0);
    }
    
    SECTION("Dequeue from empty queue fails") {
        std::shared_ptr<EnhancedPacket> pkt;
        uint32_t ip = 0;
        
        bool dequeued = queue.tryDequeue(pkt, ip);
        
        REQUIRE(!dequeued);
        REQUIRE(pkt == nullptr);
    }
}
