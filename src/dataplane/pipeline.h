#pragma once

#include "dataplane/packet.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "sai/sai_interface.h"

namespace MiniSonic::DataPlane {

class Pipeline {
public:
    explicit Pipeline(Sai::SaiInterface& sai);
    ~Pipeline() = default;

    // Rule of five
    Pipeline(const Pipeline& other) = delete;
    Pipeline& operator=(const Pipeline& other) = delete;
    Pipeline(Pipeline&& other) noexcept = delete;
    Pipeline& operator=(Pipeline&& other) noexcept = delete;

    void process(Packet& pkt);
    
    [[nodiscard]] L2::L2Service& getL2() noexcept { return m_l2; }
    [[nodiscard]] const L2::L2Service& getL2() const noexcept { return m_l2; }
    [[nodiscard]] L3::L3Service& getL3() noexcept { return m_l3; }
    [[nodiscard]] const L3::L3Service& getL3() const noexcept { return m_l3; }

private:
    L2::L2Service m_l2;
    L3::L3Service m_l3;
};

} // namespace MiniSonic::DataPlane
