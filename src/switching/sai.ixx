module;

// Use global module fragment for standard library includes to improve MSVC compatibility
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <ranges>
#include <span>
#include <optional>

export module MiniSonic.SAI;

// Import Utils module for Types namespace
import MiniSonic.Core.Utils; // Corrected module name

// Import Events module for visualization events
import MiniSonic.Core.Events; // Corrected module name

export namespace MiniSonic::SAI {

// Using declarations for standard library types
using std::vector;
using std::string;
using std::unordered_map;
using std::mutex; // Already present, but good to double check
using std::chrono::steady_clock;
namespace chrono = std::chrono;

/** 
 * @brief Abstract SAI (Switch Abstraction Interface)
 *
 * Provides hardware abstraction for switch operations using modern C++23.
 */
export class SaiInterface {
public:
    virtual ~SaiInterface() = default;

    // Virtual methods for hardware operations (using Types:: explicitly)
    virtual bool createPort(Types::PortId port_id) = 0;
    virtual bool deletePort(Types::PortId port_id) = 0;
    virtual bool setPortState(Types::PortId port_id, bool admin_state) = 0;
    virtual std::optional<bool> getPortState(Types::PortId port_id) const = 0;
    virtual Types::vector<Types::PortId> getPortList() const = 0;
    virtual string getStats() const = 0;
};

/**
 * @brief Simulated SAI implementation for testing
 *
 * Provides in-memory simulation of hardware operations using modern C++23.
 */
export class SimulatedSai : public SaiInterface {
public:
    SimulatedSai(const std::string& switch_id = "SWITCH0");
    ~SimulatedSai() override = default;

    // SaiInterface implementation
    bool createPort(Types::PortId port_id) override;
    bool deletePort(Types::PortId port_id) override;
    bool setPortState(Types::PortId port_id, bool admin_state) override;
    std::optional<bool> getPortState(Types::PortId port_id) const override;
    Types::vector<Types::PortId> getPortList() const override;
    string getStats() const override;

    /**
     * @brief Set switch ID for event emission
     */
    void setSwitchId(const std::string& switch_id) { m_switch_id = switch_id; }

private:
    struct PortInfo {
        bool admin_state = false;
        bool oper_state = false;
        chrono::steady_clock::time_point created_at;
        Types::PacketCount packets_in = 0;
        Types::PacketCount packets_out = 0;
    };

    std::string m_switch_id;
    Events::EventBus& m_event_bus;
    unordered_map<Types::PortId, PortInfo> m_ports;  // semantic alias
    mutable mutex m_ports_mutex;

    void updateOperState(Types::PortId port_id);
};

} // export namespace MiniSonic::SAI
