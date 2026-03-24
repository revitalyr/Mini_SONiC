#include "core/app.h"
#include "common/types.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        Types::Port listen_port = 9000;
        Types::String peer_ip;
        Types::Port peer_port = 0;
        
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);
            
            if (arg == "--help" || arg == "-h") {
                std::cout << "Mini SONiC - High-Performance Network OS\n"
                             << "Usage: " << argv[0] << " [options]\n"
                             << "Options:\n"
                             << "  --port <port>      Listen port (default: 9000)\n"
                             << "  --peer <ip:port>  Peer switch address\n"
                             << "  --help, -h        Show this help\n"
                             << "\nExamples:\n"
                             << "  " << argv[0] << " --port 9000\n"
                             << "  " << argv[0] << " --port 9000 --peer 192.168.1.2:9000\n";
                return 0;
            }
            
            if (arg == "--port" && i + 1 < argc) {
                listen_port = static_cast<Types::Port>(std::stoi(argv[++i]));
            }
            
            if (arg == "--peer" && i + 1 < argc) {
                const std::string peer_addr(argv[++i]);
                const auto colon_pos = peer_addr.find(':');
                if (colon_pos != std::string::npos) {
                    peer_ip = peer_addr.substr(0, colon_pos);
                    peer_port = static_cast<Types::Port>(
                        std::stoi(peer_addr.substr(colon_pos + 1))
                    );
                }
            }
        }
        
        std::cout << "[MAIN] Starting Mini SONiC with async architecture\n"
                     << "  Listen Port: " << listen_port << "\n"
                     << "  Peer: " << peer_ip << ":" << peer_port << "\n";
        
        // Create and run application
        MiniSonic::Core::App app(listen_port, peer_ip, peer_port);
        app.run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }
}
