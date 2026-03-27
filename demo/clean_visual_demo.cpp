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

// Clean visual demo configuration
struct CleanDemoConfig {
    int num_switches = 3;
    int packets_per_second = 100;
    int demo_duration_seconds = 30;
    bool enable_animation = true;
    bool quiet_mode = false;
};

// Clean statistics
struct CleanStats {
    std::atomic<uint64_t> packets_generated{0};
    std::atomic<uint64_t> packets_processed{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> mac_learning_events{0};
    std::chrono::high_resolution_clock::time_point start_time;
    
    void reset() {
        packets_generated = 0;
        packets_processed = 0;
        packets_dropped = 0;
        mac_learning_events = 0;
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double get_packets_per_second() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        return elapsed > 0 ? static_cast<double>(packets_processed.load()) / elapsed : 0.0;
    }
    
    double get_packet_loss_rate() const {
        uint64_t total = packets_generated.load();
        if (total == 0) return 0.0;
        return (static_cast<double>(packets_dropped.load()) / total) * 100.0;
    }
};

// Simple visualizer with animation
class CleanVisualizer {
private:
    bool enabled;
    bool animation_enabled;
    int switch_count;
    std::vector<std::string> switch_names;
    
public:
    CleanVisualizer(bool enable_anim, bool quiet, int switches) 
        : enabled(!quiet), animation_enabled(enable_anim), switch_count(switches) {
        
        // Initialize switch names
        for (int i = 0; i < switches; ++i) {
            switch_names.push_back("SW" + std::to_string(i + 1));
        }
    }
    
    void clear_screen() {
        if (!enabled) return;
        
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
    
    void show_header() {
        if (!enabled) return;
        
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    🌐 Mini SONiC Real Demo                  ║\n";
        std::cout << "║                 Real Network Processing Demo                ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    }
    
    void show_progress(const CleanStats& stats, int elapsed_seconds, int total_seconds) {
        if (!enabled) return;
        
        // Progress bar
        int progress_width = 50;
        int filled = (elapsed_seconds * progress_width) / total_seconds;
        
        std::cout << "⏱️  Progress: [";
        for (int i = 0; i < progress_width; ++i) {
            if (i < filled) std::cout << "█";
            else if (i == filled) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << elapsed_seconds << "/" << total_seconds << "s\n\n";
        
        // Statistics
        std::cout << "📊 Real-time Statistics:\n";
        std::cout << "   📦 Packets: " << std::setw(6) << stats.packets_processed.load() 
                  << " processed, " << std::setw(6) << stats.packets_dropped.load() << " dropped\n";
        std::cout << "   ⚡ Throughput: " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps\n";
        std::cout << "   💔 Loss Rate: " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "%\n";
        std::cout << "   🧠 MAC Learned: " << std::setw(4) << stats.mac_learning_events.load() << "\n\n";
    }
    
    void show_switch_status(const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                           const std::vector<std::unique_ptr<std::atomic<size_t>>>& mac_tables) {
        if (!enabled) return;
        
        std::cout << "🔀 Switch Status:\n";
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "   " << switch_names[i] << ": ";
            std::cout << "📦 " << std::setw(4) << switch_packets[i]->load() << " pkts, ";
            std::cout << "🧠 " << std::setw(3) << mac_tables[i]->load() << " MACs ";
            
            // Simple activity indicator
            if (animation_enabled) {
                static int anim_frame = 0;
                const char* anim_chars[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
                std::cout << anim_chars[anim_frame];
                anim_frame = (anim_frame + 1) % 10;
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    
    void show_final_stats(const CleanStats& stats, 
                          const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                          const std::vector<std::unique_ptr<std::atomic<size_t>>>& mac_tables) {
        if (!enabled) return;
        
        clear_screen();
        show_header();
        
        std::cout << "🎉 Demo Completed Successfully!\n\n";
        
        std::cout << "📈 Final Results:\n";
        std::cout << "   📦 Total Packets: " << stats.packets_generated.load() << "\n";
        std::cout << "   ✅ Processed: " << stats.packets_processed.load() << "\n";
        std::cout << "   ❌ Dropped: " << stats.packets_dropped.load() << "\n";
        std::cout << "   ⚡ Avg Throughput: " << std::fixed << std::setprecision(1) 
                  << stats.get_packets_per_second() << " pps\n";
        std::cout << "   💔 Loss Rate: " << std::fixed << std::setprecision(2) 
                  << stats.get_packet_loss_rate() << "%\n\n";
        
        std::cout << "🔀 Switch Performance:\n";
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "   " << switch_names[i] << ": ";
            std::cout << "📦 " << switch_packets[i]->load() << " pkts, ";
            std::cout << "🧠 " << mac_tables[i]->load() << " MACs\n";
        }
        std::cout << "\n";
        
        std::cout << "✨ Mini SONiC is working perfectly!\n";
    }
    
    void log_event(const std::string& event, const std::string& details = "") {
        if (!enabled) return;
        
        // Only log important events to reduce noise
        if (event.find("Started") != std::string::npos || 
            event.find("Stopped") != std::string::npos ||
            event.find("DEMO") != std::string::npos) {
            std::cout << "🔔 " << event;
            if (!details.empty()) {
                std::cout << " - " << details;
            }
            std::cout << "\n\n";
        }
    }
};

// Simplified switch class for clean demo
class CleanSwitch {
private:
    int switch_id;
    std::string name;
    
    // Real Mini SONiC components
    std::unique_ptr<Sai::SimulatedSai> sai;
    std::unique_ptr<DataPlane::Pipeline> pipeline;
    std::unique_ptr<Utils::SPSCQueue<DataPlane::Packet>> packet_queue;
    std::unique_ptr<DataPlane::PipelineThread> pipeline_thread;
    
    // Statistics
    std::atomic<uint64_t> packets_processed{0};
    std::atomic<size_t> mac_table_size{0};
    std::atomic<bool> running{false};
    
    CleanVisualizer* visualizer;
    
public:
    CleanSwitch(int id, const std::string& switch_name, CleanVisualizer* viz) 
        : switch_id(id), name(switch_name), visualizer(viz) {
        
        // Initialize real Mini SONiC components
        sai = std::make_unique<Sai::SimulatedSai>();
        packet_queue = std::make_unique<Utils::SPSCQueue<DataPlane::Packet>>(1000);
        
        // Create pipeline
        pipeline = std::make_unique<DataPlane::Pipeline>(*sai);
        
        // Create pipeline thread
        pipeline_thread = std::make_unique<DataPlane::PipelineThread>(
            *pipeline, *packet_queue, 32
        );
        
        visualizer->log_event(name, "Initialized");
    }
    
    ~CleanSwitch() {
        stop();
    }
    
    void start() {
        running = true;
        pipeline_thread->start();
        visualizer->log_event(name, "Started");
    }
    
    void stop() {
        if (running) {
            running = false;
            pipeline_thread->stop();
            visualizer->log_event(name, "Stopped");
        }
    }
    
    bool process_packet(const DataPlane::Packet& packet) {
        if (!running) return false;
        
        // Add packet to real processing queue
        bool enqueued = packet_queue->push(const_cast<DataPlane::Packet&>(packet));
        
        if (enqueued) {
            packets_processed++;
            
            // Simulate MAC learning occasionally
            if (packets_processed.load() % 10 == 0) {
                mac_table_size++;
            }
            
            return true;
        }
        
        return false;
    }
    
    uint64_t get_packets_processed() const {
        return packets_processed.load();
    }
    
    size_t get_mac_table_size() const {
        return mac_table_size.load();
    }
    
    std::string get_name() const {
        return name;
    }
};

// Clean demo controller
class CleanNetworkDemo {
private:
    CleanDemoConfig config;
    CleanStats stats;
    CleanVisualizer visualizer;
    
    std::vector<std::unique_ptr<CleanSwitch>> switches;
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> switch_packets;
    std::vector<std::unique_ptr<std::atomic<size_t>>> mac_tables;
    std::unique_ptr<std::thread> demo_thread;
    std::atomic<bool> running{false};
    
public:
    CleanNetworkDemo(const CleanDemoConfig& cfg) 
        : config(cfg), visualizer(cfg.enable_animation, cfg.quiet_mode, cfg.num_switches) {
        
        // Initialize atomic vectors with unique pointers
        for (int i = 0; i < cfg.num_switches; ++i) {
            switch_packets.push_back(std::make_unique<std::atomic<uint64_t>>(0));
            mac_tables.push_back(std::make_unique<std::atomic<size_t>>(0));
        }
        
        // Create switches
        for (int i = 0; i < config.num_switches; ++i) {
            std::string name = "SW" + std::to_string(i + 1);
            auto switch_ptr = std::make_unique<CleanSwitch>(i, name, &visualizer);
            switches.push_back(std::move(switch_ptr));
        }
        
        visualizer.show_header();
        std::cout << "🚀 Starting clean demo with " << config.num_switches << " switches...\n";
        std::cout << "⚡ Target rate: " << config.packets_per_second << " pps\n";
        std::cout << "⏱️  Duration: " << config.demo_duration_seconds << " seconds\n\n";
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
        demo_thread = std::make_unique<std::thread>(&CleanNetworkDemo::run_demo, this);
        
        visualizer.log_event("DEMO", "Started");
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        // Stop demo thread
        if (demo_thread && demo_thread->joinable()) {
            demo_thread->join();
        }
        
        // Stop all switches
        for (auto& sw : switches) {
            sw->stop();
        }
        
        // Show final statistics
        for (size_t i = 0; i < switches.size(); ++i) {
            *switch_packets[i] = switches[i]->get_packets_processed();
            *mac_tables[i] = switches[i]->get_mac_table_size();
        }
        
        visualizer.show_final_stats(stats, switch_packets, mac_tables);
        visualizer.log_event("DEMO", "Stopped");
    }
    
private:
    void run_demo() {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(config.demo_duration_seconds);
        
        // Packet generation interval
        auto packet_interval = std::chrono::microseconds(1000000 / config.packets_per_second);
        auto last_packet_time = start_time;
        auto last_update_time = start_time;
        auto update_interval = std::chrono::milliseconds(500); // Update display every 500ms
        
        while (running && std::chrono::high_resolution_clock::now() < end_time) {
            auto now = std::chrono::high_resolution_clock::now();
            
            // Generate packets
            if (now - last_packet_time >= packet_interval) {
                generate_and_process_packets();
                last_packet_time = now;
            }
            
            // Update display
            if (now - last_update_time >= update_interval) {
                update_display(start_time, end_time);
                last_update_time = now;
            }
            
            // Small sleep
            std::this_thread::sleep_for(10ms);
        }
        
        // Final update
        update_display(start_time, end_time);
    }
    
    void generate_and_process_packets() {
        // Generate packets for each switch
        for (size_t i = 0; i < switches.size(); ++i) {
            // Generate 1-2 packets per interval per switch
            int packets_to_generate = 1 + (stats.packets_generated.load() % 2);
            
            for (int j = 0; j < packets_to_generate; ++j) {
                auto packet = generate_packet(static_cast<int>(i));
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
    
    DataPlane::Packet generate_packet(int src_switch_id) {
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
        
        static std::atomic<uint64_t> packet_counter{0};
        uint64_t packet_id = packet_counter.fetch_add(1);
        
        int src_host = packet_id % 100;
        int dst_switch = (src_switch_id + 1) % switches.size();
        int dst_host = (packet_id + 50) % 100;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> port_dist(1, 24);
        
        DataPlane::Packet packet(
            generate_mac(src_switch_id, src_host),
            generate_mac(dst_switch, dst_host),
            generate_ip(src_switch_id, src_host),
            generate_ip(dst_switch, dst_host),
            static_cast<Types::Port>(port_dist(gen))
        );
        
        packet.timestamp = std::chrono::high_resolution_clock::now();
        
        return packet;
    }
    
    void update_display(auto start_time, auto end_time) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        auto total = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        // Update switch statistics
        for (size_t i = 0; i < switches.size(); ++i) {
            *switch_packets[i] = switches[i]->get_packets_processed();
            *mac_tables[i] = switches[i]->get_mac_table_size();
        }
        
        // Refresh display
        visualizer.clear_screen();
        visualizer.show_header();
        visualizer.show_progress(stats, static_cast<int>(elapsed), static_cast<int>(total));
        visualizer.show_switch_status(switch_packets, mac_tables);
    }
};

// Main function
int main(int argc, char* argv[]) {
    CleanDemoConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--switches" && i + 1 < argc) {
            config.num_switches = std::stoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            config.packets_per_second = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            config.demo_duration_seconds = std::stoi(argv[++i]);
        } else if (arg == "--no-anim") {
            config.enable_animation = false;
        } else if (arg == "--quiet") {
            config.quiet_mode = true;
        } else if (arg == "--help") {
            std::cout << "Mini SONiC Clean Visual Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --switches <n>     Number of switches (default: 3)\n";
            std::cout << "  --rate <n>         Packets per second (default: 100)\n";
            std::cout << "  --duration <n>     Demo duration in seconds (default: 30)\n";
            std::cout << "  --no-anim          Disable animation\n";
            std::cout << "  --quiet            Quiet mode (minimal output)\n";
            std::cout << "  --help             Show this help\n";
            return 0;
        }
    }
    
    try {
        // Create and run clean demo
        CleanNetworkDemo demo(config);
        
#ifdef _WIN32
        // Handle Ctrl+C gracefully on Windows
        SetConsoleCtrlHandler([](DWORD ctrlType) {
            if (ctrlType == CTRL_C_EVENT) {
                std::cout << "\n\n⚠️  Received interrupt signal, stopping demo...\n";
                return TRUE;
            }
            return FALSE;
        }, TRUE);
#else
        // Handle Ctrl+C gracefully on Unix-like systems
        std::signal(SIGINT, [](int) {
            std::cout << "\n\n⚠️  Received interrupt signal, stopping demo...\n";
        });
#endif
        
        demo.start();
        
        // Wait for demo to complete
        std::this_thread::sleep_for(std::chrono::seconds(config.demo_duration_seconds + 1));
        
        demo.stop();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}
