#include <catch2/catch_all.hpp>
#include "l3/lpm_trie.h"
#include <optional>

TEST_CASE("LpmTrie EmptyTrieReturnsNullopt", "[lpm][trie]") {
    LpmTrie trie;
    auto result = trie.lookup("192.168.1.1");
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("LpmTrie InsertAndLookupExactMatch", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert("10.0.0.0", 24, "10.0.0.1");

    auto result = trie.lookup("10.0.0.100");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "10.0.0.1");
}

TEST_CASE("LpmTrie LongestPrefixMatch", "[lpm][trie]") {
    LpmTrie trie;
    // Add multiple overlapping routes
    trie.insert("10.0.0.0", 8, "10.0.0.1");      // /8
    trie.insert("10.1.0.0", 16, "10.1.0.1");     // /16
    trie.insert("10.1.1.0", 24, "10.1.1.1");     // /24

    // Test longest prefix match
    auto result = trie.lookup("10.1.1.42");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "10.1.1.1"); // Should match /24
}

TEST_CASE("LpmTrie LongestPrefixMatchFallback", "[lpm][trie]") {
    LpmTrie trie;
    // Add overlapping routes
    trie.insert("10.0.0.0", 8, "10.0.0.1");      // /8
    trie.insert("10.1.0.0", 16, "10.1.0.1");     // /16

    // Test fallback to shorter prefix
    auto result = trie.lookup("10.2.1.42");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "10.0.0.1"); // Should match /8
}

TEST_CASE("LpmTrie DifferentIpRanges", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert("192.168.0.0", 16, "192.168.0.1");
    trie.insert("10.0.0.0", 8, "10.0.0.1");

    auto result1 = trie.lookup("192.168.1.100");
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == "192.168.0.1");

    auto result2 = trie.lookup("10.1.1.1");
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == "10.0.0.1");
}

TEST_CASE("LpmTrie EdgeCaseIps", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert("0.0.0.0", 0, "default_route");
    trie.insert("255.255.255.255", 32, "host_route");

    auto result1 = trie.lookup("1.2.3.4");
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == "default_route");

    auto result2 = trie.lookup("255.255.255.255");
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == "host_route");
}

TEST_CASE("LpmTrie NonMatchingIp", "[lpm][trie]") {
    LpmTrie trie;
    trie.insert("10.0.0.0", 24, "10.0.0.1");

    auto result = trie.lookup("10.1.0.1");
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("LpmTrie MultipleRoutesSamePrefix", "[lpm][trie]") {
    LpmTrie trie;
    // Insert route with same prefix but different next-hop
    trie.insert("10.0.0.0", 24, "10.0.0.1");
    trie.insert("10.0.0.0", 24, "10.0.0.2"); // Should overwrite

    auto result = trie.lookup("10.0.0.100");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "10.0.0.2");
}

TEST_CASE("LpmTrie ComplexPrefixHierarchy", "[lpm][trie]") {
    LpmTrie trie;
    // Create complex hierarchy
    trie.insert("10.0.0.0", 8, "gateway1");
    trie.insert("10.0.0.0", 16, "gateway2");
    trie.insert("10.0.0.0", 24, "gateway3");
    trie.insert("10.0.1.0", 24, "gateway4");

    // Test various IPs
    REQUIRE(trie.lookup("10.0.0.1").value() == "gateway3"); // matches /24
    REQUIRE(trie.lookup("10.0.1.1").value() == "gateway4"); // matches /24
    REQUIRE(trie.lookup("10.0.2.1").value() == "gateway2"); // matches /16
    REQUIRE(trie.lookup("10.1.0.1").value() == "gateway1"); // matches /8
}
