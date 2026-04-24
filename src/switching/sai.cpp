module;

#include <memory> // For std::unique_ptr, std::shared_ptr
#include <vector> // For std::vector
#include <unordered_map> // For std::unordered_map
#include <iostream> // For std::cout, std::cerr
#include <sstream> // For std::ostringstream
#include <mutex> // For std::mutex, std::lock_guard
#include <string> // For std::string
#include <chrono> // For std::chrono
#include <optional> // For std::optional

module MiniSonic.SAI;

// Import Utils module for Types namespace
import MiniSonic.Core.Utils;
// Import Events module
import MiniSonic.Core.Events;

namespace MiniSonic::SAI {
// SimulatedSai Implementation
SimulatedSai::SimulatedSai(const std::string& switch_id)
    : m_switch_id(switch_id),
      m_event_bus(Events::getGlobalEventBus()) {
    std::cout << "[SAI] Initializing simulated SAI for " << m_switch_id << "\n";

    // Create default ports
    for (Types::PortId i = 1; i <= 24; ++i) {
        createPort(i);
    }

    std::cout << "[SAI] Created " << m_ports.size() << " default ports\n";
}

bool SimulatedSai::createPort(Types::PortId port_id) {
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

bool SimulatedSai::deletePort(Types::PortId port_id) {
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

bool SimulatedSai::setPortState(Types::PortId port_id, bool admin_state) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);

    auto it = m_ports.find(port_id);
    if (it == m_ports.end()) {
        std::cerr << "[SAI] Port " << port_id << " not found\n";
        return false;
    }

    it->second.admin_state = admin_state;
    updateOperState(port_id);

    // Emit PortStateChanged event
    auto port_event = std::make_shared<Events::PortStateChanged>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        "Eth" + std::to_string(port_id),
        admin_state ? "UP" : "DOWN"
    );
    m_event_bus.publish(port_event);

    // std::cout << "[SAI] Set port " << port_id << " admin state to " << (admin_state ? "UP" : "DOWN") << "\n";
    return true;
}

std::optional<bool> SimulatedSai::getPortState(Types::PortId port_id) const {
    std::lock_guard<std::mutex> lock(m_ports_mutex);

    auto it = m_ports.find(port_id);
    if (it == m_ports.end()) {
        return std::nullopt;
    }

    return it->second.oper_state;
}

std::vector<Types::PortId> SimulatedSai::getPortList() const {
    std::lock_guard<std::mutex> lock(m_ports_mutex);

    std::vector<Types::PortId> ports;
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
         << "  Total Ports: " << m_ports.size() << "\n";

    size_t active_ports = 0;
    for (const auto& pair : m_ports) {
        if (pair.second.oper_state) {
            active_ports++;
        }
    }
    oss << "  Active Ports: " << active_ports << "\n";

    Types::PacketCount total_in = 0;
    Types::PacketCount total_out = 0;

    for (const auto& pair : m_ports) {
        total_in += pair.second.packets_in;
        total_out += pair.second.packets_out;
    }

    oss << "  Total Packets In: " << static_cast<uint64_t>(total_in) << "\n"
         << "  Total Packets Out: " << static_cast<uint64_t>(total_out) << "\n";

    return oss.str();
}

void SimulatedSai::updateOperState(Types::PortId port_id) {
    // In simulation, oper state follows admin state
    // In real hardware, this would involve link detection, etc.
    auto it = m_ports.find(port_id);
    if (it != m_ports.end()) {
        it->second.oper_state = it->second.admin_state;
    }
}

bool SimulatedSai::addFdbEntry(Types::MacAddress mac, Types::PortId port) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    m_fdb[mac] = port;
    return true;
}

bool SimulatedSai::removeFdbEntry(Types::MacAddress mac) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    m_fdb.erase(mac);
    return true;
}

bool SimulatedSai::addRoute(Types::IpAddress prefix, [[maybe_unused]] Types::PrefixLength len, Types::IpAddress next_hop) {
    std::lock_guard<std::mutex> lock(m_ports_mutex);
    m_routes[prefix] = next_hop;
    return true;
}

} // export namespace MiniSonic::SAI
