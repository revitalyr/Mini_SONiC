#include "l3/l3_service.h"
#include <iostream>
#include <string>
#include <optional>

namespace MiniSonic::L3 {

L3Service::L3Service(Sai::SaiInterface& sai)
    : m_sai(sai) {}

void L3Service::addRoute(
    const Types::String& prefix,
    Types::PrefixLength prefix_len,
    const Types::NextHop& next_hop
) {
    m_trie.insert(prefix, prefix_len, next_hop);
    m_sai.addRoute(prefix, next_hop);
}

Types::Optional<Types::NextHop> L3Service::lookup(const Types::String& ip) const {
    return m_trie.lookup(ip);
}

void L3Service::forward(const DataPlane::Packet& pkt, const Types::NextHop& next_hop) {
    std::cout << "[L3] Forward " << pkt.dstIp() << " via " << next_hop << "\n";
}

bool L3Service::handle(DataPlane::Packet& pkt) {
    // Lookup destination IP in routing table
    const auto next_hop = lookup(pkt.dstIp());
    
    if (next_hop) {
        // Forward to next hop
        forward(pkt, *next_hop);
        return true;
    } else {
        // No route found
        return false;
    }
}

} // namespace MiniSonic::L3
