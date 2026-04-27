#include <QApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsItemGroup>
#include <QTimer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QMap>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPainter>
#include <QTime>
#include <QRandomGenerator>
#include <cmath>
#include <limits>
#include <thread>
#include "visualizer_core_std.h"

// Packet card item that displays packet information like web visualizer
class PacketCardItem : public QGraphicsItem {
public:
    PacketCardItem(const QString& id, const QString& srcIp, const QString& dstIp,
                   const QString& srcPort, const QString& dstPort,
                   const QString& dscp, const QString& ttl,
                   const QString& state = "normal")
        : m_id(id), m_srcIp(srcIp), m_dstIp(dstIp),
          m_srcPort(srcPort), m_dstPort(dstPort),
          m_dscp(dscp), m_ttl(ttl), m_state(state),
          m_tiltAngle(0), m_vibrationX(0), m_vibrationY(0), m_opacity(1.0) {
        setZValue(100); // Above switches
    }

    void setTilt(double angle) { m_tiltAngle = angle; update(); }
    void setVibration(double vx, double vy) { m_vibrationX = vx; m_vibrationY = vy; update(); }
    void setOpacity(double op) { m_opacity = op; update(); }

    QRectF boundingRect() const override {
        return QRectF(-60, -30, 120, 60);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->setOpacity(m_opacity);

        // Apply tilt and vibration
        painter->translate(m_vibrationX, m_vibrationY);
        painter->rotate(m_tiltAngle * 180.0 / M_PI);

        // Background color based on state
        QColor bgColor;
        if (m_state == "drop") bgColor = QColor(244, 67, 54);
        else if (m_state == "queued") bgColor = QColor(255, 152, 0);
        else if (m_state == "remarked") bgColor = QColor(156, 39, 176);
        else if (m_state == "ecmp") bgColor = QColor(33, 150, 243);
        else bgColor = QColor(76, 175, 80);

        // Rounded rectangle background
        painter->setBrush(bgColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(-60, -30, 120, 60, 8, 8);

        // Draw packet info
        painter->setPen(Qt::white);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(-52, -15, QString("ID: %1").arg(m_id));

        painter->setFont(QFont("Segoe UI", 8));
        painter->drawText(-52, -2, QString("%1 → %2").arg(m_srcIp, m_dstIp));
        painter->drawText(-52, 10, QString("%1 → %2").arg(m_srcPort, m_dstPort));
        painter->drawText(-52, 22, QString("DSCP: %1  TTL: %2").arg(m_dscp, m_ttl));

        // Status icon
        painter->setFont(QFont("Segoe UI", 12));
        if (m_state == "queued") {
            painter->drawText(35, -15, "⏳");
        } else if (m_state == "drop") {
            painter->drawText(35, -15, "❌");
        } else if (m_state == "ecmp") {
            painter->drawText(35, -15, "🔀");
        }
    }

private:
    QString m_id, m_srcIp, m_dstIp, m_srcPort, m_dstPort, m_dscp, m_ttl, m_state;
    double m_tiltAngle, m_vibrationX, m_vibrationY, m_opacity;
};

// Switch item with detailed info like web visualizer
class SwitchItem : public QGraphicsItemGroup {
public:
    SwitchItem(const QString& name, const QString& role,
               const QJsonArray& ports = QJsonArray(),
               const QString& routing = "",
               const QString& acl = "",
               const QString& qos = "",
               const QJsonObject& stats = QJsonObject())
        : m_name(name), m_role(role), m_routing(routing), m_acl(acl), m_qos(qos),
          m_active(false) {

        // Default ports if not provided
        if (ports.isEmpty()) {
            m_ports.append({"Eth0", "100G", "UP", "", 40});
            m_ports.append({"Eth4", "100G", "UP", "LAG1", 60});
            m_ports.append({"Eth8", "100G", "DOWN", "", 0});
        } else {
            for (const auto& portVal : ports) {
                QJsonObject port = portVal.toObject();
                PortInfo info;
                info.name = port["name"].toString("Eth0");
                info.speed = port["speed"].toString("100G");
                info.state = port["state"].toString("UP");
                info.type = port["type"].toString();
                info.queue = port["queue"].toInt(40);
                m_ports.append(info);
            }
        }

        // Default stats
        m_stats.packets = stats["packets"].toInt(12400);
        m_stats.drops = stats["drops"].toInt(42);
        m_stats.cpu = stats["cpu"].toInt(3);

        createGraphicsItems();
    }

    void setActive(bool active) {
        m_active = active;
        update();
    }

    QRectF boundingRect() const override {
        return QRectF(0, 0, 200, 180);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        // Background
        QColor bgColor = m_active ? QColor(102, 187, 106) : QColor(25, 118, 210);
        QColor strokeColor = m_active ? QColor(129, 199, 132) : QColor(79, 195, 247);

        painter->setBrush(bgColor);
        painter->setPen(QPen(strokeColor, 2));
        painter->drawRect(0, 0, 200, 180);

        // Title section
        painter->setPen(Qt::white);
        painter->setFont(QFont("Segoe UI", 11, QFont::Bold));
        painter->drawText(10, 18, QString("%1 (%2)").arg(m_name, m_role));

        // Ports section header
        painter->setPen(QColor(227, 242, 253));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(10, 35, "Ports:");

        // Draw ports
        int portY = 50;
        for (const auto& port : m_ports) {
            // Port name and speed
            painter->setPen(Qt::white);
            painter->setFont(QFont("Segoe UI", 8));
            QString portText = QString("%1 %2 %3").arg(port.name, port.speed, port.type);
            painter->drawText(10, portY, portText);

            // State indicator circle
            QColor stateColor;
            if (port.state == "UP") stateColor = QColor(76, 175, 80);
            else if (port.state == "DOWN") stateColor = QColor(244, 67, 54);
            else stateColor = QColor(255, 152, 0);

            painter->setBrush(stateColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(130, portY - 7, 8, 8);

            // Queue utilization bar
            int queueWidth = 50;
            int queueHeight = 6;
            int queueX = 145;
            int queueY = portY - 9;

            // Background
            painter->setBrush(QColor(51, 51, 51));
            painter->drawRect(queueX, queueY, queueWidth, queueHeight);

            // Fill
            QColor queueColor;
            if (port.queue > 70) queueColor = QColor(244, 67, 54);
            else if (port.queue > 40) queueColor = QColor(255, 152, 0);
            else queueColor = QColor(76, 175, 80);

            painter->setBrush(queueColor);
            painter->drawRect(queueX, queueY, queueWidth * port.queue / 100, queueHeight);

            portY += 18;
        }

        // Routing section
        int yPos = 105;
        if (!m_routing.isEmpty()) {
            painter->setPen(QColor(227, 242, 253));
            painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter->drawText(10, yPos, "Routing:");

            painter->setPen(Qt::white);
            painter->setFont(QFont("Segoe UI", 8));
            painter->drawText(10, yPos + 13, m_routing);
            yPos += 28;
        }

        // ACL section
        if (!m_acl.isEmpty()) {
            painter->setPen(QColor(227, 242, 253));
            painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter->drawText(10, yPos, "ACL:");

            painter->setPen(Qt::white);
            painter->setFont(QFont("Segoe UI", 8));
            painter->drawText(10, yPos + 13, m_acl);
            yPos += 28;
        }

        // QoS section
        if (!m_qos.isEmpty()) {
            painter->setPen(QColor(227, 242, 253));
            painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter->drawText(10, yPos, "QoS:");

            painter->setPen(Qt::white);
            painter->setFont(QFont("Segoe UI", 8));
            painter->drawText(10, yPos + 13, m_qos);
            yPos += 28;
        }

        // Stats
        painter->setPen(QColor(227, 242, 253));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(10, 170, QString("Stats: %1pps, drops %2, CPU %3%")
            .arg(m_stats.packets).arg(m_stats.drops).arg(m_stats.cpu));
    }

private:
    struct PortInfo {
        QString name, speed, state, type;
        int queue;
    };
    struct Stats {
        int packets, drops, cpu;
    };

    QString m_name, m_role, m_routing, m_acl, m_qos;
    QVector<PortInfo> m_ports;
    Stats m_stats;
    bool m_active;

    void createGraphicsItems() {
        // Items are drawn in paint() for better control
    }
};

// Host item like web visualizer
class HostItem : public QGraphicsItem {
public:
    HostItem(const QString& name, const QString& ip = "", const QString& mac = "")
        : m_name(name), m_ip(ip), m_mac(mac), m_active(false) {
    }

    void setActive(bool active) {
        m_active = active;
        update();
    }

    QRectF boundingRect() const override {
        return QRectF(0, 0, 80, 40);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        QColor bgColor = m_active ? QColor(255, 152, 0) : QColor(96, 125, 139);
        QColor strokeColor = m_active ? QColor(255, 183, 77) : QColor(144, 164, 174);

        painter->setBrush(bgColor);
        painter->setPen(QPen(strokeColor, 2));
        painter->drawRect(0, 0, 80, 40);

        painter->setPen(Qt::white);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(5, 15, m_name);

        painter->setFont(QFont("Segoe UI", 8));
        painter->drawText(5, 28, m_ip.isEmpty() ? "10.0.x.x" : m_ip);
    }

private:
    QString m_name, m_ip, m_mac;
    bool m_active;
};

// Animated packet for smooth movement
struct AnimatedPacket {
    PacketCardItem* item;
    double x, y;
    double targetX, targetY;
    double progress;
    double speed;
    QString nextHop;
};

/**
 * @brief Qt6 Visualizer for Mini_SONiC - matches web visualizer functionality
 */
class MiniSonicVisualizer : public QMainWindow {
    Q_OBJECT

public:
    MiniSonicVisualizer(QWidget *parent = nullptr) : QMainWindow(parent),
        m_speed(1), m_eventCount(0), m_packetCount(0) {
        m_coreStd = std::make_shared<VisualizerCoreStd>(m_ioContext);
        setupUI();
        setupCoreConnections();
        
        m_networkThread = std::thread([this]() {
            auto work = boost::asio::make_work_guard(m_ioContext);
            m_ioContext.run();
        });
        
        startAnimation();
    }

    ~MiniSonicVisualizer() {
        m_ioContext.stop();
        if (m_networkThread.joinable()) {
            m_networkThread.join();
        }
    }

private slots:
    void logEvent(const QString& type, const QString& details) {
        m_eventCount++;
        QString time = QTime::currentTime().toString("hh:mm:ss");
        m_eventLog->appendPlainText(QString("[%1] %2: %3").arg(time, type, details));
    }

    void onConnectClicked() {
        m_coreStd->connect("127.0.0.1", 8080);
    }

    void onDisconnectClicked() {
        m_coreStd->disconnect();
    }

    void changeSpeed(int delta) {
        int newSpeed = m_speed + delta;
        if (newSpeed >= 1 && newSpeed <= 10) {
            m_speed = newSpeed;
            m_speedLabel->setText(QString("%1x").arg(m_speed));

            m_coreStd->sendSpeedControl(m_speed);
        }
    }

    void updateAnimation() {
        // Update packet positions
        for (auto it = m_activePackets.begin(); it != m_activePackets.end(); ) {
            AnimatedPacket& pkt = *it;
            pkt.progress += pkt.speed * m_speed;

            if (pkt.progress >= 1.0) {
                // Remove completed packet
                if (pkt.item->scene()) {
                    pkt.item->scene()->removeItem(pkt.item);
                }
                delete pkt.item;
                it = m_activePackets.erase(it);
                continue;
            }

            // Smooth interpolation
            double newX = pkt.x + (pkt.targetX - pkt.x) * pkt.speed * m_speed;
            double newY = pkt.y + (pkt.targetY - pkt.y) * pkt.speed * m_speed;
            pkt.x = newX;
            pkt.y = newY;

            // Calculate tilt based on direction
            double dx = pkt.targetX - pkt.x;
            double dy = pkt.targetY - pkt.y;
            double tilt = std::atan2(dy, dx) * 0.1;

            // Apply vibration if queued (random jitter)
            double vibX = 0, vibY = 0;
            if (pkt.item && false) { // queued state - disabled for now
                vibX = (QRandomGenerator::global()->bounded(100) - 50) / 25.0;
                vibY = (QRandomGenerator::global()->bounded(100) - 50) / 25.0;
            }

            pkt.item->setPos(newX, newY);
            pkt.item->setTilt(tilt);
            pkt.item->setVibration(vibX, vibY);

            ++it;
        }
    }

private:
    void setupUI() {
        setWindowTitle("Mini_SONiC Network Visualizer");
        resize(1600, 1000);

        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        auto* mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Header
        auto* headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(15);

        auto* titleLabel = new QLabel("Mini_SONiC Network Visualizer");
        titleLabel->setStyleSheet("font-size: 24px; color: #4fc3f7; font-weight: bold;");
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        m_connectButton = new QPushButton("Connect");
        m_connectButton->setStyleSheet("background: #1976d2; color: white; padding: 8px 16px;");
        connect(m_connectButton, &QPushButton::clicked, this, &MiniSonicVisualizer::onConnectClicked);
        headerLayout->addWidget(m_connectButton);

        m_disconnectButton = new QPushButton("Disconnect");
        m_disconnectButton->setStyleSheet("background: #d32f2f; color: white; padding: 8px 16px;");
        m_disconnectButton->setEnabled(false);
        connect(m_disconnectButton, &QPushButton::clicked, this, &MiniSonicVisualizer::onDisconnectClicked);
        headerLayout->addWidget(m_disconnectButton);

        m_statusLabel = new QLabel("Disconnected");
        m_statusLabel->setStyleSheet("color: #f44336; padding: 8px 15px; background: rgba(0,0,0,0.3); border-radius: 5px;");
        headerLayout->addWidget(m_statusLabel);

        // Speed control
        auto* speedLayout = new QHBoxLayout();
        speedLayout->addWidget(new QLabel("Speed:"));
        auto* speedDownBtn = new QPushButton("-");
        speedDownBtn->setFixedWidth(30);
        connect(speedDownBtn, &QPushButton::clicked, [this]() { changeSpeed(-1); });
        speedLayout->addWidget(speedDownBtn);

        m_speedLabel = new QLabel("1x");
        m_speedLabel->setStyleSheet("color: #4fc3f7; font-weight: bold; min-width: 30px;");
        m_speedLabel->setAlignment(Qt::AlignCenter);
        speedLayout->addWidget(m_speedLabel);

        auto* speedUpBtn = new QPushButton("+");
        speedUpBtn->setFixedWidth(30);
        connect(speedUpBtn, &QPushButton::clicked, [this]() { changeSpeed(1); });
        speedLayout->addWidget(speedUpBtn);

        headerLayout->addLayout(speedLayout);

        // Left panel - visualization
        auto* leftPanel = new QVBoxLayout();
        leftPanel->addLayout(headerLayout);

        // Scene setup with dark background
        auto* scene = new QGraphicsScene(this);
        scene->setSceneRect(0, 0, 1200, 800);
        scene->setBackgroundBrush(QColor(26, 26, 46));

        m_view = new QGraphicsView(scene);
        m_view->setRenderHint(QPainter::Antialiasing);
        m_view->setStyleSheet("background: #1a1a2e; border: none;");
        m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        leftPanel->addWidget(m_view, 1);

        mainLayout->addLayout(leftPanel, 2);

        // Right panel - event log and stats
        auto* rightPanel = new QVBoxLayout();
        rightPanel->setSpacing(0);
        rightPanel->setContentsMargins(0, 0, 0, 0);

        // Stats panel
        auto* statsWidget = new QWidget();
        statsWidget->setStyleSheet("background: rgba(0,0,0,0.3); padding: 15px;");
        auto* statsLayout = new QVBoxLayout(statsWidget);

        auto* statsHeader = new QLabel("Statistics");
        statsHeader->setStyleSheet("color: #4fc3f7; font-size: 14px; font-weight: bold; margin-bottom: 10px;");
        statsLayout->addWidget(statsHeader);

        auto addStat = [&](const QString& label, const QString& value, const QString& valueColor = "#4fc3f7") {
            auto* row = new QHBoxLayout();
            auto* lbl = new QLabel(label);
            lbl->setStyleSheet("color: #888;");
            auto* val = new QLabel(value);
            val->setStyleSheet(QString("color: %1; font-weight: bold;").arg(valueColor));
            row->addWidget(lbl);
            row->addStretch();
            row->addWidget(val);
            statsLayout->addLayout(row);
        };

        addStat("Events Received:", "0");
        addStat("Packets Processed:", "0");
        addStat("Active Switches:", "0");
        addStat("Active Links:", "0");
        addStat("Active Hosts:", "0");
        addStat("Packet Drops:", "0", "#f44336");

        statsLayout->addStretch();
        rightPanel->addWidget(statsWidget);

        // Event log
        auto* eventHeader = new QLabel("Event Log");
        eventHeader->setStyleSheet("color: #4fc3f7; font-size: 14px; font-weight: bold; padding: 15px; background: rgba(0,0,0,0.2);");
        rightPanel->addWidget(eventHeader);

        m_eventLog = new QPlainTextEdit();
        m_eventLog->setReadOnly(true);
        m_eventLog->setMaximumBlockCount(100);
        m_eventLog->setStyleSheet("background: rgba(0,0,0,0.3); color: #e0e0e0; border: none; padding: 15px;");
        rightPanel->addWidget(m_eventLog, 1);

        // Packet history
        auto* historyHeader = new QLabel("Packet Traversal History");
        historyHeader->setStyleSheet("color: #4fc3f7; font-size: 14px; font-weight: bold; padding: 15px; background: rgba(0,0,0,0.2);");
        rightPanel->addWidget(historyHeader);

        m_historyLog = new QPlainTextEdit();
        m_historyLog->setReadOnly(true);
        m_historyLog->setMaximumBlockCount(50);
        m_historyLog->setStyleSheet("background: rgba(0,0,0,0.3); color: #e0e0e0; border: none; padding: 15px;");
        rightPanel->addWidget(m_historyLog, 1);

        mainLayout->addLayout(rightPanel, 1);
    }

    void setupCoreConnections() {
        m_coreStd->onConnectionStatus = [this](bool connected, const std::string& status) {
            QMetaObject::invokeMethod(this, [this, connected, status = QString::fromStdString(status)]() {
                m_statusLabel->setText(status);
                m_statusLabel->setStyleSheet(connected ? "color: #4caf50;" : "color: #f44336;");
                m_connectButton->setEnabled(!connected);
                m_disconnectButton->setEnabled(connected);
            });
        };

        m_coreStd->onLogMessage = [this](const std::string& type, const std::string& details) {
            QMetaObject::invokeMethod(this, [this, type = QString::fromStdString(type), details = QString::fromStdString(details)]() {
                logEvent(type, details);
            });
        };

        m_coreStd->onTopologyLoaded = [this](const json& topo) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(topo.dump()));
            QMetaObject::invokeMethod(this, [this, obj = doc.object()]() {
                handleTopology(obj);
            });
        };

        m_coreStd->requestPacketAnimation = [this](const std::string& pId, const std::string& from, const std::string& to, const json& details) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(details.dump()));
            QMetaObject::invokeMethod(this, [this, 
                pId = QString::fromStdString(pId), 
                from = QString::fromStdString(from), 
                to = QString::fromStdString(to), 
                details = doc.object()]() {
                animatePacketMovement(pId, from, to, details);
            });
        };

        m_coreStd->nodeActivated = [this](const std::string& id, bool isSw) {
            QMetaObject::invokeMethod(this, [this, id = QString::fromStdString(id), isSw]() {
                if (isSw && m_switches.contains(id)) {
                    m_switches[id]->setActive(true);
                    QTimer::singleShot(500, [this, id]() { if(m_switches.contains(id)) m_switches[id]->setActive(false); });
                }
            });
        };

        m_coreStd->linkActivated = [this](const std::string& s, const std::string& t) {
            QMetaObject::invokeMethod(this, [this, s = QString::fromStdString(s), t = QString::fromStdString(t)]() {
                QString key = makeLinkKey(s, t);
                if (m_linkMap.contains(key)) {
                    auto* l = m_linkMap[key];
                    l->setPen(QPen(QColor(76, 175, 80), 4));
                    QTimer::singleShot(300, [l]() { if(l) l->setPen(QPen(QColor(102, 187, 106), 2)); });
                }
            });
        };

        m_coreStd->portStateChanged = [this](const std::string& swId, const std::string& port, const std::string& state) {
            QMetaObject::invokeMethod(this, [this, swId = QString::fromStdString(swId), port = QString::fromStdString(port), state = QString::fromStdString(state)]() {
                logEvent("PortStateChanged", QString("Port %1 on %2 changed to %3").arg(port, swId, state));
            });
        };
    }

    void startAnimation() {
        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MiniSonicVisualizer::updateAnimation);
        timer->start(16); // ~60 FPS
    }

    void animatePacketMovement(const QString& pId, const QString& fromId, const QString& toId, const QJsonObject& obj) {
        Q_UNUSED(obj);

        QPointF fromPos, toPos;
        
        if (m_switches.contains(fromId)) fromPos = m_switches[fromId]->pos() + QPointF(100, 90);
        else if (m_hosts.contains(fromId)) fromPos = m_hosts[fromId]->pos() + QPointF(40, 20);
        else return;

        if (m_switches.contains(toId)) toPos = m_switches[toId]->pos() + QPointF(100, 90);
        else if (m_hosts.contains(toId)) toPos = m_hosts[toId]->pos() + QPointF(40, 20);
        else return;

        auto* card = new PacketCardItem(pId, "10.0.1.2", "10.0.3.7", "443", "51832", "46", "62");
        card->setPos(fromPos);
        m_view->scene()->addItem(card);

        AnimatedPacket pkt;
        pkt.item = card;
        pkt.x = fromPos.x();
        pkt.y = fromPos.y();
        pkt.targetX = toPos.x();
        pkt.targetY = toPos.y();
        pkt.progress = 0;
        pkt.speed = 0.005;
        pkt.nextHop = toId;
        m_activePackets.append(pkt);
        
        m_historyLog->appendPlainText(QString("Packet %1: %2 -> %3").arg(pId, fromId, toId));
    }

    void handleTopology(const QJsonObject& obj) {
        m_view->scene()->clear();
        m_switches.clear();
        m_hosts.clear();
        m_hostIpMap.clear();
        m_linkMap.clear();

        QRectF rect = m_view->scene()->sceneRect();
        double panelWidth = rect.width();
        double panelHeight = rect.height();

        // Calculate bounds
        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::min();
        double maxY = std::numeric_limits<double>::min();

        // First pass: find bounds from nodes
        QJsonArray nodes = obj["nodes"].toArray();
        for (const auto& nodeVal : nodes) {
            QJsonObject node = nodeVal.toObject();
            double x = node["x"].toDouble();
            double y = node["y"].toDouble();
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }

        double contentWidth = maxX - minX;
        double contentHeight = maxY - minY;
        double scaleX = (contentWidth > 0) ? (panelWidth - 100) / contentWidth : 1.0;
        double scaleY = (contentHeight > 0) ? (panelHeight - 100) / contentHeight : 1.0;
        double scale = std::min(scaleX, scaleY);

        // Create switches and hosts
        QMap<QString, QPointF> nodePositions;

        for (const auto& nodeVal : nodes) {
            QJsonObject node = nodeVal.toObject();
            QString id = node["id"].toString();
            QString type = node["type"].toString();
            double x = node["x"].toDouble();
            double y = node["y"].toDouble();

            double scaledX = 50 + (x - minX) * scale - (type == "switch" ? 100 : 40);
            double scaledY = 50 + (y - minY) * scale - (type == "switch" ? 90 : 20);

            if (type == "switch") {
                auto* sw = new SwitchItem(id, node["role"].toString("switch"));
                sw->setPos(scaledX, scaledY);
                m_view->scene()->addItem(sw);
                m_switches[id] = sw;
                nodePositions[id] = QPointF(scaledX + 100, scaledY + 90);
            } else if (type == "host") {
                auto* host = new HostItem(id, node["ip"].toString(), node["mac"].toString());
                host->setPos(scaledX, scaledY);
                m_view->scene()->addItem(host);
                m_hosts[id] = host;
                m_hostIpMap[node["ip"].toString()] = id;
                nodePositions[id] = QPointF(scaledX + 40, scaledY + 20);
            }
        }

        // Create links
        QJsonArray links = obj["links"].toArray();
        for (const auto& linkVal : links) {
            QJsonObject link = linkVal.toObject();
            QString source = link["source"].toString();
            QString target = link["target"].toString();

            if (nodePositions.contains(source) && nodePositions.contains(target)) {
                QPointF src = nodePositions[source];
                QPointF tgt = nodePositions[target];

                auto* line = m_view->scene()->addLine(src.x(), src.y(), tgt.x(), tgt.y());
                
                QString linkKey = makeLinkKey(source, target);
                m_linkMap[linkKey] = line;
            }
        }

        m_eventLog->appendPlainText(QString("[Topology] Loaded %1 switches, %2 hosts, %3 links")
            .arg(m_switches.size()).arg(m_hosts.size()).arg(m_linkMap.size()));
    }

    void handlePacketGenerated(const QJsonObject& obj) {
        QJsonObject packet = obj["packet"].toObject();
        QString id = QString::number(packet["id"].toVariant().toLongLong());
        QString srcIp = packet["src_ip"].toString();
        QString dstIp = packet["dst_ip"].toString();

        // Save port/IP information for the first segment of the path
        m_packetHistory[id].clear();
        m_packetHistory[id].append(srcIp);
        m_packetHistory[id].append(dstIp);

        logEvent("PacketGenerated", QString("Packet %1: %2 -> %3").arg(id, srcIp, dstIp));
        m_packetCount++;
    }

    void handlePacketEnteredSwitch(const QJsonObject& obj) {
        QString switchId = obj["switch_id"].toString();
        QString packetId = QString::number(obj["packet_id"].toVariant().toLongLong());
        QString ingressPort = obj["ingress_port"].toString();

        logEvent("PacketEnteredSwitch", QString("Packet %1 entered %2 on %3")
            .arg(packetId, switchId, ingressPort));

        // If this is the first time the packet enters a switch, animate the path from the Host
        if (m_packetHistory.contains(packetId) && m_packetHistory[packetId].size() == 2) {
            QString srcIp = m_packetHistory[packetId][0];
            QString dstIp = m_packetHistory[packetId][1];

            if (m_hostIpMap.contains(srcIp) && m_switches.contains(switchId)) {
                QString hostId = m_hostIpMap[srcIp];
                HostItem* fromHost = m_hosts[hostId];
                SwitchItem* toSw = m_switches[switchId];

                QPointF fromPos(fromHost->pos().x() + 40, fromHost->pos().y() + 20);
                QPointF toPos(toSw->pos().x() + 100, toSw->pos().y() + 90);

                auto* card = new PacketCardItem(packetId, srcIp, dstIp, "443", "51832", "46", "62");
                card->setPos(fromPos);
                m_view->scene()->addItem(card);

                AnimatedPacket pkt;
                pkt.item = card;
                pkt.x = fromPos.x();
                pkt.y = fromPos.y();
                pkt.targetX = toPos.x();
                pkt.targetY = toPos.y();
                pkt.progress = 0;
                pkt.speed = 0.005;
                pkt.nextHop = switchId;
                m_activePackets.append(pkt);
            }
        }

        m_packetHistory[packetId].append(switchId);

        // Highlight switch
        if (m_switches.contains(switchId)) {
            m_switches[switchId]->setActive(true);
            QTimer::singleShot(500, [this, switchId]() {
                if (m_switches.contains(switchId)) {
                    m_switches[switchId]->setActive(false);
                }
            });
        }
    }

    void handlePacketForwardDecision(const QJsonObject& obj) {
        QString switchId = obj["switch_id"].toString();
        QString egressPort = obj["egress_port"].toString();
        QString nextHop = obj["next_hop"].toString();

        logEvent("PacketForwardDecision", QString("%1 -> %2 via %3")
            .arg(switchId, nextHop, egressPort));

        // Highlight only the specific link involved
        QString linkKey = makeLinkKey(switchId, nextHop);
        if (m_linkMap.contains(linkKey)) {
            auto* link = m_linkMap[linkKey];
            link->setPen(QPen(QColor(76, 175, 80), 4));
            QTimer::singleShot(300, [link]() {
                if (link) link->setPen(QPen(QColor(102, 187, 106), 2));
            });
        }
    }

    void handlePacketExitedSwitch(const QJsonObject& obj) {
        QString switchId = obj["switch_id"].toString();
        QString packetId = QString::number(obj["packet_id"].toVariant().toLongLong());
        QString egressPort = obj["egress_port"].toString();
        QString nextHop = obj["next_hop"].toString();
        QString srcIp = obj["src_ip"].toString("10.0.1.2");
        QString dstIp = obj["dst_ip"].toString("10.0.3.7");

        logEvent("PacketExitedSwitch", QString("Packet %1 exited %2 via %3 to %4")
            .arg(packetId, switchId, egressPort, nextHop));

        // Add to history
        m_historyLog->appendPlainText(QString("Packet %1 exited %2 to %3")
            .arg(packetId, switchId, nextHop));

        // Create animated packet
        if (m_switches.contains(switchId) && (m_switches.contains(nextHop) || m_hosts.contains(nextHop))) {
            QPointF fromPos, toPos;

            SwitchItem* fromSw = m_switches[switchId];
            fromPos = QPointF(fromSw->pos().x() + 100, fromSw->pos().y() + 90);

            if (m_switches.contains(nextHop)) {
                SwitchItem* toSw = m_switches[nextHop];
                toPos = QPointF(toSw->pos().x() + 100, toSw->pos().y() + 90);
            } else {
                HostItem* toHost = m_hosts[nextHop];
                toPos = QPointF(toHost->pos().x() + 40, toHost->pos().y() + 20);
            }

            auto* card = new PacketCardItem(packetId, srcIp, dstIp, "443", "51832", "46", "62");
            card->setPos(fromPos);
            m_view->scene()->addItem(card);

            AnimatedPacket pkt;
            pkt.item = card;
            pkt.x = fromPos.x();
            pkt.y = fromPos.y();
            pkt.targetX = toPos.x();
            pkt.targetY = toPos.y();
            pkt.progress = 0;
            pkt.speed = 0.005; // Base speed, multiplied by m_speed
            pkt.nextHop = nextHop;

            m_activePackets.append(pkt);
        }
    }

    void handlePortStateChanged(const QJsonObject& obj) {
        QString switchId = obj["switch_id"].toString();
        QString port = obj["port"].toString();
        QString state = obj["state"].toString();

        logEvent("PortStateChanged", QString("Port %1 on %2 changed to %3")
            .arg(port, switchId, state));
    }

    QString makeLinkKey(const QString& s1, const QString& s2) {
        return (s1 < s2) ? (s1 + "-" + s2) : (s2 + "-" + s1);
    }

private:
    QGraphicsView* m_view = nullptr;
    boost::asio::io_context m_ioContext;
    std::shared_ptr<VisualizerCoreStd> m_coreStd;
    std::thread m_networkThread;
    QPushButton* m_connectButton = nullptr;
    QPushButton* m_disconnectButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_speedLabel = nullptr;
    QPlainTextEdit* m_eventLog = nullptr;
    QPlainTextEdit* m_historyLog = nullptr;

    int m_speed;
    int m_eventCount;
    int m_packetCount;

    QMap<QString, SwitchItem*> m_switches;
    QMap<QString, HostItem*> m_hosts;
    QMap<QString, QString> m_hostIpMap; // IP -> HostID
    QMap<QString, QGraphicsLineItem*> m_linkMap;
    QVector<AnimatedPacket> m_activePackets;
    QMap<QString, QStringList> m_packetHistory;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Dark theme
    app.setStyleSheet(R"(
        QWidget {
            background-color: #1a1a2e;
            color: #e0e0e0;
            font-family: 'Segoe UI';
        }
        QPushButton {
            background: #1976d2;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background: #1565c0;
        }
        QPushButton:disabled {
            background: #555;
        }
        QLabel {
            color: #e0e0e0;
        }
        QPlainTextEdit {
            background: rgba(0,0,0,0.3);
            border: none;
        }
    )");

    MiniSonicVisualizer visualizer;
    visualizer.show();

    return app.exec();
}

#include "qt_visualizer.moc"
