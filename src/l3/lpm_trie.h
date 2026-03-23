#pragma once

#include <memory>
#include <optional>
#include <string>

class LpmTrie {
public:
    void insert(const std::string& prefix, int prefix_len,
                const std::string& next_hop);

    std::optional<std::string> lookup(const std::string& ip);

private:
    struct Node {
        std::unique_ptr<Node> left;   // bit 0
        std::unique_ptr<Node> right;  // bit 1
        std::optional<std::string> next_hop;
    };

    std::unique_ptr<Node> root_ = std::make_unique<Node>();
    uint32_t ip_to_int(const std::string& ip);
};
