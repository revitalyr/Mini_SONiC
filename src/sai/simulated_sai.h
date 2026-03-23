#pragma once

#include "sai/sai_interface.h"
#include <unordered_map>
#include <unordered_set>

class SimulatedSai : public SaiInterface {
public:
    void create_port(int port_id) override;
    void remove_port(int port_id) override;
    void add_fdb_entry(const std::string& mac, int port) override;
    void remove_fdb_entry(const std::string& mac) override;
    void add_route(const std::string& prefix,
                   const std::string& next_hop) override;
    void remove_route(const std::string& prefix) override;

private:
    std::unordered_set<int> ports_;
    std::unordered_map<std::string, int> fdb_;
    std::unordered_map<std::string, std::string> routes_;
};
