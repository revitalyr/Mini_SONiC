#pragma once

#include "dataplane/packet.h"
#include "sai/sai_interface.h"
#include "l3/lpm_trie.h"

class L3Service {
public:
    explicit L3Service(SaiInterface& sai);

    bool handle(Packet& pkt);
    
    void add_route(const std::string& prefix,
                   int prefix_len,
                   const std::string& next_hop);

private:
    std::optional<std::string> lookup(const std::string& ip);
    void forward(const Packet& pkt, const std::string& next_hop);

    SaiInterface& sai_;
    LpmTrie trie_;
};
