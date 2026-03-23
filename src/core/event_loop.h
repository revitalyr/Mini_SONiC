#pragma once

#include "dataplane/pipeline.h"
#include <atomic>

class EventLoop {
public:
    explicit EventLoop(Pipeline& pipeline);

    void run();
    void stop();

private:
    Pipeline& pipeline_;
    std::atomic<bool> running_{true};
};
