#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <csignal>
#include <unistd.h>
#endif

using namespace std::chrono_literals;

// Simple TUI Demo Configuration
struct SimpleTUIConfig {
    int num_switches = 3;
    int packets_per_second = 1;
    int demo_duration_seconds = 60;
    bool enable_colors = true;
    int refresh_interval_ms = 3000;
};

// Simple TUI Statistics
struct SimpleTUIStats {
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

// Simple TUI Visualizer
class SimpleTUIVisualizer {
private:
    bool enabled;
    bool color_enabled;
    int switch_count;
    std::vector<std::string> switch_names;
    
    // Animation state
    int animation_frame = 0;
    std::vector<int> switch_activity;
    std::vector<int> packet_positions;
    
public:
    SimpleTUIVisualizer(bool enable_colors, int switches) 
        : enabled(true), color_enabled(enable_colors), switch_count(switches) {
        
        // Initialize switch names
        for (int i = 0; i < switches; ++i) {
            switch_names.push_back("SW" + std::to_string(i + 1));
        }
        
        // Initialize activity tracking
        switch_activity.resize(switches, 0);
        packet_positions.resize(switches, 0);
        
        // Setup console colors on Windows
        if (color_enabled) {
            setup_console();
        }
    }
    
    void setup_console() {
#ifdef _WIN32
        // Disable cursor for smoother animation
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
        
        SetConsoleTextAttribute(hConsole, 7); // WHITE
#endif
    }
    
    void set_color(int color) {
        if (!color_enabled) return;
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
#endif
    }
    
    void clear_screen() {
        if (!enabled) return;
        
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
    
    void show_header(const SimpleTUIStats& stats) {
        if (!enabled) return;
        
        set_color(11); // CYAN
        std::cout << "\n";
        std::cout << "================================================================\n";
        std::cout << "           Mini SONiC - SIMPLE TUI ANIMATION           \n";
        std::cout << "              Text User Interface Demo              \n";
        std::cout << "================================================================\n";
        set_color(7); // WHITE
        
        std::cout << "FRAME: " << std::setw(4) << stats.frame_number.load() << " ";
        std::cout << "| Time: " << std::setw(3) 
                  << std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::high_resolution_clock::now() - stats.start_time
                  ).count() << "s\n\n";
    }
    
    void show_progress(const SimpleTUIStats& stats, int elapsed_seconds, int total_seconds) {
        if (!enabled) return;
        
        // Progress bar
        int progress_width = 50;
        int filled = (elapsed_seconds * progress_width) / total_seconds;
        
        set_color(9); // BLUE
        std::cout << "PROGRESS: [";
        for (int i = 0; i < progress_width; ++i) {
            if (i < filled) {
                std::cout << "=";
            } else if (i == filled && elapsed_seconds < total_seconds) {
                const char* anim_chars[] = {"|", "/", "-", "\\"};
                std::cout << anim_chars[animation_frame % 4];
            } else {
                std::cout << " ";
            }
        }
        std::cout << "] " << elapsed_seconds << "/" << total_seconds << "s\n\n";
        set_color(7); // WHITE
    }
    
    void show_statistics(const SimpleTUIStats& stats) {
        if (!enabled) return;
        
        set_color(10); // GREEN
        std::cout << "STATISTICS:\n";
        set_color(7); // WHITE
        std::cout << "  Packets Processed: " << std::setw(6) << stats.packets_processed.load() << "\n";
        std::cout << "  Packets Dropped:   " << std::setw(6) << stats.packets_dropped.load() << "\n";
        std::cout << "  Processing Rate:   " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps\n";
        std::cout << "  Packet Loss Rate: " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "%\n";
        std::cout << "  MAC Addresses:    " << std::setw(4) << stats.mac_learning_events.load() << "\n\n";
    }
    
    void show_tui_topology(const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                       const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& mac_tables) {
        if (!enabled) return;
        
        set_color(14); // YELLOW
        std::cout << "NETWORK TOPOLOGY - TUI ANIMATION:\n\n";
        set_color(7); // WHITE
        
        // Visual network topology
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "  ";
            
            // Switch box
            if (switch_activity[i] > 0) {
                set_color(10); // GREEN
                std::cout << "[ACTIVE] ";
                switch_activity[i]--;
            } else {
                set_color(8); // GRAY
                std::cout << "[IDLE]   ";
            }
            
            set_color(11); // CYAN
            std::cout << switch_names[i];
            set_color(7); // WHITE
            std::cout << " [";
            set_color(10); // GREEN
            std::cout << std::setw(4) << switch_packets[i]->load();
            set_color(7); // WHITE
            std::cout << " pkts, ";
            set_color(14); // YELLOW
            std::cout << std::setw(3) << mac_tables[i]->load();
            set_color(7); // WHITE
            std::cout << " MACs";
            
            // Packet animation
            if (packet_positions[i] > 0) {
                const char* packet_chars[] = {"<", "*", ">"};
                set_color(13); // MAGENTA
                std::cout << " " << packet_chars[packet_positions[i] % 3];
                set_color(7); // WHITE
                packet_positions[i]--;
            }
            
            std::cout << "\n";
        }
        std::cout << "\n";
        
        // Connection lines
        set_color(9); // BLUE
        std::cout << "CONNECTIONS: ";
        for (int i = 0; i < switch_count; ++i) {
            std::cout << switch_names[i];
            
            if (i < switch_count - 1) {
                if (switch_activity[i] > 0 || switch_activity[i+1] > 0) {
                    const char* traffic_chars[] = {"===", "---", "..."};
                    std::cout << " " << traffic_chars[animation_frame % 3] << " ";
                } else {
                    std::cout << " --- ";
                }
            }
        }
        std::cout << "\n\n";
        set_color(7); // WHITE
    }
    
    void show_frame_separator() {
        if (!enabled) return;
        
        set_color(8); // GRAY
        std::cout << "------------------------------------------------------------\n";
        std::cout << "FRAME " << std::setw(4) << animation_frame << " END\n";
        std::cout << "------------------------------------------------------------\n\n";
        set_color(7); // WHITE
    }
    
    void show_final_stats(const SimpleTUIStats& stats, 
                        const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                        const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& mac_tables) {
        clear_screen();
        show_header(stats);
        
        set_color(10); // GREEN
        std::cout << "TUI ANIMATION COMPLETED SUCCESSFULLY!\n\n";
        
        set_color(14); // YELLOW
        std::cout << "FINAL RESULTS:\n";
        set_color(7); // WHITE
        std::cout << "  Packets Processed:      " << std::setw(6) << stats.packets_processed.load() << "\n";
        std::cout << "  Packets Dropped:        " << std::setw(6) << stats.packets_dropped.load() << "\n";
        std::cout << "  Average Throughput:     " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps\n";
        std::cout << "  Packet Loss Rate:        " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "%\n";
        std::cout << "  Total Frames:           " << std::setw(6) << stats.frame_number.load() << "\n\n";
        
        set_color(11); // CYAN
        std::cout << "SWITCH PERFORMANCE SUMMARY:\n";
        set_color(7); // WHITE
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "  " << switch_names[i] << ": ";
            std::cout << std::setw(4) << switch_packets[i]->load() << " packets, ";
            std::cout << std::setw(3) << mac_tables[i]->load() << " MACs\n";
        }
        std::cout << "\n";
        
        set_color(10); // GREEN
        std::cout << "Mini SONiC: TUI Animation Complete!\n";
        set_color(7); // WHITE
    }
    
    void update_animation() {
        animation_frame++;
    }
    
    void mark_switch_active(int switch_id) {
        if (switch_id >= 0 && switch_id < switch_count) {
            switch_activity[switch_id] = 3;
            packet_positions[switch_id] = 2;
        }
    }
};

// Simple TUI Demo Controller
class SimpleTUIDemo {
private:
    SimpleTUIConfig config;
    SimpleTUIStats stats;
    SimpleTUIVisualizer visualizer;
    
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> switch_packets;
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> mac_tables;
    std::unique_ptr<std::thread> demo_thread;
    std::atomic<bool> running{false};
    
public:
    SimpleTUIDemo(const SimpleTUIConfig& cfg) 
        : config(cfg), visualizer(cfg.enable_colors, cfg.num_switches) {
        
        // Initialize atomic vectors with unique pointers
        for (int i = 0; i < cfg.num_switches; ++i) {
            switch_packets.push_back(std::make_unique<std::atomic<uint64_t>>(0));
            mac_tables.push_back(std::make_unique<std::atomic<uint64_t>>(0));
        }
        
        visualizer.show_header(stats);
        std::cout << "Starting SIMPLE TUI animation with " << config.num_switches << " switches...\n";
        std::cout << "Rate: " << config.packets_per_second << " packets/second (slow for TUI)\n";
        std::cout << "Duration: " << config.demo_duration_seconds << " seconds\n";
        std::cout << "Refresh: " << config.refresh_interval_ms << "ms (TUI visible)\n";
        std::cout << "*** SIMPLE TUI MODE - Easy to see animation ***\n\n";
    }
    
    void start() {
        if (running) return;
        
        running = true;
        stats.reset();
        
        // Start demo thread
        demo_thread = std::make_unique<std::thread>(&SimpleTUIDemo::run_demo, this);
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
            
            // Update display with TUI animation
            if (now - last_update_time >= update_interval) {
                update_display(start_time, end_time);
                last_update_time = now;
                visualizer.update_animation();
                stats.frame_number++;
            }
            
            // Small sleep for smooth animation
            std::this_thread::sleep_for(500ms);
        }
        
        // Final update
        update_display(start_time, end_time);
    }
    
    void generate_and_process_packets() {
        // Generate packets for each switch
        for (int i = 0; i < config.num_switches; ++i) {
            // Generate 1 packet per interval per switch
            if (i % 4 != 0) {  // Simulate some drops
                stats.packets_processed++;
                *switch_packets[i] += 1;
                
                // Simulate MAC learning
                if (switch_packets[i]->load() % 25 == 0) {
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
        
        // Refresh display with TUI animation
        visualizer.clear_screen();
        visualizer.show_header(stats);
        visualizer.show_progress(stats, static_cast<int>(elapsed), static_cast<int>(total));
        visualizer.show_statistics(stats);
        visualizer.show_tui_topology(switch_packets, mac_tables);
        visualizer.show_frame_separator();
    }
};

// Main function
int main(int argc, char* argv[]) {
    SimpleTUIConfig config;
    
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
        } else if (arg == "--no-color") {
            config.enable_colors = false;
        } else if (arg == "--help") {
            std::cout << "Mini SONiC Simple TUI Animation Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --switches <n>     Number of switches (default: 3)\n";
            std::cout << "  --rate <n>         Packets per second (default: 1)\n";
            std::cout << "  --duration <n>     Demo duration in seconds (default: 60)\n";
            std::cout << "  --refresh <n>      Refresh interval in ms (default: 3000)\n";
            std::cout << "  --no-color         Disable colors\n";
            std::cout << "  --help             Show this help\n";
            std::cout << "\nFeatures:\n";
            std::cout << "  - Simple TUI (Text User Interface) mode\n";
            std::cout << "  - Very slow refresh for visibility\n";
            std::cout << "  - Easy to see frame-by-frame animation\n";
            std::cout << "  - No complex graphics, just text\n";
            return 0;
        }
    }
    
    try {
        // Create and run simple TUI demo
        SimpleTUIDemo demo(config);
        
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
        std::cerr << "Simple TUI Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}
