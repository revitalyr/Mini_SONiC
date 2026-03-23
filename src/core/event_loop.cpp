#include "core/event_loop.h"
#include <thread>
#include <chrono>

EventLoop::EventLoop(Pipeline& pipeline)
    : pipeline_(pipeline) {}

void EventLoop::run() {
    // Add some test routes
    pipeline_.get_l3().add_route("10.0.0.0", 8, "1.1.1.1");
    pipeline_.get_l3().add_route("10.1.0.0", 16, "2.2.2.2");
    pipeline_.get_l3().add_route("10.1.1.0", 24, "3.3.3.3");

    while (running_) {
        Packet pkt{
            .src_mac = "aa:bb:cc:dd:ee:ff",
            .dst_mac = "ff:ee:dd:cc:bb:aa",
            .src_ip = "10.1.1.100",
            .dst_ip = "10.1.1.42",
            .ingress_port = 1
        };

        pipeline_.process(pkt);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void EventLoop::stop() {
    running_ = false;
}
