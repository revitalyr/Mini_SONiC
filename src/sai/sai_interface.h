#pragma once

#include <string>

class SaiInterface {
public:
    virtual ~SaiInterface() = default;

    virtual void create_port(int port_id) = 0;
    virtual void remove_port(int port_id) = 0;

    virtual void add_fdb_entry(const std::string& mac, int port) = 0;
    virtual void remove_fdb_entry(const std::string& mac) = 0;

    virtual void add_route(const std::string& prefix,
                           const std::string& next_hop) = 0;
    virtual void remove_route(const std::string& prefix) = 0;
};
