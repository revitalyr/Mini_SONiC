#include "dataplane/pipeline.h"
#include <iostream>

Pipeline::Pipeline(SaiInterface& sai)
    : l2_(sai), l3_(sai) {}

void Pipeline::process(Packet& pkt) {
    if (l2_.handle(pkt)) return;
    if (l3_.handle(pkt)) return;

    std::cout << "[DROP] Packet dropped\n";
}
