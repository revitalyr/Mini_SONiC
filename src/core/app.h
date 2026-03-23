#pragma once

#include "dataplane/pipeline.h"
#include "core/event_loop.h"
#include "sai/sai_interface.h"
#include "sai/simulated_sai.h"
#include <memory>

class App {
public:
    App();
    void run();

private:
    std::unique_ptr<SaiInterface> sai_;
    std::unique_ptr<Pipeline> pipeline_;
    std::unique_ptr<EventLoop> event_loop_;
};
