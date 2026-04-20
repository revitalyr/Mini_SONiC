/**
 * @file test_constants.cpp
 * @brief Unit tests for MiniSonic.Constants module
 */

#include <catch2/catch_all.hpp>

// Import the module
import MiniSonic.Constants;

using namespace MiniSonic::Constants;

TEST_CASE("Constants EthernetProtocolTypes", "[constants][ethernet]") {
    SECTION("ARP protocol type") {
        REQUIRE(ETH_P_ARP == 0x0806);
    }
    
    SECTION("IP protocol type") {
        REQUIRE(ETH_P_IP == 0x0800);
    }
    
    SECTION("802.1Q VLAN protocol type") {
        REQUIRE(ETH_P_8021Q == 0x8100);
    }
    
    SECTION("IPv6 protocol type") {
        REQUIRE(ETH_P_IPV6 == 0x86DD);
    }
}

TEST_CASE("Constants IpProtocolTypes", "[constants][ip]") {
    SECTION("ICMP protocol") {
        REQUIRE(IP_PROTOCOL_ICMP == 1);
    }
    
    SECTION("TCP protocol") {
        REQUIRE(IP_PROTOCOL_TCP == 6);
    }
    
    SECTION("UDP protocol") {
        REQUIRE(IP_PROTOCOL_UDP == 17);
    }
}

TEST_CASE("Constants BufferAndTableSizes", "[constants][sizes]") {
    SECTION("Buffer size") {
        REQUIRE(BUFFER_SIZE == 2048);
    }
    
    SECTION("Max ports") {
        REQUIRE(MAX_PORTS == 8);
    }
    
    SECTION("MAC table size") {
        REQUIRE(MAC_TABLE_SIZE == 1024);
    }
    
    SECTION("ARP table size") {
        REQUIRE(ARP_TABLE_SIZE == 256);
    }
    
    SECTION("VLAN table size") {
        REQUIRE(VLAN_TABLE_SIZE == 64);
    }
    
    SECTION("Ring buffer size") {
        REQUIRE(RING_SIZE == 1024);
    }
}

TEST_CASE("Constants TimeoutValues", "[constants][timeouts]") {
    SECTION("MAC age timeout") {
        REQUIRE(MAC_AGE_TIMEOUT_SEC == 300);
    }
    
    SECTION("ARP timeout") {
        REQUIRE(ARP_TIMEOUT_SEC == 300);
    }
}

TEST_CASE("Constants StringLiterals", "[constants][strings]") {
    SECTION("Default bind address") {
        REQUIRE(std::string(DEFAULT_BIND_ADDRESS) == "0.0.0.0");
    }
    
    SECTION("Default loopback address") {
        REQUIRE(std::string(DEFAULT_LOOPBACK_ADDRESS) == "127.0.0.1");
    }
    
    SECTION("Gossip protocol name") {
        REQUIRE(std::string(PROTOCOL_NAME_GOSSIP) == "Gossip");
    }
}

TEST_CASE("Constants GossipProtocol", "[constants][gossip]") {
    SECTION("Default gossip port") {
        REQUIRE(DEFAULT_GOSSIP_PORT == 7946);
    }
    
    SECTION("Default gossip interval") {
        REQUIRE(DEFAULT_GOSSIP_INTERVAL_MS == 1000);
    }
    
    SECTION("Default gossip fanout") {
        REQUIRE(DEFAULT_GOSSIP_FANOUT == 3);
    }
    
    SECTION("Default suspicion timeout") {
        REQUIRE(DEFAULT_SUSPICION_TIMEOUT_MS == 5000);
    }
    
    SECTION("Default dead timeout") {
        REQUIRE(DEFAULT_DEAD_TIMEOUT_MS == 30000);
    }
    
    SECTION("Default ping interval") {
        REQUIRE(DEFAULT_PING_INTERVAL_MS == 1000);
    }
    
    SECTION("Default ping timeout") {
        REQUIRE(DEFAULT_PING_TIMEOUT_MS == 500);
    }
    
    SECTION("Default ping retries") {
        REQUIRE(DEFAULT_PING_RETRIES == 3);
    }
    
    SECTION("Default max peers") {
        REQUIRE(DEFAULT_MAX_PEERS == 100);
    }
}

TEST_CASE("Constants Pipeline", "[constants][pipeline]") {
    SECTION("Default batch size") {
        REQUIRE(DEFAULT_BATCH_SIZE == 32);
    }
    
    SECTION("Default SPSC queue capacity") {
        REQUIRE(DEFAULT_SPSC_QUEUE_CAPACITY == 1024);
    }
}

TEST_CASE("Constants CacheAlignment", "[constants][cache]") {
    SECTION("Cache line size") {
        REQUIRE(CACHE_LINE_SIZE == 64);
    }
}
