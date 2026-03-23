#include "l3/l3_service.h"
#include <iostream>

L3Service::L3Service(SaiInterface& sai)
    : sai_(sai) {}

void L3Service::add_route(const std::string& prefix,
                          int prefix_len,
                          const std::string& next_hop) {
    trie_.insert(prefix, prefix_len, next_hop);
    sai_.add_route(prefix, next_hop);
}

std::optional<std::string> L3Service::lookup(const std::string& ip) {
    return trie_.lookup(ip);
}

void L3Service::forward(const Packet& pkt, const std::string& next_hop) {
    std::cout << "[L3] Forward " << pkt.dst_ip << " via " << next_hop << "\n";
}

bool L3Service::handle(Packet& pkt) {
    auto nh = lookup(pkt.dst_ip);

    if (nh) {
        forward(pkt, *nh);
        return true;
    }

    return false;
}
