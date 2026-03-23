#include "dataplane/pipeline.h"
#include <iostream>

namespace MiniSonic::DataPlane {

Pipeline::Pipeline(Sai::SaiInterface& sai)
    : m_l2(sai), m_l3(sai) {}

void Pipeline::process(Packet& pkt) {
    // Try L2 switching first
    if (m_l2.handle(pkt)) {
        return;
    }
    
    // If L2 didn't handle, try L3 routing
    if (m_l3.handle(pkt)) {
        return;
    }

    // Neither L2 nor L3 could handle the packet
    std::cout << "[DROP] Packet dropped\n";
}

} // namespace MiniSonic::DataPlane
