module;

// Import standard library modules
import <memory>;
import <string>;
import <vector>;
import <unordered_map>;
import <atomic>;
import <iostream>;
import <sstream>;

export module MiniSonic.SAI;

export namespace MiniSonic::SAI {

/**
 * @brief Abstract SAI (Switch Abstraction Interface)
 * 
 * Provides hardware abstraction for switch operations.
 */
export class SaiInterface {
public:
    virtual ~SaiInterface() = default;
    
    // Virtual methods for hardware operations
    virtual bool createPort(Types::Port port_id) = 0;
    virtual bool deletePort(Types::Port port_id) = 0;
    virtual bool setPortState(Types::Port port_id, bool admin_state) = 0;
    virtual bool getPortState(Types::Port port_id) const = 0;
    virtual std::vector<Types::Port> getPortList() const = 0;
    virtual std::string getStats() const = 0;
};

/**
 * @brief Simulated SAI implementation for testing
 * 
 * Provides in-memory simulation of hardware operations.
 */
export class SimulatedSai : public SaiInterface {
public:
    SimulatedSai();
    ~SimulatedSai() override = default;
    
    // SaiInterface implementation
    bool createPort(Types::Port port_id) override;
    bool deletePort(Types::Port port_id) override;
    bool setPortState(Types::Port port_id, bool admin_state) override;
    bool getPortState(Types::Port port_id) const override;
    std::vector<Types::Port> getPortList() const override;
    std::string getStats() const override;

private:
    struct PortInfo {
        bool admin_state = false;
        bool oper_state = false;
        std::chrono::steady_clock::time_point created_at;
        Types::Count packets_in = 0;
        Types::Count packets_out = 0;
    };
    
    std::unordered_map<Types::Port, PortInfo> m_ports;
    mutable std::mutex m_ports_mutex;
    
    void updateOperState(Types::Port port_id);
};

} // export namespace MiniSonic::SAI
