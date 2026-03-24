#pragma once

#include "dataplane/pipeline.h"
#include "common/types.hpp"
#include <thread>
#include <atomic>

namespace MiniSonic::Core {

class EventLoop {
public:
    explicit EventLoop(DataPlane::Pipeline& pipeline);
    ~EventLoop();

    // Rule of five
    EventLoop(const EventLoop& other) = delete;
    EventLoop& operator=(const EventLoop& other) = delete;
    EventLoop(EventLoop&& other) noexcept = delete;
    EventLoop& operator=(EventLoop&& other) noexcept = delete;

    void run();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

private:
    void generateTestPackets();

    DataPlane::Pipeline& m_pipeline;
    Types::AtomicBool m_running{true};
    Types::UniquePtr<std::thread> m_worker_thread;
};

} // namespace MiniSonic::Core
