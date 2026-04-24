#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>

namespace MiniSonic::L3 {

class LpmTrie {
public:
    LpmTrie() = default;
    ~LpmTrie() = default;

    // Rule of five
    LpmTrie(const LpmTrie& other) = delete;
    LpmTrie& operator=(const LpmTrie& other) = delete;
    LpmTrie(LpmTrie&& other) noexcept = default;
    LpmTrie& operator=(LpmTrie&& other) noexcept = default;

    void insert(const std::string& network, int prefix_len, const std::string& next_hop);
    void remove(const std::string& network, int prefix_len);
    [[nodiscard]] std::optional<std::string> lookup(const std::string& ip) const;
    [[nodiscard]] size_t size() const;
    void clear();
    [[nodiscard]] std::string getStats() const;

private:
    struct TrieNode {
        std::unique_ptr<TrieNode> children[2];
        std::string next_hop;
        bool is_route = false;
    };

    std::unique_ptr<TrieNode> m_root;

    [[nodiscard]] static std::vector<bool> ipToBinary(const std::string& ip);
    [[nodiscard]] static std::string binaryToIp(std::span<const bool> binary, int prefix_len);
};

} // namespace MiniSonic::L3
