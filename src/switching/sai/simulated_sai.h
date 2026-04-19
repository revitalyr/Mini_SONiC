#pragma once

#include "sai/sai_interface.h"
#include "common/types.hpp"
#include <unordered_set>

namespace MiniSonic::Sai {

class SimulatedSai : public SaiInterface {
public:
    SimulatedSai() = default;
    ~SimulatedSai() override = default;

    // Rule of five
    SimulatedSai(const SimulatedSai& other) = delete;
    SimulatedSai& operator=(const SimulatedSai& other) = delete;
    SimulatedSai(SimulatedSai&& other) noexcept = delete;
    SimulatedSai& operator=(SimulatedSai&& other) noexcept = delete;

    void createPort(Types::Port port_id) override;
    void removePort(Types::Port port_id) override;
    void addFdbEntry(const Types::MacAddress& mac, Types::Port port) override;
    void removeFdbEntry(const Types::MacAddress& mac) override;
    void addRoute(
        const Types::String& prefix,
        const Types::NextHop& next_hop
    ) override;
    void removeRoute(const Types::String& prefix) override;

    // Getters for testing/monitoring
    [[nodiscard]] const Types::Map<Types::Port, bool>& getPorts() const noexcept { return m_ports; }
    [[nodiscard]] const Types::FdbTable& getFdbTable() const noexcept { return m_fdb; }
    [[nodiscard]] const Types::RoutingTable& getRoutingTable() const noexcept { return m_routes; }

private:
    Types::Map<Types::Port, bool> m_ports;
    Types::FdbTable m_fdb;
    Types::RoutingTable m_routes;
};

} // namespace MiniSonic::Sai
