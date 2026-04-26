#include <catch2/catch_all.hpp>
#include <optional>

import MiniSonic.Core.Types;
import MiniSonic.L2L3;
using MiniSonic::L3::LpmTrie;
using namespace MiniSonic::Types;

TEST_CASE("LpmTrie EmptyTrieReturnsNullopt", "[lpm][trie]") {
    LpmTrie trie;
    auto result = trie.lookup(ipToUint32("192.168.1.1"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("LpmTrie InsertAndLookupExactMatch", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    auto result = trie.lookup(ipToUint32("10.0.0.100"));
    REQUIRE(result.has_value());
    REQUIRE(result.value() == ipToUint32("10.0.0.1"));
}

TEST_CASE("LpmTrie LongestPrefixMatch", "[lpm][trie]") {
    LpmTrie trie;
    // Add multiple overlapping routes
    trie.insert(ipToUint32("10.0.0.0"), 8, ipToUint32("10.0.0.1"));      // /8
    trie.insert(ipToUint32("10.1.0.0"), 16, ipToUint32("10.1.0.1"));     // /16
    trie.insert(ipToUint32("10.1.1.0"), 24, ipToUint32("10.1.1.1"));     // /24

    // Test longest prefix match
    auto result = trie.lookup(ipToUint32("10.1.1.42"));
    REQUIRE(result.has_value());
    REQUIRE(result.value() == ipToUint32("10.1.1.1")); // Should match /24
}

TEST_CASE("LpmTrie LongestPrefixMatchFallback", "[lpm][trie]") {
    LpmTrie trie;
    // Add overlapping routes
    trie.insert(ipToUint32("10.0.0.0"), 8, ipToUint32("10.0.0.1"));      // /8
    trie.insert(ipToUint32("10.1.0.0"), 16, ipToUint32("10.1.0.1"));     // /16

    // Test fallback to shorter prefix
    auto result = trie.lookup(ipToUint32("10.2.1.42"));
    REQUIRE(result.has_value());
    REQUIRE(result.value() == ipToUint32("10.0.0.1")); // Should match /8
}

TEST_CASE("LpmTrie DifferentIpRanges", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert(ipToUint32("192.168.0.0"), 16, ipToUint32("192.168.0.1"));
    trie.insert(ipToUint32("10.0.0.0"), 8, ipToUint32("10.0.0.1"));

    auto result1 = trie.lookup(ipToUint32("192.168.1.100"));
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == ipToUint32("192.168.0.1"));

    auto result2 = trie.lookup(ipToUint32("10.1.1.1"));
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == ipToUint32("10.0.0.1"));
}

TEST_CASE("LpmTrie EdgeCaseIps", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert(ipToUint32("0.0.0.0"), 0, ipToUint32("192.168.1.1"));
    trie.insert(ipToUint32("255.255.255.255"), 32, ipToUint32("1.1.1.1"));

    auto result1 = trie.lookup(ipToUint32("1.2.3.4"));
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == ipToUint32("192.168.1.1"));

    auto result2 = trie.lookup(ipToUint32("255.255.255.255"));
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == ipToUint32("1.1.1.1"));
}

TEST_CASE("LpmTrie NonMatchingIp", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));

    auto result = trie.lookup(ipToUint32("10.1.0.1"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("LpmTrie MultipleRoutesSamePrefix", "[lpm][trie]") {
    LpmTrie trie;
    // Insert route with same prefix but different next-hop
    trie.insert(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.1"));
    trie.insert(ipToUint32("10.0.0.0"), 24, ipToUint32("10.0.0.2")); // Should overwrite

    auto result = trie.lookup(ipToUint32("10.0.0.100"));
    REQUIRE(result.has_value());
    REQUIRE(result.value() == ipToUint32("10.0.0.2"));
}

TEST_CASE("LpmTrie ComplexPrefixHierarchy", "[lpm][trie]") {
    LpmTrie trie;
    // Create complex hierarchy
    trie.insert(ipToUint32("10.0.0.0"), 8, ipToUint32("1.1.1.1"));
    trie.insert(ipToUint32("10.0.0.0"), 16, ipToUint32("2.2.2.2"));
    trie.insert(ipToUint32("10.0.0.0"), 24, ipToUint32("3.3.3.3"));
    trie.insert(ipToUint32("10.0.1.0"), 24, ipToUint32("4.4.4.4"));

    // Test various IPs
    REQUIRE(trie.lookup(ipToUint32("10.0.0.1")).value() == ipToUint32("3.3.3.3")); // matches /24
    REQUIRE(trie.lookup(ipToUint32("10.0.1.1")).value() == ipToUint32("4.4.4.4")); // matches /24
    REQUIRE(trie.lookup(ipToUint32("10.0.2.1")).value() == ipToUint32("2.2.2.2")); // matches /16
    REQUIRE(trie.lookup(ipToUint32("10.1.0.1")).value() == ipToUint32("1.1.1.1")); // matches /8
}
