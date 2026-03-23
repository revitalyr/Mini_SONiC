#pragma once

#include "dataplane/pipeline.h"
#include "core/event_loop.h"
#include "sai/sai_interface.h"
#include "sai/simulated_sai.h"
#include "common/types.hpp"

namespace MiniSonic::Core {

class App {
public:
    App();
    ~App() = default;

    // Rule of five
    App(const App& other) = delete;
    App& operator=(const App& other) = delete;
    App(App&& other) noexcept = delete;
    App& operator=(App&& other) noexcept = delete;

    void run();

    [[nodiscard]] bool isRunning() const noexcept;

private:
    Types::UniquePtr<Sai::SaiInterface> m_sai;
    Types::UniquePtr<DataPlane::Pipeline> m_pipeline;
    Types::UniquePtr<EventLoop> m_event_loop;
};

} // namespace MiniSonic::Core
