#include "l3/lpm_trie.h"
#include <sstream>
#include <array>

namespace MiniSonic::L3 {

std::uint32_t LpmTrie::ipToInt(const Types::String& ip) {
    std::stringstream ss(ip);
    Types::String item;
    std::uint32_t result = 0;

    for (int i = 0; i < 4; ++i) {
        std::getline(ss, item, '.');
        result = (result << 8) | std::stoi(item);
    }

    return result;
}

void LpmTrie::insert(
    const Types::String& prefix, 
    Types::PrefixLength prefix_len,
    const Types::NextHop& next_hop
) {
    const auto ip = ipToInt(prefix);
    auto* node = m_root.get();

    for (int i = 31; i >= static_cast<int>(32 - prefix_len); --i) {
        const bool bit = (ip >> i) & 1;

        if (bit) {
            if (!node->m_right) {
                node->m_right = std::make_unique<Node>();
            }
            node = node->m_right.get();
        } else {
            if (!node->m_left) {
                node->m_left = std::make_unique<Node>();
            }
            node = node->m_left.get();
        }
    }

    node->m_next_hop = next_hop;
}

Types::Optional<Types::NextHop> LpmTrie::lookup(const Types::String& ip) const {
    const auto ip_int = ipToInt(ip);

    const auto* node = m_root.get();
    Types::Optional<Types::NextHop> best_match;

    // Check root first (for default route 0.0.0.0/0)
    if (node->m_next_hop) {
        best_match = node->m_next_hop;
    }

    for (int i = 31; i >= 0; --i) {
        const bool bit = (ip_int >> i) & 1;

        if (bit) {
            if (!node->m_right) break;
            node = node->m_right.get();
        } else {
            if (!node->m_left) break;
            node = node->m_left.get();
        }
        
        if (node->m_next_hop) {
            best_match = node->m_next_hop;
        }
    }

    return best_match;
}

} // namespace MiniSonic::L3
