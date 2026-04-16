module;

#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <iostream>
#include <sstream>
#include <mutex>
#include <string>
#include <chrono>
#include <algorithm>

module MiniSonic.SAI;

namespace MiniSonic::SAI {
// SimulatedSai Implementation
SimulatedSai::SimulatedSai() {
    std::cout << "[SAI] Initializing simulated SAI\n";
    
    // Create default ports
    for (Types::Port i = 1; i <= 24; ++i) {
        createPort(i);
    }
    
    std::cout << "[SAI] Created " << m_ports.size() << " default ports\n";
}

bool SimulatedSai::createPort(Types::Port port_id) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    if (m_ports.find(port_id) != m_ports.end()) {
        std::cerr << "[SAI] Port " << port_id << " already exists\n";
        return false;
    }
    
    PortInfo port_info;
    port_info.admin_state = true;
    port_info.oper_state = true;
    port_info.created_at = std::chrono::steady_clock::now();
    
    m_ports[port_id] = port_info;
    
    // std::cout << "[SAI] Created port " << port_id << "\n";
    return true;
}

bool SimulatedSai::deletePort(Types::Port port_id) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    auto it = m_ports.find(port_id);
    if (it == m_ports.end()) {
        std::cerr << "[SAI] Port " << port_id << " not found\n";
        return false;
    }
    
    m_ports.erase(it);
    // std::cout << "[SAI] Deleted port " << port_id << "\n";
    return true;
}

bool SimulatedSai::setPortState(Types::Port port_id, bool admin_state) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    auto it = m_ports.find(port_id);
    if (it == m_ports.end()) {
        std::cerr << "[SAI] Port " << port_id << " not found\n";
        return false;
    }
    
    it->second.admin_state = admin_state;
    updateOperState(port_id);
    
    // std::cout << "[SAI] Set port " << port_id << " admin state to " 
    //           << (admin_state ? "up" : "down") << "\n";
    return true;
}

bool SimulatedSai::getPortState(Types::Port port_id) const {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    auto it = m_ports.find(port_id);
    if (it == m_ports.end()) {
        return false;
    }
    
    return it->second.oper_state;
}

std::vector<Types::Port> SimulatedSai::getPortList() const {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    std::vector<Types::Port> ports;
    ports.reserve(m_ports.size());
    
    for (const auto& [port_id, _] : m_ports) {
        ports.push_back(port_id);
    }
    
    return ports;
}

std::string SimulatedSai::getStats() const {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    
    std::ostringstream oss;
    oss << "SAI Statistics:\n"
         << "  Total Ports: " << m_ports.size() << "\n"
         << "  Active Ports: " << static_cast<size_t>(std::count_if(
                m_ports.begin(), m_ports.end(),
                [](const auto& pair) { return pair.second.oper_state; }
            )) << "\n";
    
    Types::Count total_in = 0;
    Types::Count total_out = 0;
    
    for (const auto& [port_id, info] : m_ports) {
        total_in += info.packets_in;
        total_out += info.packets_out;
    }
    
    oss << "  Total Packets In: " << static_cast<uint64_t>(total_in) << "\n"
         << "  Total Packets Out: " << static_cast<uint64_t>(total_out) << "\n";
    
    return oss.str();
}

void SimulatedSai::updateOperState(Types::Port port_id) {
    // In simulation, oper state follows admin state
    // In real hardware, this would involve link detection, etc.
    auto it = m_ports.find(port_id);
    if (it != m_ports.end()) {
        it->second.oper_state = it->second.admin_state;
    }
}

} // export namespace MiniSonic::SAI
