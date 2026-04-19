module;

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

module MiniSonic.DataPlane;

// Import local modules
import MiniSonic.SAI;
import MiniSonic.Events;

namespace MiniSonic::DataPlane {

// Pipeline Implementation
Pipeline::Pipeline(SAI::SaiInterface& sai, const string& switch_id)
    : m_sai(sai),
      m_switch_id(switch_id),
      m_event_bus(Events::getGlobalEventBus()) {
}

void Pipeline::process(Packet& pkt) {
    // Assign packet ID if not set
    if (pkt.id() == 0) {
        pkt.setId(m_packet_counter.fetch_add(1) + 1);
    }

    // Emit PacketEnteredSwitch event
    auto entered_event = std::make_shared<Events::PacketEnteredSwitch>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        pkt.id(),
        "Eth" + std::to_string(pkt.ingressPort())
    );
    m_event_bus.publish(entered_event);

    // Emit PipelineStageEntered event for Parser
    auto parser_entered = std::make_shared<Events::PipelineStageEntered>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        pkt.id(),
        "Parser"
    );
    m_event_bus.publish(parser_entered);

    // Stub implementation - actual processing would go here
    // This would include L2 lookup, L3 lookup, ACL check, QoS classification, etc.

    // Emit PipelineStageExited event for Parser
    auto parser_exited = std::make_shared<Events::PipelineStageExited>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        pkt.id(),
        "Parser",
        100  // 100ns latency
    );
    m_event_bus.publish(parser_exited);

    // Emit PacketForwardDecision event
    auto forward_event = std::make_shared<Events::PacketForwardDecision>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        pkt.id(),
        "Eth1",
        "0.0.0.0/0",
        "0x1234",
        "SWITCH1"
    );
    m_event_bus.publish(forward_event);

    // Emit PacketExitedSwitch event
    auto exited_event = std::make_shared<Events::PacketExitedSwitch>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count(),
        m_switch_id,
        pkt.id(),
        "Eth1"
    );
    m_event_bus.publish(exited_event);
}

std::string Pipeline::getStats() const {
    std::ostringstream oss;
    oss << "Pipeline stats:\n"
       << "  Switch ID: " << m_switch_id << "\n"
       << "  Packets processed: " << m_packet_counter.load() << "\n";
    return oss.str();
}

// PipelineThread Implementation
PipelineThread::PipelineThread(
    Pipeline& pipeline, 
    SPSCQueue<Packet>& queue,
    Types::Count batch_size
) : m_pipeline(pipeline),
    m_queue(queue),
    m_batch_size(batch_size) {
}

PipelineThread::~PipelineThread() {
    stop();
}

void PipelineThread::start() {
    if (m_running.load()) {
        return; // Already running
    }

    m_running.store(true);
    m_thread = std::thread(&PipelineThread::run, this);
    
    // std::cout << "[PIPELINE] Started pipeline thread with batch size " 
    //           << m_batch_size << "\n";
}

void PipelineThread::stop() {
    if (!m_running.load()) {
        return; // Already stopped
    }

    m_running.store(false);
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    // std::cout << "[PIPELINE] Stopped pipeline thread\n";
}

bool PipelineThread::isRunning() const noexcept {
    return m_running.load();
}

std::string PipelineThread::getStats() const {
    std::ostringstream oss;
    oss << "Pipeline Thread Stats:\n"
         << "  Running: " << (m_running.load() ? "Yes" : "No") << "\n"
         << "  Packets Processed: " << m_packets_processed.load() << "\n"
         << "  Batches Processed: " << m_batches_processed.load() << "\n"
         << "  Empty Cycles: " << m_empty_cycles.load() << "\n"
         << "  Batch Size: " << m_batch_size << "\n"
         << "  Queue Size: " << m_queue.size() << "/" << m_queue.capacity() << "\n";
    
    const auto total_time = m_total_processing_time_us.load();
    const auto packets = m_packets_processed.load();
    if (packets > 0) {
        oss << "  Avg Processing Time: " << (total_time / packets) << " μs/packet\n";
        oss << "  Throughput: " << (packets * 1000000 / total_time) << " packets/sec\n";
    }
    
    return oss.str();
}

void PipelineThread::run() {
    std::vector<Packet> batch;
    batch.reserve(m_batch_size);
    
    while (m_running.load()) {
        batch.clear();
        
        // Collect batch of packets
        Packet pkt;
        Types::Count collected = 0;
        
        while (collected < m_batch_size && m_queue.pop(pkt)) {
            markPacketTimestamp(pkt);
            batch.push_back(std::move(pkt));
            ++collected;
        }
        
        if (!batch.empty()) {
            processBatch(batch);
            m_batches_processed.fetch_add(1, std::memory_order_relaxed);
        } else {
            // No packets available - brief sleep to yield CPU
            m_empty_cycles.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

void PipelineThread::processBatch(std::vector<Packet>& batch) {
    const auto start_time = Clock::now();
    
    // Process each packet in the batch
    for (auto& pkt : batch) {
        m_pipeline.process(pkt);
        m_packets_processed.fetch_add(1, std::memory_order_relaxed);
        
        // Calculate and record latency
        const auto now = Clock::now();
        const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now - pkt.timestamp
        ).count();
        
        // Note: This would need Metrics module integration
        // Utils::Metrics::instance().addLatency(latency_us);
    }
    
    const auto end_time = Clock::now();
    const auto processing_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    ).count();
    
    m_total_processing_time_us.fetch_add(processing_time_us, std::memory_order_relaxed);
    
    // Update metrics
    // Utils::Metrics::instance().inc("pipeline_batches");
    // Utils::Metrics::instance().inc("pipeline_packets", batch.size());
}

void PipelineThread::markPacketTimestamp(Packet& pkt) {
    // Add timestamp field to packet for latency measurement
    pkt.updateTimestamp();
}

} // export namespace MiniSonic::DataPlane
