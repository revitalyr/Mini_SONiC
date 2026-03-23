#include "core/app.h"

namespace MiniSonic::Core {

App::App() {
    m_sai = std::make_unique<Sai::SimulatedSai>();
    m_pipeline = std::make_unique<DataPlane::Pipeline>(*m_sai);
    m_event_loop = std::make_unique<EventLoop>(*m_pipeline);
}

void App::run() {
    m_event_loop->run();
}

bool App::isRunning() const noexcept {
    return m_event_loop && m_event_loop->isRunning();
}

} // namespace MiniSonic::Core
