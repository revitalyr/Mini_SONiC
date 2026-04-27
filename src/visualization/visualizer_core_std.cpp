#include "visualizer_core_std.h"
#include <iostream>

VisualizerCoreStd::VisualizerCoreStd(boost::asio::io_context& io_context)
    : m_socket(io_context) {}

void VisualizerCoreStd::connect(const std::string& host, PortNo port) {
    tcp::resolver resolver(m_socket.get_executor());
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::async_connect(m_socket, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
            if (!ec) {
                if (onConnectionStatus) onConnectionStatus(true, "Connected");
                requestTopology();
                do_read();
            } else {
                if (onConnectionStatus) onConnectionStatus(false, "Connection failed: " + ec.message());
            }
        });
}

void VisualizerCoreStd::disconnect() {
    boost::system::error_code ec;
    m_socket.close(ec);
}

void VisualizerCoreStd::do_read() {
    boost::asio::async_read_until(m_socket, m_buffer, "\n",
        [this](boost::system::error_code ec, std::size_t bytes) {
            if (!ec) {
                std::istream is(&m_buffer);
                std::string line;
                std::getline(is, line);
                handle_line(line);
                do_read();
            } else if (onConnectionStatus) {
                onConnectionStatus(false, "Disconnected: " + ec.message());
            }
        });
}

void VisualizerCoreStd::handle_line(const std::string& line) {
    try {
        auto obj = json::parse(line);
        std::string type = obj["type"];
        
        if (onLogMessage) onLogMessage(type, line);

        if (type == "topology") handleTopology(obj);
        else if (type == "PacketGenerated") handlePacketGenerated(obj);
        else if (type == "PacketEnteredSwitch") handlePacketEnteredSwitch(obj);
        else if (type == "PacketExitedSwitch") handlePacketExitedSwitch(obj);
        else if (type == "PortStateChanged") {
            if (portStateChanged) portStateChanged(obj["switch_id"], obj["port"], obj["state"]);
        }
    } catch (...) {}
}

void VisualizerCoreStd::handleTopology(const json& obj) {
    m_hostIpMap.clear();
    for (const auto& node : obj["nodes"]) {
        if (node["type"] == "host") {
            m_hostIpMap[node["ip"]] = node["id"];
        }
    }
    if (onTopologyLoaded) onTopologyLoaded(obj);
}

void VisualizerCoreStd::handlePacketGenerated(const json& obj) {
    auto pkt = obj["packet"];
    std::string id = get_packet_id(pkt, "id");
    m_packetInfo[id] = {pkt["src_ip"], pkt["dst_ip"]};
    m_packetLastNode.erase(id);
}

void VisualizerCoreStd::handlePacketEnteredSwitch(const json& obj) {
    std::string pId = get_packet_id(obj, "packet_id");
    std::string swId = obj["switch_id"];

    if (m_packetLastNode.find(pId) == m_packetLastNode.end() && m_packetInfo.count(pId)) {
        std::string srcIp = m_packetInfo[pId].first;
        if (m_hostIpMap.count(srcIp)) {
            if (requestPacketAnimation) requestPacketAnimation(pId, m_hostIpMap[srcIp], swId, obj);
        }
    }
    m_packetLastNode[pId] = swId;
    if (nodeActivated) nodeActivated(swId, true);
}

void VisualizerCoreStd::handlePacketExitedSwitch(const json& obj) {
    std::string pId = get_packet_id(obj, "packet_id");
    std::string swId = obj["switch_id"];
    std::string nextHop = obj["next_hop"];
    if (requestPacketAnimation) requestPacketAnimation(pId, swId, nextHop, obj);
    if (linkActivated) linkActivated(swId, nextHop);
    m_packetLastNode[pId] = nextHop;
}

void VisualizerCoreStd::write(const json& obj) {
    std::string data = obj.dump() + "\n";
    boost::asio::write(m_socket, boost::asio::buffer(data));
}

void VisualizerCoreStd::requestTopology() {
    write({{"type", "topology_query"}});
}

// Internal type for numeric packet IDs
using PacketIdInt = std::int64_t;

std::string VisualizerCoreStd::get_packet_id(const json& obj, const std::string& key) {
    if (!obj.contains(key)) return "";
    if (obj[key].is_number()) return std::to_string(obj[key].get<PacketIdInt>());
    return obj[key].get<std::string>();
}

void VisualizerCoreStd::sendSpeedControl(SpeedMultiplier speed) {
    write({{"type", "speed_control"}, {"speed", speed}});
}