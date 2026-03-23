#include "l3/lpm_trie.h"
#include <sstream>

uint32_t LpmTrie::ip_to_int(const std::string& ip) {
    std::stringstream ss(ip);
    std::string item;
    uint32_t result = 0;

    for (int i = 0; i < 4; ++i) {
        std::getline(ss, item, '.');
        result = (result << 8) | std::stoi(item);
    }

    return result;
}

void LpmTrie::insert(const std::string& prefix, int prefix_len,
                     const std::string& next_hop) {
    uint32_t ip = ip_to_int(prefix);
    Node* node = root_.get();

    for (int i = 31; i >= 32 - prefix_len; --i) {
        bool bit = (ip >> i) & 1;

        if (bit) {
            if (!node->right)
                node->right = std::make_unique<Node>();
            node = node->right.get();
        } else {
            if (!node->left)
                node->left = std::make_unique<Node>();
            node = node->left.get();
        }
    }

    node->next_hop = next_hop;
}

std::optional<std::string> LpmTrie::lookup(const std::string& ip_str) {
    uint32_t ip = ip_to_int(ip_str);

    Node* node = root_.get();
    std::optional<std::string> best_match;

    // Check root first (for default route 0.0.0.0/0)
    if (node->next_hop)
        best_match = node->next_hop;

    for (int i = 31; i >= 0; --i) {
        bool bit = (ip >> i) & 1;

        if (bit) {
            if (!node->right) break;
            node = node->right.get();
        } else {
            if (!node->left) break;
            node = node->left.get();
        }
        
        if (node->next_hop)
            best_match = node->next_hop;
    }

    return best_match;
}
