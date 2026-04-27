#include <iostream>
#include <thread>
#include <chrono>
#include "event_server.h"

int main(int argc, char* argv[]) {
    using namespace MiniSonic::Visualization;

    uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::cout << "Mini_SONiC Event Server\n";
    std::cout << "========================\n";
    std::cout << "Starting WebSocket event server on port " << port << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    startEventServer(port);

    // Keep running until interrupted
    std::cout << "[EventServer] Running. Waiting for events...\n";
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    stopEventServer();

    return 0;
}
