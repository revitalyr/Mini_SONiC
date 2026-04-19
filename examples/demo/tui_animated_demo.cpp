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
#include <io.h>
#include <fcntl.h>
#else
#include <csignal>
#include <unistd.h>
#endif

using namespace std::chrono_literals;

// TUI Animation Demo Configuration
struct TUIDemoConfig {
    int num_switches = 3;
    int packets_per_second = 1;  // Very slow for TUI visibility
    int demo_duration_seconds = 90;  // Long duration
    bool enable_animation = true;
    bool enable_colors = true;
    int refresh_interval_ms = 4000;  // Very slow refresh for TUI
    bool show_frame_numbers = true;
};

// TUI Statistics
struct TUIStats {
    std::atomic<uint64_t> packets_generated{0};
    std::atomic<uint64_t> packets_processed{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> mac_learning_events{0};
    std::atomic<uint64_t> frame_number{0};
    std::chrono::high_resolution_clock::time_point start_time;
    
    void reset() {
        packets_generated = 0;
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
        uint64_t total = packets_generated.load();
        if (total == 0) return 0.0;
        return (static_cast<double>(packets_dropped.load()) / total) * 100.0;
    }
};

// TUI Visualizer with frame-by-frame animation
class TUIVisualizer {
private:
    bool enabled;
    bool animation_enabled;
    bool color_enabled;
    bool show_frames;
    int switch_count;
    std::vector<std::string> switch_names;
    
    // Animation state
    int animation_frame = 0;
    std::vector<int> switch_activity;
    std::vector<int> packet_positions;
    std::vector<std::string> animation_chars;
    
    // Color codes
    enum Color {
        RESET = 7,
        RED = 12,
        GREEN = 10,
        YELLOW = 14,
        BLUE = 9,
        CYAN = 11,
        WHITE = 15,
        MAGENTA = 13,
        GRAY = 8
    };
    
public:
    TUIVisualizer(bool enable_anim, bool enable_colors, bool show_frame_nums, int switches) 
        : enabled(true), animation_enabled(enable_anim), color_enabled(enable_colors), 
          show_frames(show_frame_nums), switch_count(switches) {
        
        // Initialize switch names
        for (int i = 0; i < switches; ++i) {
            switch_names.push_back("SW" + std::to_string(i + 1));
        }
        
        // Initialize activity tracking
        switch_activity.resize(switches, 0);
        packet_positions.resize(switches, 0);
        
        // Animation characters for packets
        animation_chars = {"◄", "◆", "►", "●", "○", "▲", "▼"};
        
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
        
        SetConsoleTextAttribute(hConsole, WHITE);
#endif
    }
    
    void set_color(Color color) {
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
    
    void show_header(const TUIStats& stats) {
        if (!enabled) return;
        
        set_color(CYAN);
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           Mini SONiC - TUI ANIMATION DEMO                    ║\n";
        std::cout << "║              Text User Interface - Frame by Frame         ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        set_color(WHITE);
        
        if (show_frames) {
            set_color(YELLOW);
            std::cout << "FRAME: " << std::setw(4) << stats.frame_number.load() << " ";
            set_color(GRAY);
            std::cout << "| Time: " << std::setw(3) 
                      << std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::high_resolution_clock::now() - stats.start_time
                      ).count() << "s\n";
            set_color(WHITE);
        }
        
        std::cout << "\n";
    }
    
    void show_progress(const TUIStats& stats, int elapsed_seconds, int total_seconds) {
        if (!enabled) return;
        
        // Progress bar with TUI style
        int progress_width = 60;
        int filled = (elapsed_seconds * progress_width) / total_seconds;
        
        set_color(BLUE);
        std::cout << "╔═ PROGRESS ═";
        std::cout << "[";
        for (int i = 0; i < progress_width; ++i) {
            if (i < filled) {
                std::cout << "█";
            } else if (i == filled && elapsed_seconds < total_seconds) {
                // Animated progress indicator
                const char* anim_chars[] = {"▌", "▐", "▀", "▄", "■", "□"};
                std::cout << anim_chars[animation_frame % 6];
            } else {
                std::cout << " ";
            }
        }
        std::cout << "] ";
        std::cout << std::setw(2) << elapsed_seconds << "/" << std::setw(2) << total_seconds << "s";
        std::cout << " ═╗\n";
        set_color(WHITE);
    }
    
    void show_statistics(const TUIStats& stats) {
        if (!enabled) return;
        
        set_color(GREEN);
        std::cout << "╔═ STATISTICS ═╗\n";
        set_color(WHITE);
        std::cout << "║ Packets Processed: " << std::setw(6) << stats.packets_processed.load() << " ║\n";
        std::cout << "║ Packets Dropped:   " << std::setw(6) << stats.packets_dropped.load() << " ║\n";
        std::cout << "║ Processing Rate:   " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps ║\n";
        std::cout << "║ Packet Loss Rate: " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "% ║\n";
        std::cout << "║ MAC Addresses:    " << std::setw(4) << stats.mac_learning_events.load() << " ║\n";
        set_color(GREEN);
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        set_color(WHITE);
    }
    
    void show_tui_topology(const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                       const std::vector<std::unique_ptr<std::atomic<uint64_t>>& mac_tables) {
        if (!enabled) return;
        
        set_color(YELLOW);
        std::cout << "╔═ NETWORK TOPOLOGY - TUI ANIMATION ═╗\n";
        set_color(WHITE);
        
        // Visual network topology with TUI style
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "║ ";
            
            // Switch box with TUI style
            if (switch_activity[i] > 0) {
                set_color(GREEN);
                std::cout << "┌───┐ ";
                set_color(WHITE);
                std::cout << switch_names[i] << " ";
                set_color(GREEN);
                std::cout << "┌───┐";
                switch_activity[i]--;
            } else {
                set_color(GRAY);
                std::cout << "┌───┐ ";
                set_color(WHITE);
                std::cout << switch_names[i] << " ";
                set_color(GRAY);
                std::cout << "┌───┐";
            }
            
            set_color(WHITE);
            std::cout << " ║\n";
            
            // Stats line
            std::cout << "║ ";
            if (switch_activity[i] > 0) {
                set_color(GREEN);
            } else {
                set_color(GRAY);
            }
            std::cout << "│";
            set_color(CYAN);
            std::cout << std::setw(4) << switch_packets[i]->load();
            set_color(WHITE);
            std::cout << "│";
            set_color(YELLOW);
            std::cout << std::setw(3) << mac_tables[i]->load();
            set_color(WHITE);
            std::cout << "│";
            
            // Packet animation
            if (animation_enabled && packet_positions[i] > 0) {
                set_color(MAGENTA);
                std::cout << " " << animation_chars[packet_positions[i] % animation_chars.size()];
                set_color(WHITE);
                packet_positions[i]--;
            } else {
                std::cout << "   ";
            }
            
            std::cout << " ║\n";
            
            // Bottom of switch box
            std::cout << "║ ";
            if (switch_activity[i] > 0) {
                set_color(GREEN);
            } else {
                set_color(GRAY);
            }
            std::cout << "└───┘";
            std::cout << "     ";
            std::cout << "└───┘";
            set_color(WHITE);
            std::cout << " ║\n";
            
            // Separator
            if (i < switch_count - 1) {
                std::cout << "║ ";
                if (switch_activity[i] > 0 || switch_activity[i+1] > 0) {
                    set_color(GREEN);
                    const char* traffic_chars[] = {"═══", "───", "···", "~~~", "==="};
                    std::cout << "  " << traffic_chars[animation_frame % 5] << "  ";
                } else {
                    set_color(GRAY);
                    std::cout << "  ---  ";
                }
                set_color(WHITE);
                std::cout << " ║\n";
            }
        }
        
        set_color(YELLOW);
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        set_color(WHITE);
    }
    
    void show_connections() {
        if (!enabled) return;
        
        set_color(BLUE);
        std::cout << "╔═ CONNECTION STATUS ═╗\n";
        set_color(WHITE);
        std::cout << "║ ";
        
        for (int i = 0; i < switch_count; ++i) {
            std::cout << switch_names[i];
            
            if (i < switch_count - 1) {
                if (switch_activity[i] > 0 || switch_activity[i+1] > 0) {
                    set_color(GREEN);
                    const char* traffic_chars[] = {"≈≈≈", "~~", "===", "---"};
                    std::cout << " " << traffic_chars[animation_frame % 4] << " ";
                } else {
                    set_color(GRAY);
                    std::cout << " --- ";
                }
                set_color(WHITE);
            }
        }
        
        std::cout << " ║\n";
        set_color(BLUE);
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        set_color(WHITE);
    }
    
    void show_frame_separator() {
        if (!enabled || !show_frames) return;
        
        set_color(GRAY);
        std::cout << "\n";
        std::cout << "─────────────────────────────────────────────────────────────────\n";
        std::cout << "FRAME " << std::setw(4) << animation_frame << " END";
        std::cout << "─────────────────────────────────────────────────────────────────\n";
        std::cout << "\n";
        set_color(WHITE);
    }
    
    void show_final_stats(const TUIStats& stats, 
                        const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& switch_packets,
                        const std::vector<std::unique_ptr<std::atomic<uint64_t>>>& mac_tables) {
        clear_screen();
        show_header(stats);
        
        set_color(GREEN);
        std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           TUI ANIMATION COMPLETED SUCCESSFULLY!               ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";
        
        set_color(YELLOW);
        std::cout << "╔═ FINAL RESULTS ═╗\n";
        set_color(WHITE);
        std::cout << "║ Total Packets Generated: " << std::setw(6) << stats.packets_generated.load() << " ║\n";
        std::cout << "║ Packets Processed:      " << std::setw(6) << stats.packets_processed.load() << " ║\n";
        std::cout << "║ Packets Dropped:        " << std::setw(6) << stats.packets_dropped.load() << " ║\n";
        std::cout << "║ Average Throughput:     " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << stats.get_packets_per_second() << " pps ║\n";
        std::cout << "║ Packet Loss Rate:        " << std::fixed << std::setprecision(2) 
                  << std::setw(5) << stats.get_packet_loss_rate() << "% ║\n";
        std::cout << "║ Total Frames:           " << std::setw(6) << stats.frame_number.load() << " ║\n";
        set_color(YELLOW);
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
        
        set_color(CYAN);
        std::cout << "╔═ SWITCH PERFORMANCE SUMMARY ═╗\n";
        set_color(WHITE);
        for (int i = 0; i < switch_count; ++i) {
            std::cout << "║ " << switch_names[i] << ": ";
            std::cout << std::setw(4) << switch_packets[i]->load() << " packets, ";
            std::cout << std::setw(3) << mac_tables[i]->load() << " MACs ║\n";
        }
        set_color(CYAN);
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
        
        set_color(GREEN);
        std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           Mini SONiC: TUI Animation Complete!               ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
        set_color(WHITE);
    }
    
    void update_animation() {
        if (animation_enabled) {
            animation_frame++;
        }
    }
    
    void mark_switch_active(int switch_id) {
        if (switch_id >= 0 && switch_id < switch_count) {
            switch_activity[switch_id] = 5;
            packet_positions[switch_id] = 3;
        }
    }
    
    void increment_frame() {
        frame_number++;
    }
    
    void log_event(const std::string& event) {
        if (enabled && show_frames) {
            set_color(CYAN);
            std::cout << ">>> " << event << " <<<\n";
            set_color(WHITE);
        }
    }
};

// TUI Demo Controller
class TUIDemo {
private:
    TUIDemoConfig config;
    TUIStats stats;
    TUIVisualizer visualizer;
    
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> switch_packets;
    std::vector<std::unique_ptr<std::atomic<uint64_t>>> mac_tables;
    std::unique_ptr<std::thread> demo_thread;
    std::atomic<bool> running{false};
    
public:
    TUIDemo(const TUIDemoConfig& cfg) 
        : config(cfg), visualizer(cfg.enable_animation, cfg.enable_colors, cfg.show_frame_numbers, cfg.num_switches) {
        
        // Initialize atomic vectors with unique pointers
        for (int i = 0; i < cfg.num_switches; ++i) {
            switch_packets.push_back(std::make_unique<std::atomic<uint64_t>>(0));
            mac_tables.push_back(std::make_unique<std::atomic<uint64_t>>(0));
        }
        
        visualizer.show_header(stats);
        std::cout << "Starting TUI animation with " << config.num_switches << " switches...\n";
        std::cout << "Rate: " << config.packets_per_second << " packets/second (VERY slow for TUI)\n";
        std::cout << "Duration: " << config.demo_duration_seconds << " seconds\n";
        std::cout << "Refresh: " << config.refresh_interval_ms << "ms (TUI frame-by-frame)\n";
        std::cout << "Frames: " << (config.show_frame_numbers ? "ENABLED" : "DISABLED") << "\n";
        std::cout << "*** TUI MODE - Text User Interface Animation ***\n\n";
        
        visualizer.log_event("TUI DEMO STARTED");
    }
    
    void start() {
        if (running) return;
        
        running = true;
        stats.reset();
        
        // Start demo thread
        demo_thread = std::make_unique<std::thread>(&TUIDemo::run_demo, this);
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
        visualizer.log_event("TUI DEMO COMPLETED");
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
            
            // Update display with TUI frame-by-frame animation
            if (now - last_update_time >= update_interval) {
                update_display(start_time, end_time);
                last_update_time = now;
                visualizer.update_animation();
                visualizer.increment_frame();
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
            stats.packets_generated++;
            
            // Simulate processing
            if (i % 4 != 0) {  // Simulate some drops
                stats.packets_processed++;
                *switch_packets[i] += 1;
                
                // Simulate MAC learning
                if (switch_packets[i]->load() % 30 == 0) {
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
        
        // Refresh display with TUI frame-by-frame animation
        visualizer.clear_screen();
        visualizer.show_header(stats);
        visualizer.show_progress(stats, static_cast<int>(elapsed), static_cast<int>(total));
        visualizer.show_statistics(stats);
        visualizer.show_tui_topology(switch_packets, mac_tables);
        visualizer.show_connections();
        visualizer.show_frame_separator();
    }
};

// Main function
int main(int argc, char* argv[]) {
    TUIDemoConfig config;
    
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
        } else if (arg == "--no-frames") {
            config.show_frame_numbers = false;
        } else if (arg == "--no-anim") {
            config.enable_animation = false;
        } else if (arg == "--no-color") {
            config.enable_colors = false;
        } else if (arg == "--help") {
            std::cout << "Mini SONiC TUI Animation Demo\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --switches <n>     Number of switches (default: 3)\n";
            std::cout << "  --rate <n>         Packets per second (default: 1)\n";
            std::cout << "  --duration <n>     Demo duration in seconds (default: 90)\n";
            std::cout << "  --refresh <n>      Refresh interval in ms (default: 4000)\n";
            std::cout << "  --no-frames        Hide frame numbers\n";
            std::cout << "  --no-anim          Disable animation\n";
            std::cout << "  --no-color         Disable colors\n";
            std::cout << "  --help             Show this help\n";
            std::cout << "\nFeatures:\n";
            std::cout << "  - TUI (Text User Interface) mode\n";
            std::cout << "  - Frame-by-frame animation\n";
            std::cout << "  - Very slow refresh for visibility\n";
            std::cout << "  - Box-style TUI graphics\n";
            return 0;
        }
    }
    
    try {
        // Create and run TUI demo
        TUIDemo demo(config);
        
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
        std::cerr << "TUI Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}
