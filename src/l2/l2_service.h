#pragma once

#include "dataplane/packet.h"
#include "sai/sai_interface.h"
#include "common/types.hpp"
#include <unordered_map>

namespace MiniSonic::L2 {

class L2Service {
public:
    explicit L2Service(Sai::SaiInterface& sai);
    ~L2Service() = default;

    // Rule of five
    L2Service(const L2Service& other) = delete;
    L2Service& operator=(const L2Service& other) = delete;
    L2Service(L2Service&& other) noexcept = delete;
    L2Service& operator=(L2Service&& other) noexcept = delete;

    bool handle(DataPlane::Packet& pkt);

private:
    void learn(const Types::MacAddress& mac, Types::Port port);
    [[nodiscard]] Types::Optional<Types::Port> lookup(const Types::MacAddress& mac) const;
    void forward(const DataPlane::Packet& pkt, Types::Port out_port);
    void flood(const DataPlane::Packet& pkt);

    Sai::SaiInterface& m_sai;
    Types::FdbTable m_mac_table;
};

} // namespace MiniSonic::L2
