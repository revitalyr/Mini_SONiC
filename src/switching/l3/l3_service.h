#pragma once

#include "dataplane/packet.h"
#include "sai/sai_interface.h"
#include "l3/lpm_trie.h"
#include "common/types.hpp"

namespace MiniSonic::L3 {

class L3Service {
public:
    explicit L3Service(Sai::SaiInterface& sai);
    ~L3Service() = default;

    // Rule of five
    L3Service(const L3Service& other) = delete;
    L3Service& operator=(const L3Service& other) = delete;
    L3Service(L3Service&& other) noexcept = delete;
    L3Service& operator=(L3Service&& other) noexcept = delete;

    bool handle(DataPlane::Packet& pkt);
    
    void addRoute(
        const Types::String& prefix,
        Types::PrefixLength prefix_len,
        const Types::NextHop& next_hop
    );

private:
    [[nodiscard]] Types::Optional<Types::NextHop> lookup(const Types::String& ip) const;
    void forward(const DataPlane::Packet& pkt, const Types::NextHop& next_hop);

    Sai::SaiInterface& m_sai;
    LpmTrie m_trie;
};

} // namespace MiniSonic::L3
