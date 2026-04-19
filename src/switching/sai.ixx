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
import MiniSonic.Utils;

export namespace MiniSonic::SAI {

// Using declarations for standard library types
using std::vector;
using std::string;
using std::unordered_map;
using std::mutex;
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

    // Virtual methods for hardware operations
    virtual bool createPort(Types::Port port_id) = 0;
    virtual bool deletePort(Types::Port port_id) = 0;
    virtual bool setPortState(Types::Port port_id, bool admin_state) = 0;
    virtual std::optional<bool> getPortState(Types::Port port_id) const = 0;
    virtual vector<Types::Port> getPortList() const = 0;
    virtual string getStats() const = 0;
};

/**
 * @brief Simulated SAI implementation for testing
 *
 * Provides in-memory simulation of hardware operations using modern C++23.
 */
export class SimulatedSai : public SaiInterface {
public:
    SimulatedSai();
    ~SimulatedSai() override = default;

    // SaiInterface implementation
    bool createPort(Types::Port port_id) override;
    bool deletePort(Types::Port port_id) override;
    bool setPortState(Types::Port port_id, bool admin_state) override;
    std::optional<bool> getPortState(Types::Port port_id) const override;
    vector<Types::Port> getPortList() const override;
    string getStats() const override;

private:
    struct PortInfo {
        bool admin_state = false;
        bool oper_state = false;
        chrono::steady_clock::time_point created_at;
        Types::Count packets_in = 0;
        Types::Count packets_out = 0;
    };

    unordered_map<Types::Port, PortInfo> m_ports;
    mutable mutex m_ports_mutex;

    void updateOperState(Types::Port port_id);
};

} // export namespace MiniSonic::SAI
