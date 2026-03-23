#pragma once

#include "dataplane/packet.h"
#include "l2/l2_service.h"
#include "l3/l3_service.h"
#include "sai/sai_interface.h"

class Pipeline {
public:
    explicit Pipeline(SaiInterface& sai);
    void process(Packet& pkt);
    
    L2Service& get_l2() { return l2_; }
    L3Service& get_l3() { return l3_; }

private:
    L2Service l2_;
    L3Service l3_;
};
