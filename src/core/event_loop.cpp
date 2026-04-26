#include "core/event_loop.h"
#include <chrono>
#include <thread>
#include <atomic>

namespace MiniSonic::Core {

EventLoop::EventLoop(DataPlane::Pipeline& pipeline)
    : m_pipeline(pipeline) {}

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::run() {
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
