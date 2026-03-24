#pragma once

#include "common/types.hpp"
#include <memory>

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

    void insert(
        const Types::String& prefix, 
        Types::PrefixLength prefix_len,
        const Types::NextHop& next_hop
    );

    [[nodiscard]] Types::Optional<Types::NextHop> lookup(const Types::String& ip) const;

private:
    struct Node {
        Types::UniquePtr<Node> m_left;   // bit 0
        Types::UniquePtr<Node> m_right;  // bit 1
        Types::Optional<Types::NextHop> m_next_hop;

        Node() = default;
        ~Node() = default;
    };

    Types::UniquePtr<Node> m_root = std::make_unique<Node>();
    
    [[nodiscard]] static std::uint32_t ipToInt(const Types::String& ip);
};

} // namespace MiniSonic::L3
