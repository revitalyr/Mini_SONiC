#include "core/event_loop.h"
#include <chrono>

namespace MiniSonic::Core {

EventLoop::EventLoop(DataPlane::Pipeline& pipeline)
    : m_pipeline(pipeline) {}

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::run() {
    // Add some test routes
    m_pipeline.getL3().addRoute("10.0.0.0", 8, "1.1.1.1");
    m_pipeline.getL3().addRoute("10.1.0.0", 16, "2.2.2.2");
    m_pipeline.getL3().addRoute("10.1.1.0", 24, "3.3.3.3");

    while (m_running.load()) {
        generateTestPackets();

        std::this_thread::sleep_for(Types::Milliseconds(500));
    }
}

void EventLoop::stop() {
    m_running.store(false);
    if (m_worker_thread && m_worker_thread->joinable()) {
        m_worker_thread->join();
    }
}

void EventLoop::generateTestPackets() {
    DataPlane::Packet pkt(
        "aa:bb:cc:dd:ee:ff",  // src_mac
        "ff:ee:dd:cc:bb:aa",  // dst_mac
        "10.1.1.100",         // src_ip
        "10.1.1.42",          // dst_ip
        1                     // ingress_port
    );

    m_pipeline.process(pkt);
}

} // namespace MiniSonic::Core
