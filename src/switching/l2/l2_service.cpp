#include "l2/l2_service.h"
#include <iostream>
#include <algorithm>

namespace MiniSonic::L2 {

L2Service::L2Service(Sai::SaiInterface& sai)
    : m_sai(sai) {}

void L2Service::learn(const Types::MacAddress& mac, const Types::Port port) {
    m_mac_table[mac] = port;
    m_sai.addFdbEntry(mac, port);
}

Types::Optional<Types::Port> L2Service::lookup(const Types::MacAddress& mac) const {
    if (const auto it = m_mac_table.find(mac); it != m_mac_table.end()) {
        return it->second;
    }
    return std::nullopt;
}

void L2Service::forward(const DataPlane::Packet& pkt, const Types::Port out_port) {
    std::cout << "[L2] Forward " << pkt.dstMac() << " to port " << out_port << "\n";
}

void L2Service::flood(const DataPlane::Packet& pkt) {
    std::cout << "[L2] Flood " << pkt.dstMac() << " to all ports\n";
}

bool L2Service::handle(DataPlane::Packet& pkt) {
    // Learn source MAC
    learn(pkt.srcMac(), pkt.ingressPort());

    // Lookup destination MAC
    const auto dst_port = lookup(pkt.dstMac());
    
    if (dst_port) {
        // Forward to known port
        forward(pkt, *dst_port);
        return true;
    } else {
        // Flood to all ports
        flood(pkt);
        return true;
    }
}

} // namespace MiniSonic::L2
