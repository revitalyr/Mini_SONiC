#pragma once

#include "common/types.hpp"
#include <chrono>

namespace MiniSonic::DataPlane {

struct Packet {
    Types::MacAddress m_src_mac;
    Types::MacAddress m_dst_mac;
    Types::IpAddress m_src_ip;
    Types::IpAddress m_dst_ip;
    Types::Port m_ingress_port;
    
    // Timestamp for latency measurement
    std::chrono::high_resolution_clock::time_point timestamp;

    // Default constructor
    Packet() = default;

    // Parameterized constructor
    Packet(
        Types::MacAddress src_mac,
        Types::MacAddress dst_mac,
        Types::IpAddress src_ip,
        Types::IpAddress dst_ip,
        Types::Port ingress_port
    ) : m_src_mac(std::move(src_mac)),
        m_dst_mac(std::move(dst_mac)),
        m_src_ip(std::move(src_ip)),
        m_dst_ip(std::move(dst_ip)),
        m_ingress_port(ingress_port),
        timestamp(std::chrono::high_resolution_clock::now()) {}

    // Move constructor
    Packet(Packet&& other) noexcept = default;

    // Move assignment operator
    Packet& operator=(Packet&& other) noexcept = default;

    // Copy constructor
    Packet(const Packet& other) = default;

    // Copy assignment operator
    Packet& operator=(const Packet& other) = default;

    // Destructor
    ~Packet() = default;

    // Getters with const correctness
    [[nodiscard]] const Types::MacAddress& srcMac() const noexcept { return m_src_mac; }
    [[nodiscard]] const Types::MacAddress& dstMac() const noexcept { return m_dst_mac; }
    [[nodiscard]] const Types::IpAddress& srcIp() const noexcept { return m_src_ip; }
    [[nodiscard]] const Types::IpAddress& dstIp() const noexcept { return m_dst_ip; }
    [[nodiscard]] Types::Port ingressPort() const noexcept { return m_ingress_port; }

    // Setters
    void setSrcMac(Types::MacAddress mac) { m_src_mac = std::move(mac); }
    void setDstMac(Types::MacAddress mac) { m_dst_mac = std::move(mac); }
    void setSrcIp(Types::IpAddress ip) { m_src_ip = std::move(ip); }
    void setDstIp(Types::IpAddress ip) { m_dst_ip = std::move(ip); }
    void setIngressPort(Types::Port port) { m_ingress_port = port; }
    
    // Update timestamp
    void updateTimestamp() noexcept {
        timestamp = std::chrono::high_resolution_clock::now();
    }
};

} // namespace MiniSonic::DataPlane
