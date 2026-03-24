#include <gtest/gtest.h>
#include "l3/lpm_trie.h"
#include <optional>

class LpmTrieTest : public ::testing::Test {
protected:
    void SetUp() override {
        trie_ = std::make_unique<LpmTrie>();
    }

    std::unique_ptr<LpmTrie> trie_;
};

TEST_F(LpmTrieTest, EmptyTrieReturnsNullopt) {
    auto result = trie_->lookup("192.168.1.1");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LpmTrieTest, InsertAndLookupExactMatch) {
    trie_->insert("10.0.0.0", 24, "10.0.0.1");
    
    auto result = trie_->lookup("10.0.0.100");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "10.0.0.1");
}

TEST_F(LpmTrieTest, LongestPrefixMatch) {
    // Add multiple overlapping routes
    trie_->insert("10.0.0.0", 8, "10.0.0.1");      // /8
    trie_->insert("10.1.0.0", 16, "10.1.0.1");     // /16
    trie_->insert("10.1.1.0", 24, "10.1.1.1");     // /24
    
    // Test longest prefix match
    auto result = trie_->lookup("10.1.1.42");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "10.1.1.1"); // Should match /24
}

TEST_F(LpmTrieTest, LongestPrefixMatchFallback) {
    // Add overlapping routes
    trie_->insert("10.0.0.0", 8, "10.0.0.1");      // /8
    trie_->insert("10.1.0.0", 16, "10.1.0.1");     // /16
    
    // Test fallback to shorter prefix
    auto result = trie_->lookup("10.2.1.42");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "10.0.0.1"); // Should match /8
}

TEST_F(LpmTrieTest, DifferentIpRanges) {
    trie_->insert("192.168.0.0", 16, "192.168.0.1");
    trie_->insert("10.0.0.0", 8, "10.0.0.1");
    
    auto result1 = trie_->lookup("192.168.1.100");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), "192.168.0.1");
    
    auto result2 = trie_->lookup("10.1.1.1");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "10.0.0.1");
}

TEST_F(LpmTrieTest, EdgeCaseIps) {
    trie_->insert("0.0.0.0", 0, "default_route");
    trie_->insert("255.255.255.255", 32, "host_route");
    
    auto result1 = trie_->lookup("1.2.3.4");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), "default_route");
    
    auto result2 = trie_->lookup("255.255.255.255");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), "host_route");
}

TEST_F(LpmTrieTest, NonMatchingIp) {
    trie_->insert("10.0.0.0", 24, "10.0.0.1");
    
    auto result = trie_->lookup("10.1.0.1");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LpmTrieTest, MultipleRoutesSamePrefix) {
    // Insert route with same prefix but different next-hop
    trie_->insert("10.0.0.0", 24, "10.0.0.1");
    trie_->insert("10.0.0.0", 24, "10.0.0.2"); // Should overwrite
    
    auto result = trie_->lookup("10.0.0.100");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "10.0.0.2");
}

TEST_F(LpmTrieTest, ComplexPrefixHierarchy) {
    // Create complex hierarchy
    trie_->insert("10.0.0.0", 8, "gateway1");
    trie_->insert("10.0.0.0", 16, "gateway2");
    trie_->insert("10.0.0.0", 24, "gateway3");
    trie_->insert("10.0.1.0", 24, "gateway4");
    
    // Test various IPs
    EXPECT_EQ(trie_->lookup("10.0.0.1").value(), "gateway3"); // matches /24
    EXPECT_EQ(trie_->lookup("10.0.1.1").value(), "gateway4"); // matches /24
    EXPECT_EQ(trie_->lookup("10.0.2.1").value(), "gateway2"); // matches /16
    EXPECT_EQ(trie_->lookup("10.1.0.1").value(), "gateway1"); // matches /8
}
