#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <csignal>
#endif

// Include real Mini SONiC components
#include "../src/core/app_basic.h"
#include "../src/dataplane/pipeline.h"
#include "../src/dataplane/pipeline_thread.h"
#include "../src/l2/l2_service.h"
#include "../src/l3/l3_service.h"
#include "../src/sai/simulated_sai.h"
#include "../src/utils/spsc_queue.hpp"
#include "../src/common/types.hpp"

using namespace MiniSonic;
using namespace std::chrono_literals;

// Demo configuration
struct DemoConfig {
    int num_switches = 3;
    int packets_per_second = 100;
    int demo_duration_seconds = 30;
    bool enable_visualization = true;
    bool enable_logging = true;
    std::string log_file = "demo_output.log";
};

// Statistics tracking
struct DemoStats {
    std::atomic<uint64_t> packets_generated{0};
    std::atomic<uint64_t> packets_processed{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> mac_learning_events{0};
    std::atomic<uint64_t> route_lookup_events{0};
    std::atomic<uint64_t> total_processing_time_us{0};
    std::chrono::high_resolution_clock::time_point start_time;
    
    void reset() {
        packets_generated = 0;
        packets_processed = 0;
        packets_dropped = 0;
        mac_learning_events = 0;
        route_lookup_events = 0;
        total_processing_time_us = 0;
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double get_packets_per_second() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        return elapsed > 0 ? static_cast<double>(packets_processed.load()) / elapsed : 0.0;
    }
    
    double get_average_latency_us() const {
        uint64_t processed = packets_processed.load();
        return processed > 0 ? static_cast<double>(total_processing_time_us.load()) / processed : 0.0;
    }
};

// Visualization output
class Visualizer {
private:
    std::ofstream log_file;
    bool enabled;
    
public:
    Visualizer(const std::string& filename, bool enable = true) : enabled(enable) {
        if (enabled) {
            log_file.open(filename);
            if (log_file.is_open()) {
                log_file << "Mini SONiC Real Demo - Event Log\n";
                log_file << "=====================================\n\n";
            }
        }
    }
    
    ~Visualizer() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }
    
    void log_switch_state(int switch_id, const std::string& state, const std::string& details = "") {
        if (!enabled) return;
        
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        std::cout << "[" << timestamp << "] SW" << switch_id << ": " << state;
        if (!details.empty()) {
            std::cout << " - " << details;
        }
        std::cout << std::endl;
        
        if (log_file.is_open()) {
            log_file << "[" << timestamp << "] SW" << switch_id << ": " << state;
            if (!details.empty()) {
                log_file << " - " << details;
            }
            log_file << "\n";
            log_file.flush();
        }
    }
    
    void log_packet_flow(int packet_id, const std::string& from, const std::string& to, const std::string& action) {
        if (!enabled) return;
        
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        std::cout << "[" << timestamp << "] PKT" << packet_id << ": " << from << " -> " << to << " (" << action << ")" << std::endl;
        
        if (log_file.is_open()) {
            log_file << "[" << timestamp << "] PKT" << packet_id << ": " << from << " -> " << to << " (" << action << ")\n";
            log_file.flush();
        }
    }
    
    void log_statistics(const DemoStats& stats) {
        if (!enabled) return;
        
        std::cout << "\n=== REAL-TIME STATISTICS ===\n";
        std::cout << "Packets Generated: " << stats.packets_generated.load() << "\n";
        std::cout << "Packets Processed: " << stats.packets_processed.load() << "\n";
        std::cout << "Packets Dropped: " << stats.packets_dropped.load() << "\n";
        std::cout << "MAC Learning Events: " << stats.mac_learning_events.load() << "\n";
        std::cout << "Route Lookups: " << stats.route_lookup_events.load() << "\n";
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << stats.get_packets_per_second() << " pps\n";
        std::cout << "Average Latency: " << std::setprecision(2) << stats.get_average_latency_us() << " μs\n";
        std::cout << "============================\n\n";
        
        if (log_file.is_open()) {
            log_file << "\n=== STATISTICS ===\n";
            log_file << "Generated: " << stats.packets_generated.load() << "\n";
            log_file << "Processed: " << stats.packets_processed.load() << "\n";
            log_file << "Dropped: " << stats.packets_dropped.load() << "\n";
            log_file << "MAC Learning: " << stats.mac_learning_events.load() << "\n";
            log_file << "Route Lookups: " << stats.route_lookup_events.load() << "\n";
            log_file << "Throughput: " << std::fixed << std::setprecision(2) << stats.get_packets_per_second() << " pps\n";
            log_file << "Latency: " << std::setprecision(2) << stats.get_average_latency_us() << " μs\n";
            log_file << "==================\n\n";
            log_file.flush();
        }
    }
};

// Real Switch implementation using Mini SONiC components
class RealSwitch {
private:
    int switch_id;
    std::string name;
    
    // Real Mini SONiC components
    std::unique_ptr<Sai::SimulatedSai> sai;
    std::unique_ptr<DataPlane::Pipeline> pipeline;
    std::unique_ptr<Utils::SPSCQueue<DataPlane::Packet>> packet_queue;
    std::unique_ptr<DataPlane::PipelineThread> pipeline_thread;
    
    // Switch state
    std::map<Types::MacAddress, Types::Port> mac_table;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> packets_processed{0};
    
    Visualizer* visualizer;
    
public:
    RealSwitch(int id, const std::string& switch_name, Visualizer* viz) 
        : switch_id(id), name(switch_name), visualizer(viz) {
        
        // Initialize real Mini SONiC components
        sai = std::make_unique<Sai::SimulatedSai>();
        packet_queue = std::make_unique<Utils::SPSCQueue<DataPlane::Packet>>(1000);
        
        // Create pipeline with real services
        pipeline = std::make_unique<DataPlane::Pipeline>(*sai);
        
        // Create pipeline thread
        pipeline_thread = std::make_unique<DataPlane::PipelineThread>(
            *pipeline, *packet_queue, 32
        );
        
        visualizer->log_switch_state(switch_id, "Initialized", "Real Mini SONiC components loaded");
    }
    
    ~RealSwitch() {
        stop();
    }
    
    void start() {
        running = true;
        pipeline_thread->start();
        visualizer->log_switch_state(switch_id, "Started", "Pipeline thread active");
    }
    
    void stop() {
        if (running) {
            running = false;
            pipeline_thread->stop();
            visualizer->log_switch_state(switch_id, "Stopped", "Pipeline thread terminated");
        }
    }
    
    bool process_packet(const DataPlane::Packet& packet) {
        if (!running) return false;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Log packet arrival
        visualizer->log_packet_flow(
            static_cast<int>(packet.m_ingress_port),
            "INGRESS",
            name,
            "Packet received"
        );
        
        // Add packet to real processing queue
        bool enqueued = packet_queue->push(const_cast<DataPlane::Packet&>(packet));
        
        if (enqueued) {
            packets_processed++;
            
            // Simulate MAC learning
            auto it = mac_table.find(packet.m_src_mac);
            if (it == mac_table.end()) {
                mac_table[packet.m_src_mac] = packet.m_ingress_port;
                visualizer->log_switch_state(switch_id, "MAC Learning", 
                    "Learned MAC " + packet.m_src_mac + " on port " + std::to_string(packet.m_ingress_port));
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time).count();
            
            visualizer->log_packet_flow(
                static_cast<int>(packet.m_ingress_port),
                name,
                "EGRESS",
                "Processed in " + std::to_string(processing_time) + "μs"
            );
            
            return true;
        } else {
            visualizer->log_packet_flow(
                static_cast<int>(packet.m_ingress_port),
                name,
                "DROP",
                "Queue full"
            );
            return false;
        }
    }
    
    uint64_t get_packets_processed() const {
        return packets_processed.load();
    }
    
    size_t get_mac_table_size() const {
        return mac_table.size();
    }
    
    std::string get_name() const {
        return name;
    }
    
    int get_id() const {
        return switch_id;
    }
};

// Packet generator using real Mini SONiC packet format
class PacketGenerator {
private:
    std::atomic<uint64_t> packet_counter{0};
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> port_dist;
    std::uniform_int_distribution<> switch_dist;
    
    Visualizer* visualizer;
    
public:
    PacketGenerator(Visualizer* viz) : gen(rd()), port_dist(1, 24), switch_dist(0, 2), visualizer(viz) {}
    
    DataPlane::Packet generate_packet(int src_switch_id) {
        uint64_t packet_id = packet_counter.fetch_add(1);
        
        // Generate realistic MAC addresses
        auto generate_mac = [](int switch_id, int host_id) -> Types::MacAddress {
            std::ostringstream mac;
            mac << std::hex << std::setfill('0') << std::setw(2) << (0x00 + switch_id) << ":"
                << std::setw(2) << (0x11) << ":"
                << std::setw(2) << (0x22) << ":"
                << std::setw(2) << (0x33) << ":"
                << std::setw(2) << (0x44) << ":"
                << std::setw(2) << host_id;
            return mac.str();
        };
        
        // Generate realistic IP addresses
        auto generate_ip = [](int switch_id, int host_id) -> Types::IpAddress {
            return "10." + std::to_string(switch_id + 1) + "." + 
                   std::to_string(host_id / 256 + 1) + "." + 
                   std::to_string(host_id % 256 + 1);
        };
        
        int src_host = packet_id % 100;
        int dst_switch = switch_dist(gen);
        int dst_host = (packet_id + 50) % 100;
        
        // Ensure different destination
        while (dst_switch == src_switch_id) {
            dst_switch = switch_dist(gen);
        }
        
        DataPlane::Packet packet(
            generate_mac(src_switch_id, src_host),
            generate_mac(dst_switch, dst_host),
            generate_ip(src_switch_id, src_host),
            generate_ip(dst_switch, dst_host),
            static_cast<Types::Port>(port_dist(gen))
        );
        
        packet.timestamp = std::chrono::high_resolution_clock::now();
        
        visualizer->log_packet_flow(
            static_cast<int>(packet_id),
            "GENERATOR",
            "SW" + std::to_string(src_switch_id),
            "Generated packet"
        );
        
        return packet;
    }
};

// Real network demo controller
class RealNetworkDemo {
private:
    DemoConfig config;
    DemoStats stats;
    Visualizer visualizer;
    
    std::vector<std::unique_ptr<RealSwitch>> switches;
    std::unique_ptr<PacketGenerator> packet_generator;
    std::thread demo_thread;
    std::atomic<bool> running{false};
    
public:
    RealNetworkDemo(const DemoConfig& cfg) 
        : config(cfg), visualizer(cfg.log_file, cfg.enable_logging) {
        
        packet_generator = std::make_unique<PacketGenerator>(&visualizer);
        
        // Create real switches
        for (int i = 0; i < config.num_switches; ++i) {
            std::string name = "RealSW" + std::to_string(i + 1);
            auto switch_ptr = std::make_unique<RealSwitch>(i, name, &visualizer);
            switches.push_back(std::move(switch_ptr));
        }
        
        std::cout << "=== Mini SONiC REAL Network Demo ===\n";
        std::cout << "Switches: " << config.num_switches << "\n";
        std::cout << "Packet Rate: " << config.packets_per_second << " pps\n";
        std::cout << "Duration: " << config.demo_duration_seconds << " seconds\n";
        std::cout << "Log File: " << config.log_file << "\n";
        std::cout << "=====================================\n\n";
    }
    
    void start() {
        if (running) return;
        
        running = true;
        stats.reset();
        
        // Start all switches
        for (auto& sw : switches) {
            sw->start();
        }
        
        // Start demo thread
        demo_thread = std::thread(&RealNetworkDemo::run_demo, this);
        
        visualizer.log_switch_state(0, "DEMO", "Real network demo started");
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        // Stop demo thread
        if (demo_thread.joinable()) {
            demo_thread.join();
        }
        
        // Stop all switches
        for (auto& sw : switches) {
            sw->stop();
        }
        
        visualizer.log_statistics(stats);
        visualizer.log_switch_state(0, "DEMO", "Real network demo stopped");
    }
    
    void run_demo() {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(config.demo_duration_seconds);
        
        // Statistics reporting interval
        auto last_stats_time = start_time;
        auto stats_interval = std::chrono::seconds(5);
        
        // Packet generation interval
        auto packet_interval = std::chrono::microseconds(1000000 / config.packets_per_second);
        auto last_packet_time = start_time;
        
        while (running && std::chrono::high_resolution_clock::now() < end_time) {
            auto now = std::chrono::high_resolution_clock::now();
            
            // Generate packets
            if (now - last_packet_time >= packet_interval) {
                generate_and_process_packets();
                last_packet_time = now;
            }
            
            // Report statistics
            if (now - last_stats_time >= stats_interval) {
                visualizer.log_statistics(stats);
                last_stats_time = now;
            }
            
            // Small sleep to prevent CPU overload
            std::this_thread::sleep_for(10ms);
        }
        
        // Final statistics
        visualizer.log_statistics(stats);
        
        // Print switch-specific statistics
        std::cout << "\n=== SWITCH STATISTICS ===\n";
        for (const auto& sw : switches) {
            std::cout << sw->get_name() << ":\n";
            std::cout << "  Packets Processed: " << sw->get_packets_processed() << "\n";
            std::cout << "  MAC Table Size: " << sw->get_mac_table_size() << "\n";
        }
        std::cout << "========================\n\n";
    }
    
private:
    void generate_and_process_packets() {
        // Generate packets for each switch
        for (size_t i = 0; i < switches.size(); ++i) {
            // Generate 1-3 packets per interval per switch
            int packets_to_generate = 1 + (stats.packets_generated.load() % 3);
            
            for (int j = 0; j < packets_to_generate; ++j) {
                auto packet = packet_generator->generate_packet(static_cast<int>(i));
                stats.packets_generated++;
                
                // Process packet through real switch
                if (switches[i]->process_packet(packet)) {
                    stats.packets_processed++;
                } else {
                    stats.packets_dropped++;
                }
            }
        }
    }
};

// Main function
int main(int argc, char* argv[]) {
    DemoConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--switches" && i + 1 < argc) {
            config.num_switches = std::stoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            config.packets_per_second = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            config.demo_duration_seconds = std::stoi(argv[++i]);
        } else if (arg == "--no-viz") {
            config.enable_visualization = false;
        } else if (arg == "--help") {
            std::cout << "Mini SONiC Real Network Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --switches <n>     Number of switches (default: 3)\n";
            std::cout << "  --rate <n>         Packets per second (default: 100)\n";
            std::cout << "  --duration <n>     Demo duration in seconds (default: 30)\n";
            std::cout << "  --no-viz           Disable visualization\n";
            std::cout << "  --help             Show this help\n";
            return 0;
        }
    }
    
    try {
        // Create and run real demo
        RealNetworkDemo demo(config);
        
#ifdef _WIN32
        // Handle Ctrl+C gracefully on Windows
        SetConsoleCtrlHandler([](DWORD ctrlType) {
            if (ctrlType == CTRL_C_EVENT) {
                std::cout << "\nReceived interrupt signal, stopping demo...\n";
                // Demo will be stopped automatically in destructor
                return TRUE;
            }
            return FALSE;
        }, TRUE);
#else
        // Handle Ctrl+C gracefully on Unix-like systems
        std::signal(SIGINT, [](int) {
            std::cout << "\nReceived interrupt signal, stopping demo...\n";
            // Demo will be stopped automatically in destructor
        });
#endif
        
        demo.start();
        
        // Wait for demo to complete or interrupt
        std::this_thread::sleep_for(std::chrono::seconds(config.demo_duration_seconds + 1));
        
        demo.stop();
        
        std::cout << "Demo completed successfully!\n";
        std::cout << "Check " << config.log_file << " for detailed logs.\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}
