#include <QApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView> // For QGraphicsView
#include <QGraphicsRectItem> // For QGraphicsRectItem
#include <QGraphicsTextItem> // For QGraphicsTextItem
#include <QTimer> // For QTimer
#include <QTcpSocket> // For QTcpSocket
#include <QJsonDocument> // For QJsonDocument
#include <QJsonObject> // For QJsonObject
#include <QJsonArray> // For QJsonArray
#include <QVBoxLayout> // For QVBoxLayout
#include <QHBoxLayout> // For QHBoxLayout
#include <QLabel> // For QLabel
#include <QPushButton> // For QPushButton
#include <QPlainTextEdit> // For QPlainTextEdit
#include <QMap> // For QMap
#include <QColor> // For QColor
#include <QBrush> // For QBrush
#include <QPen> // For QPen

/**
 * @brief Qt6 Visualizer for Mini_SONiC
 * 
 * Visualizes network packets and switch states in real-time.
 * Connects to event stream via TCP socket and renders the visualization.
 */
class MiniSonicVisualizer : public QMainWindow {
    Q_OBJECT

public:
    MiniSonicVisualizer(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupNetwork();
    }

    ~MiniSonicVisualizer() {
        if (m_socket) {
            m_socket->disconnectFromHost();
        }
    }

private slots:
    void onConnectClicked() {
        m_socket->connectToHost("127.0.0.1", 8080);
        m_statusLabel->setText("Connecting...");
    }

    void onSocketConnected() {
        m_statusLabel->setText("Connected to event stream");
        m_connectButton->setEnabled(false);
    }

    void onSocketDisconnected() {
        m_statusLabel->setText("Disconnected");
        m_connectButton->setEnabled(true);
    }

    void onSocketError(QAbstractSocket::SocketError /*error*/) {
        m_statusLabel->setText("Socket error: " + m_socket->errorString());
    }

    void onSocketReadyRead() {
        while (m_socket->canReadLine()) {
            QByteArray line = m_socket->readLine();
            processEvent(line);
        }
    }

private:
    void setupUI() {
        setWindowTitle("Mini_SONiC Visualizer");
        resize(1200, 800);

        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        auto* mainLayout = new QHBoxLayout(centralWidget);

        // Left panel: Network topology visualization
        auto* leftPanel = new QVBoxLayout();
        
        auto* scene = new QGraphicsScene(this);
        scene->setSceneRect(0, 0, 800, 600);
        
        m_view = new QGraphicsView(scene);
        m_view->setRenderHint(QPainter::Antialiasing);
        m_view->setDragMode(QGraphicsView::ScrollHandDrag);
        
        leftPanel->addWidget(m_view);
        
        // Control buttons
        auto* controlLayout = new QHBoxLayout();
        m_connectButton = new QPushButton("Connect to Event Stream");
        connect(m_connectButton, &QPushButton::clicked, this, &MiniSonicVisualizer::onConnectClicked);
        
        m_statusLabel = new QLabel("Not connected");
        
        controlLayout->addWidget(m_connectButton);
        controlLayout->addWidget(m_statusLabel);
        controlLayout->addStretch();
        
        leftPanel->addLayout(controlLayout);
        
        mainLayout->addLayout(leftPanel, 2);

        // Right panel: Event log
        auto* rightPanel = new QVBoxLayout();
        m_eventLog = new QPlainTextEdit();
        m_eventLog->setReadOnly(true);
        m_eventLog->setMaximumBlockCount(1000);
        
        rightPanel->addWidget(new QLabel("Event Log:"));
        rightPanel->addWidget(m_eventLog);
        
        mainLayout->addLayout(rightPanel, 1);

        // Initialize topology visualization
        initializeTopology(scene);
    }

    void setupNetwork() {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, &MiniSonicVisualizer::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &MiniSonicVisualizer::onSocketDisconnected);
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                this, &MiniSonicVisualizer::onSocketError);
        connect(m_socket, &QTcpSocket::readyRead, this, &MiniSonicVisualizer::onSocketReadyRead);
    }

    void initializeTopology(QGraphicsScene* scene) {
        // Create switches
        createSwitch(scene, 100, 200, "TOR1", "Top-of-Rack");
        createSwitch(scene, 400, 100, "Spine1", "Spine");
        createSwitch(scene, 400, 300, "Spine2", "Spine");
        createSwitch(scene, 700, 200, "TOR2", "Top-of-Rack");

        // Create links
        createLink(scene, 200, 225, 350, 125);  // TOR1 -> Spine1
        createLink(scene, 200, 275, 350, 275);  // TOR1 -> Spine2
        createLink(scene, 450, 125, 600, 225);  // Spine1 -> TOR2
        createLink(scene, 450, 275, 600, 275);  // Spine2 -> TOR2

        // Create hosts
        createHost(scene, 50, 250, "HostA");
        createHost(scene, 750, 250, "HostB");

        // Connect hosts to switches
        createLink(scene, 80, 250, 100, 225);  // HostA -> TOR1
        createLink(scene, 700, 225, 730, 250);  // TOR2 -> HostB
    }

    void createSwitch(QGraphicsScene* scene, int x, int y, const QString& name, const QString& role) {
        auto* rect = new QGraphicsRectItem(x, y, 120, 80);
        rect->setBrush(QBrush(QColor(64, 128, 255)));
        rect->setPen(QPen(Qt::black, 2));
        scene->addItem(rect);

        auto* nameText = new QGraphicsTextItem(name);
        nameText->setDefaultTextColor(Qt::white);
        nameText->setFont(QFont("Arial", 10, QFont::Bold));
        nameText->setPos(x + 10, y + 10);
        scene->addItem(nameText);

        auto* roleText = new QGraphicsTextItem(role);
        roleText->setDefaultTextColor(Qt::white);
        roleText->setFont(QFont("Arial", 8));
        roleText->setPos(x + 10, y + 35);
        scene->addItem(roleText);

        // Store switch item for updates
        m_switches[name] = rect;
    }

    void createHost(QGraphicsScene* scene, int x, int y, const QString& name) {
        auto* rect = new QGraphicsRectItem(x, y, 30, 30);
        rect->setBrush(QBrush(QColor(128, 128, 128)));
        rect->setPen(QPen(Qt::black, 2));
        scene->addItem(rect);

        auto* text = new QGraphicsTextItem(name);
        text->setDefaultTextColor(Qt::black);
        text->setFont(QFont("Arial", 8));
        text->setPos(x - 10, y + 35);
        scene->addItem(text);

        m_hosts[name] = rect;
    }

    void createLink(QGraphicsScene* scene, int x1, int y1, int x2, int y2) {
        auto* line = scene->addLine(x1, y1, x2, y2);
        line->setPen(QPen(Qt::gray, 2));
        m_links.append(line);
    }

    void processEvent(const QByteArray& eventData) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(eventData, &error);
        
        if (error.error != QJsonParseError::NoError) {
            m_eventLog->appendPlainText("Error parsing JSON: " + error.errorString());
            return;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        m_eventLog->appendPlainText(QString("[%1] %2").arg(
            QTime::currentTime().toString(),
            type
        ));

        if (type == "PacketGenerated") {
            handlePacketGenerated(obj);
        } else if (type == "PacketEnteredSwitch") {
            handlePacketEnteredSwitch(obj);
        } else if (type == "PacketForwardDecision") {
            handlePacketForwardDecision(obj);
        } else if (type == "PacketExitedSwitch") {
            handlePacketExitedSwitch(obj);
        } else if (type == "PortStateChanged") {
            handlePortStateChanged(obj);
        }
    }

    void handlePacketGenerated(const QJsonObject& obj) {
        QJsonObject packet = obj["packet"].toObject();
        uint64_t id = packet["id"].toInteger();
        QString src_ip = packet["src_ip"].toString();
        QString dst_ip = packet["dst_ip"].toString();

        m_eventLog->appendPlainText(QString("  Packet %1: %2 -> %3").arg(id).arg(src_ip, dst_ip));
    }

    void handlePacketEnteredSwitch(const QJsonObject& obj) {
        QString switch_id = obj["switch_id"].toString();
        QString ingress_port = obj["ingress_port"].toString();
        uint64_t packet_id = obj["packet_id"].toInteger();

        m_eventLog->appendPlainText(QString("  Packet %1 entered %2 on %3").arg(packet_id).arg(switch_id, ingress_port));

        // Highlight switch
        if (m_switches.contains(switch_id)) {
            auto* item = m_switches[switch_id];
            item->setBrush(QBrush(QColor(0, 255, 0)));
            
            // Reset after delay
            QTimer::singleShot(500, [item]() {
                item->setBrush(QBrush(QColor(64, 128, 255)));
            });
        }
    }

    void handlePacketForwardDecision(const QJsonObject& obj) {
        QString switch_id = obj["switch_id"].toString();
        QString egress_port = obj["egress_port"].toString();
        QString next_hop = obj["next_hop"].toString();

        m_eventLog->appendPlainText(QString("  Forward decision: %1 -> %2 via %3").arg(switch_id, next_hop, egress_port));
    }

    void handlePacketExitedSwitch(const QJsonObject& obj) {
        QString switch_id = obj["switch_id"].toString();
        QString egress_port = obj["egress_port"].toString();
        uint64_t packet_id = obj["packet_id"].toInteger();

        m_eventLog->appendPlainText(QString("  Packet %1 exited %2 via %3").arg(packet_id).arg(switch_id, egress_port));
    }

    void handlePortStateChanged(const QJsonObject& obj) {
        QString switch_id = obj["switch_id"].toString();
        QString port = obj["port"].toString();
        QString state = obj["state"].toString();

        m_eventLog->appendPlainText(QString("  Port %1 on %2 changed to %3").arg(port, switch_id, state));
    }

private:
    QGraphicsView* m_view = nullptr;
    QTcpSocket* m_socket = nullptr;
    QPushButton* m_connectButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPlainTextEdit* m_eventLog = nullptr;
    
    QMap<QString, QGraphicsRectItem*> m_switches;
    QMap<QString, QGraphicsRectItem*> m_hosts;
    QList<QGraphicsLineItem*> m_links;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    MiniSonicVisualizer visualizer;
    visualizer.show();
    
    return app.exec();
}

#include "qt_visualizer.moc"
