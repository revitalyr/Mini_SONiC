#pragma once
#include <string>

struct Packet {
    std::string src_mac;
    std::string dst_mac;
    std::string src_ip;
    std::string dst_ip;
    int ingress_port;
};
