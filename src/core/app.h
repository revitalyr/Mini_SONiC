#pragma once

#include "dataplane/pipeline.h"
#include "dataplane/pipeline_thread.h"
#include "core/event_loop.h"
#include "sai/sai_interface.h"
#include "sai/simulated_sai.h"
#include "link/tcp_link_async.h"
#include "utils/spsc_queue.hpp"
#include "utils/metrics.hpp"
#include "common/types.hpp"

#include <boost/asio.hpp>
#include <memory>

namespace MiniSonic::Core {

class App {
public:
    /**
     * @brief Construct application with async architecture
     * @param listen_port Port for this switch instance
     * @param peer_ip Peer switch IP (optional)
     * @param peer_port Peer switch port (optional)
     */
    App(
        Types::Port listen_port = 9000,
        const Types::String& peer_ip = "",
        Types::Port peer_port = 0
    );
    
    ~App() = default;

    // Rule of five
    App(const App& other) = delete;
    App& operator=(const App& other) = delete;
    App(App&& other) noexcept = delete;
    App& operator=(App&& other) noexcept = delete;

    void run();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Get application statistics
     */
    [[nodiscard]] Types::String getStats() const;

private:
    /**
     * @brief Initialize async components
     */
    void initialize();

    /**
     * @brief Start packet generator thread
     */
    void startPacketGenerator();

    /**
     * @brief Generate test packets
     */
    void generateTestPackets();

    // Core components
    Types::UniquePtr<boost::asio::io_context> m_io_context;
    Types::UniquePtr<Sai::SaiInterface> m_sai;
    Types::UniquePtr<DataPlane::Pipeline> m_pipeline;
    Types::UniquePtr<Utils::SPSCQueue<DataPlane::Packet>> m_packet_queue;
    Types::UniquePtr<DataPlane::PipelineThread> m_pipeline_thread;
    Types::UniquePtr<Link::TcpLinkAsync> m_tcp_link;
    Types::UniquePtr<EventLoop> m_event_loop;

    // Configuration
    const Types::Port m_listen_port;
    const Types::String m_peer_ip;
    const Types::Port m_peer_port;

    // State
    Types::AtomicBool m_running{false};
    Types::UniquePtr<std::thread> m_asio_thread;
    Types::UniquePtr<std::thread> m_generator_thread;
};

} // namespace MiniSonic::Core
