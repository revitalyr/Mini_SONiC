#include "sai/simulated_sai.h"
#include <iostream>

namespace MiniSonic::Sai {

void SimulatedSai::createPort(const Types::Port port_id) {
    m_ports[port_id] = true;
    std::cout << "[SAI] Create port " << port_id << "\n";
}

void SimulatedSai::removePort(const Types::Port port_id) {
    m_ports.erase(port_id);
    std::cout << "[SAI] Remove port " << port_id << "\n";
}

void SimulatedSai::addFdbEntry(const Types::MacAddress& mac, const Types::Port port) {
    m_fdb[mac] = port;
    std::cout << "[SAI] FDB " << mac << " -> " << port << "\n";
}

void SimulatedSai::removeFdbEntry(const Types::MacAddress& mac) {
    m_fdb.erase(mac);
    std::cout << "[SAI] Remove FDB entry " << mac << "\n";
}

void SimulatedSai::addRoute(
    const Types::String& prefix,
    const Types::NextHop& next_hop
) {
    m_routes[prefix] = next_hop;
    std::cout << "[SAI] Route " << prefix << " -> " << next_hop << "\n";
}

void SimulatedSai::removeRoute(const Types::String& prefix) {
    m_routes.erase(prefix);
    std::cout << "[SAI] Remove route " << prefix << "\n";
}

} // namespace MiniSonic::Sai
