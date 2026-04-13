#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <iomanip>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std::chrono_literals;

// Visual TUI Demo Configuration
struct VisualConfig {
    int num_switches = 3;
    int packets_per_second = 2;
    int demo_duration_seconds = 30;
    int refresh_interval_ms = 1500;
};

// Visual Statistics
struct VisualStats {
    std::atomic<uint64_t> packets_processed{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> mac_learning_events{0};
    std::atomic<uint64_t> frame_number{0};
    std::chrono::high_resolution_clock::time_point start_time;
    
    void reset() {
        packets_processed = 0;
        packets_dropped = 0;
        mac_learning_events = 0;
        frame_number = 0;
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double get_packets_per_second() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        return elapsed > 0 ? static_cast<double>(packets_processed.load()) / elapsed : 0.0;
    }
    
    double get_packet_loss_rate() const {
        uint64_t total = packets_processed.load() + packets_dropped.load();
        if (total == 0) return 0.0;
        return (static_cast<double>(packets_dropped.load()) / total) * 100.0;
    }
};

// Visual TUI Visualizer
class VisualTUI {
private:
    int switch_count;
    std::vector<std::string> switch_names;
    int animation_frame = 0;
    std::vector<int> switch_activity;
    std::vector<std::string> packet_flows;
    
public:
    VisualTUI(int switches) : switch_count(switches) {
        for (int i = 0; i < switches; ++i) {
            switch_names.push_back("SWITCH-" + std::to_string(i + 1));
        }
        switch_activity.resize(switch_count, 0);
        packet_flows.resize(switch_count, "");
        
#ifdef _WIN32
        // Hide cursor
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
#endif
    }
    
    void clear_screen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
    
    void show_header(const VisualStats& stats) {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              Mini SONiC - VISUAL TUI NETWORK DEMO                 ║\n";
        std::cout << "║                    Real Network Visualization                   ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - stats.start_time
        ).count();
        
        std::cout << "FRAME: " << std::setw(4) << stats.frame_number.load() 
                  << " | Time: " << std::setw(3) << elapsed << "s\n\n";
    }
    
    void show_network_topology(const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                               const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& mac_tables) {
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                        NETWORK TOPOLOGY                           ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
        
        // Draw switches with visual representation
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "  ";
            
            // Switch status
            if (switch_activity[i] > 0) {
                std::cout << "🔴 ";
                switch_activity[i]--;
            } else {
                std::cout << "⚪ ";
            }
            
            // Switch box
            std::cout << "┌─────────────────────┐ ";
            
            // Connection to next switch
            if (i < switch_count - 1) {
                if (switch_activity[i] > 0 || switch_activity[i+1] > 0) {
                    std::cout << "═════════════════════ ";
                } else {
                    std::cout << "───────────────────── ";
                }
            }
            std::cout << "\n";
            
            // Switch name and stats
            std::cout << "  │ " << std::setw(16) << std::left << switch_names[i] << " │ ";
            if (i < switch_count - 1) {
                std::cout << "       │            ";
            }
            std::cout << "\n";
            
            // Switch metrics
            std::cout << "  │ Pkts:" << std::setw(4) << switch_packets[i]->load() 
                      << " MAC:" << std::setw(3) << mac_tables[i]->load() << " │ ";
            if (i < switch_count - 1) {
                if (switch_activity[i] > 0 || switch_activity[i+1] > 0) {
                    std::cout << "  🌐 DATA FLOW 🌐   ";
                } else {
                    std::cout << "       │            ";
                }
            }
            std::cout << "\n";
            
            // Switch bottom
            std::cout << "  └─────────────────────┘ ";
            if (i < switch_count - 1) {
                std::cout << "       │            ";
            }
            std::cout << "\n";
            
            // Connection lines
            if (i < switch_count - 1) {
                std::cout << "                          │                            ";
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
    
    void show_data_flows() {
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                          DATA FLOWS                               ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
        
        // Show animated data flows
        const char* flow_patterns[] = {
            "📦 ➡️ 🌐 ➡️ 📦",
            "📦 ➡️ 💻 ➡️ 📦", 
            "📦 ➡️ 🔌 ➡️ 📦",
            "📦 ➡️ 📡 ➡️ 📦"
        };
        
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "  " << switch_names[i] << ": ";
            if (switch_activity[i] > 0) {
                std::cout << flow_patterns[animation_frame % 4];
            } else {
                std::cout << "⏸️  IDLE";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    
    void show_statistics(const VisualStats& stats) {
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                        STATISTICS                                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "  📊 Packets Processed:    " << std::setw(6) << stats.packets_processed.load() << "\n";
        std::cout << "  ❌ Packets Dropped:      " << std::setw(6) << stats.packets_dropped.load() << "\n";
        std::cout << "  ⚡ Processing Rate:      " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps\n";
        std::cout << "  📉 Packet Loss Rate:     " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "%\n";
        std::cout << "  🗂️  MAC Addresses:       " << std::setw(4) << stats.mac_learning_events.load() << "\n\n";
    }
    
    void show_progress(const VisualStats& stats, int elapsed_seconds, int total_seconds) {
        int progress_width = 50;
        int filled = (elapsed_seconds * progress_width) / total_seconds;
        
        std::cout << "PROGRESS: [";
        for (int i = 0; i < progress_width; ++i) {
            if (i < filled) {
                std::cout << "█";
            } else if (i == filled && elapsed_seconds < total_seconds) {
                const char* anim_chars[] = {"▌", "▀", "▐", "▄"};
                std::cout << anim_chars[animation_frame % 4];
            } else {
                std::cout << "░";
            }
        }
        std::cout << "] " << elapsed_seconds << "/" << total_seconds << "s\n\n";
    }
    
    void show_frame_separator() {
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    FRAME " << std::setw(4) << animation_frame << " END                     ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
    }
    
    void show_final_stats(const VisualStats& stats, 
                         const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                         const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& mac_tables) {
        clear_screen();
        show_header(stats);
        
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    VISUAL TUI ANIMATION COMPLETED                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "FINAL RESULTS:\n";
        std::cout << "  📊 Packets Processed:      " << std::setw(6) << stats.packets_processed.load() << "\n";
        std::cout << "  ❌ Packets Dropped:        " << std::setw(6) << stats.packets_dropped.load() << "\n";
        std::cout << "  ⚡ Average Throughput:     " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps\n";
        std::cout << "  📉 Packet Loss Rate:        " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "%\n";
        std::cout << "  🎬 Total Frames:           " << std::setw(6) << stats.frame_number.load() << "\n\n";
        
        std::cout << "SWITCH PERFORMANCE SUMMARY:\n";
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "  " << switch_names[i] << ": ";
            std::cout << std::setw(4) << switch_packets[i]->load() << " packets, ";
            std::cout << std::setw(3) << mac_tables[i]->load() << " MACs\n";
        }
        std::cout << "\n";
        
        std::cout << "🎉 Mini SONiC: Visual TUI Network Animation Complete!\n";
    }
    
    void update_animation() {
        animation_frame++;
    }
    
    void mark_switch_active(int switch_id) {
        if (switch_id >= 0 && switch_id < switch_count) {
            switch_activity[switch_id] = 4; // Longer activity for visibility
        }
    }
};

// Visual TUI Demo Controller
class VisualTUIDemo {
private:
    VisualConfig config;
    VisualStats stats;
    VisualTUI visualizer;
    
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> switch_packets;
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> mac_tables;
    std::unique_ptr<std::thread> demo_thread;
    std::atomic<bool> running{false};
    
public:
    VisualTUIDemo(const VisualConfig& cfg) 
        : config(cfg), visualizer(cfg.num_switches) {
        
        // Initialize atomic vectors with unique pointers
        for (int i = 0; i < cfg.num_switches; ++i) {
            switch_packets.push_back(std::make_unique<std::atomic<uint64_t>>(0));
            mac_tables.push_back(std::make_unique<std::atomic<uint64_t>>(0));
        }
        
        visualizer.show_header(stats);
        std::cout << "Starting VISUAL TUI animation with " << config.num_switches << " switches...\n";
        std::cout << "Rate: " << config.packets_per_second << " packets/second (visual)\n";
        std::cout << "Duration: " << config.demo_duration_seconds << " seconds\n";
        std::cout << "Refresh: " << config.refresh_interval_ms << "ms (visible animation)\n";
        std::cout << "*** VISUAL TUI MODE - Real Network Visualization ***\n\n";
    }
    
    void start() {
        if (running) return;
        
        running = true;
        stats.reset();
        
        // Start demo thread
        demo_thread = std::make_unique<std::thread>(&VisualTUIDemo::run_demo, this);
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        // Stop demo thread
        if (demo_thread && demo_thread->joinable()) {
            demo_thread->join();
        }
        
        // Show final statistics
        visualizer.show_final_stats(stats, switch_packets, mac_tables);
    }
    
private:
    void run_demo() {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(config.demo_duration_seconds);
        
        // Packet generation interval
        auto packet_interval = std::chrono::microseconds(1000000 / config.packets_per_second);
        auto last_packet_time = start_time;
        auto last_update_time = start_time;
        auto update_interval = std::chrono::milliseconds(config.refresh_interval_ms);
        
        while (running && std::chrono::high_resolution_clock::now() < end_time) {
            auto now = std::chrono::high_resolution_clock::now();
            
            // Generate packets
            if (now - last_packet_time >= packet_interval) {
                generate_and_process_packets();
                last_packet_time = now;
            }
            
            // Update display with visual animation
            if (now - last_update_time >= update_interval) {
                update_display(start_time, end_time);
                last_update_time = now;
                visualizer.update_animation();
                stats.frame_number++;
            }
            
            // Small sleep for smooth animation
            std::this_thread::sleep_for(200ms);
        }
        
        // Final update
        update_display(start_time, end_time);
    }
    
    void generate_and_process_packets() {
        // Generate packets for each switch
        for (int i = 0; i < config.num_switches; ++i) {
            // Generate 1 packet per interval per switch
            if (i % 5 != 0) {  // Simulate some drops
                stats.packets_processed++;
                *switch_packets[i] += 1;
                
                // Simulate MAC learning
                if (switch_packets[i]->load() % 15 == 0) {
                    *mac_tables[i] += 1;
                    stats.mac_learning_events++;
                }
                
                // Mark switch as active for animation
                visualizer.mark_switch_active(i);
            } else {
                stats.packets_dropped++;
            }
        }
    }
    
    void update_display(auto start_time, auto end_time) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        auto total = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        // Refresh display with visual animation
        visualizer.clear_screen();
        visualizer.show_header(stats);
        visualizer.show_progress(stats, static_cast<int>(elapsed), static_cast<int>(total));
        visualizer.show_network_topology(switch_packets, mac_tables);
        visualizer.show_data_flows();
        visualizer.show_statistics(stats);
        visualizer.show_frame_separator();
    }
};

// Main function
int main(int argc, char* argv[]) {
    VisualConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--switches" && i + 1 < argc) {
            config.num_switches = std::stoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            config.packets_per_second = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            config.demo_duration_seconds = std::stoi(argv[++i]);
        } else if (arg == "--refresh" && i + 1 < argc) {
            config.refresh_interval_ms = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Mini SONiC Visual TUI Network Animation Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --switches <n>     Number of switches (default: 3)\n";
            std::cout << "  --rate <n>         Packets per second (default: 2)\n";
            std::cout << "  --duration <n>     Demo duration in seconds (default: 30)\n";
            std::cout << "  --refresh <n>      Refresh interval in ms (default: 1500)\n";
            std::cout << "  --help             Show this help\n";
            std::cout << "\nFeatures:\n";
            std::cout << "  - Visual TUI (Text User Interface) mode\n";
            std::cout << "  - Real network topology visualization\n";
            std::cout << "  - Animated data flows and switches\n";
            std::cout << "  - Frame-by-frame display\n";
            std::cout << "  - No Mini SONiC dependencies\n";
            return 0;
        }
    }
    
    try {
        // Create and run visual TUI demo
        VisualTUIDemo demo(config);
        
#ifdef _WIN32
        // Handle Ctrl+C gracefully on Windows
        SetConsoleCtrlHandler([](DWORD ctrlType) {
            if (ctrlType == CTRL_C_EVENT) {
                return TRUE;
            }
            return FALSE;
        }, TRUE);
#else
        // Handle Ctrl+C gracefully on Unix-like systems
        std::signal(SIGINT, [](int) {
            // Silent exit
        });
#endif
        
        demo.start();
        
        // Wait for demo to complete
        std::this_thread::sleep_for(std::chrono::seconds(config.demo_duration_seconds + 1));
        
        demo.stop();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Visual TUI Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}
