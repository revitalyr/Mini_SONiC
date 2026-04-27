#ifndef VISUALIZER_CORE_H
#define VISUALIZER_CORE_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QTimer>

class VisualizerCore : public QObject {
    Q_OBJECT
public:
    explicit VisualizerCore(QObject *parent = nullptr);
    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    void sendSpeedControl(int speed);
    void requestTopology();

signals:
    void connectionStatusChanged(bool connected, const QString& status);
    void logMessage(const QString& type, const QString& details);
    void topologyLoaded(const QJsonObject& topology);
    
    // High-level events for animation
    void requestPacketAnimation(const QString& packetId, const QString& fromNodeId, const QString& toNodeId, const QJsonObject& details);
    void nodeActivated(const QString& nodeId, bool isSwitch);
    void linkActivated(const QString& sourceId, const QString& targetId);
    void portStateChanged(const QString& switchId, const QString& port, const QString& state);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    void processEvent(const QByteArray& data);
    void handleTopology(const QJsonObject& obj);
    void handlePacketGenerated(const QJsonObject& obj);
    void handlePacketEnteredSwitch(const QJsonObject& obj);
    void handlePacketExitedSwitch(const QJsonObject& obj);

    QTcpSocket* m_socket;
    QMap<QString, QString> m_hostIpMap;        // IP -> HostID
    QMap<QString, QString> m_packetLastNode;   // PacketID -> Last Node ID (Switch or Host)
    QMap<QString, QPair<QString, QString>> m_packetInfo; // PacketID -> {srcIp, dstIp}
};

#endif // VISUALIZER_CORE_H