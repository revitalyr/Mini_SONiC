#include "visualizer_core.h"

VisualizerCore::VisualizerCore(QObject *parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &VisualizerCore::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &VisualizerCore::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &VisualizerCore::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &VisualizerCore::onError);
}

void VisualizerCore::connectToHost(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port);
}

void VisualizerCore::disconnectFromHost() {
    m_socket->disconnectFromHost();
}

void VisualizerCore::onConnected() {
    emit connectionStatusChanged(true, "Connected");
    requestTopology();
}

void VisualizerCore::onDisconnected() {
    emit connectionStatusChanged(false, "Disconnected");
}

void VisualizerCore::onError(QAbstractSocket::SocketError) {
    emit connectionStatusChanged(false, "Error: " + m_socket->errorString());
}

void VisualizerCore::onReadyRead() {
    int processed = 0;
    while (m_socket->canReadLine() && processed < 50) {
        processEvent(m_socket->readLine());
        processed++;
    }
    if (m_socket->canReadLine()) {
        QTimer::singleShot(0, this, &VisualizerCore::onReadyRead);
    }
}

void VisualizerCore::processEvent(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;
    
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    emit logMessage(type, QString::fromUtf8(data).trimmed());

    if (type == "topology") handleTopology(obj);
    else if (type == "PacketGenerated") handlePacketGenerated(obj);
    else if (type == "PacketEnteredSwitch") handlePacketEnteredSwitch(obj);
    else if (type == "PacketExitedSwitch") handlePacketExitedSwitch(obj);
    else if (type == "PortStateChanged") {
        emit portStateChanged(obj["switch_id"].toString(), obj["port"].toString(), obj["state"].toString());
    }
}

void VisualizerCore::handleTopology(const QJsonObject& obj) {
    m_hostIpMap.clear();
    QJsonArray nodes = obj["nodes"].toArray();
    for (const auto& n : nodes) {
        QJsonObject node = n.toObject();
        if (node["type"].toString() == "host") {
            m_hostIpMap[node["ip"].toString()] = node["id"].toString();
        }
    }
    emit topologyLoaded(obj);
}

void VisualizerCore::handlePacketGenerated(const QJsonObject& obj) {
    QJsonObject pkt = obj["packet"].toObject();
    QString id = QString::number(pkt["id"].toVariant().toLongLong());
    m_packetInfo[id] = {pkt["src_ip"].toString(), pkt["dst_ip"].toString()};
    m_packetLastNode.remove(id); // Reset state for new packet ID reuse
}

void VisualizerCore::handlePacketEnteredSwitch(const QJsonObject& obj) {
    QString pId = QString::number(obj["packet_id"].toVariant().toLongLong());
    QString swId = obj["switch_id"].toString();

    // If we don't know where the packet was before, it came from the Host
    if (!m_packetLastNode.contains(pId) && m_packetInfo.contains(pId)) {
        QString srcIp = m_packetInfo[pId].first;
        if (m_hostIpMap.contains(srcIp)) {
            QString hostId = m_hostIpMap[srcIp];
            emit requestPacketAnimation(pId, hostId, swId, obj);
        }
    }
    m_packetLastNode[pId] = swId;
    emit nodeActivated(swId, true);
}

void VisualizerCore::handlePacketExitedSwitch(const QJsonObject& obj) {
    QString pId = QString::number(obj["packet_id"].toVariant().toLongLong());
    QString swId = obj["switch_id"].toString();
    QString nextHop = obj["next_hop"].toString();

    emit requestPacketAnimation(pId, swId, nextHop, obj);
    emit linkActivated(swId, nextHop);
    m_packetLastNode[pId] = nextHop;
}

void VisualizerCore::sendSpeedControl(int speed) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject msg;
        msg["type"] = "speed_control";
        msg["speed"] = speed;
        m_socket->write(QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n");
    }
}

void VisualizerCore::requestTopology() {
    QJsonObject req;
    req["type"] = "topology_query";
    m_socket->write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");
}