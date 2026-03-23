#include "l2/l2_service.h"
#include <iostream>

L2Service::L2Service(SaiInterface& sai)
    : sai_(sai) {}

void L2Service::learn(const std::string& mac, int port) {
    mac_table_[mac] = port;
    sai_.add_fdb_entry(mac, port);
}

std::optional<int> L2Service::lookup(const std::string& mac) {
    if (mac_table_.contains(mac))
        return mac_table_[mac];
    return std::nullopt;
}

void L2Service::forward(const Packet& pkt, int out_port) {
    std::cout << "[L2] Forward " << pkt.dst_mac << " to port " << out_port << "\n";
}

void L2Service::flood(const Packet& pkt) {
    std::cout << "[L2] Flood " << pkt.dst_mac << " to all ports\n";
}

bool L2Service::handle(Packet& pkt) {
    learn(pkt.src_mac, pkt.ingress_port);

    auto out = lookup(pkt.dst_mac);
    if (out) {
        forward(pkt, *out);
        return true;
    }

    flood(pkt);
    return true;
}
