#include "sai/simulated_sai.h"
#include <iostream>

void SimulatedSai::create_port(int port_id) {
    ports_.insert(port_id);
    std::cout << "[SAI] Create port " << port_id << "\n";
}

void SimulatedSai::remove_port(int port_id) {
    ports_.erase(port_id);
    std::cout << "[SAI] Remove port " << port_id << "\n";
}

void SimulatedSai::add_fdb_entry(const std::string& mac, int port) {
    fdb_[mac] = port;
    std::cout << "[SAI] FDB " << mac << " -> " << port << "\n";
}

void SimulatedSai::remove_fdb_entry(const std::string& mac) {
    fdb_.erase(mac);
    std::cout << "[SAI] Remove FDB entry " << mac << "\n";
}

void SimulatedSai::add_route(const std::string& prefix,
                            const std::string& next_hop) {
    routes_[prefix] = next_hop;
    std::cout << "[SAI] Route " << prefix << " -> " << next_hop << "\n";
}

void SimulatedSai::remove_route(const std::string& prefix) {
    routes_.erase(prefix);
    std::cout << "[SAI] Remove route " << prefix << "\n";
}
