#pragma once

#include "common/types.hpp"

namespace MiniSonic::Sai {

class SaiInterface {
public:
    virtual ~SaiInterface() = default;

    virtual void createPort(Types::Port port_id) = 0;
    virtual void removePort(Types::Port port_id) = 0;

    virtual void addFdbEntry(const Types::MacAddress& mac, Types::Port port) = 0;
    virtual void removeFdbEntry(const Types::MacAddress& mac) = 0;

    virtual void addRoute(
        const Types::String& prefix,
        const Types::NextHop& next_hop
    ) = 0;
    virtual void removeRoute(const Types::String& prefix) = 0;
};

} // namespace MiniSonic::Sai
