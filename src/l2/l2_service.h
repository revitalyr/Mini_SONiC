#pragma once

#include "dataplane/packet.h"
#include "sai/sai_interface.h"
#include <unordered_map>
#include <optional>

class L2Service {
public:
    explicit L2Service(SaiInterface& sai);

    bool handle(Packet& pkt);

private:
    void learn(const std::string& mac, int port);
    std::optional<int> lookup(const std::string& mac);
    void forward(const Packet& pkt, int out_port);
    void flood(const Packet& pkt);

    SaiInterface& sai_;
    std::unordered_map<std::string, int> mac_table_;
};
