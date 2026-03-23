#include "core/app.h"

App::App() {
    sai_ = std::make_unique<SimulatedSai>();
    pipeline_ = std::make_unique<Pipeline>(*sai_);
    event_loop_ = std::make_unique<EventLoop>(*pipeline_);
}

void App::run() {
    event_loop_->run();
}
