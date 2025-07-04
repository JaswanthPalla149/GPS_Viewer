#include "TcpServer.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

TcpServer::TcpServer(QObject *parent) : QObject(parent) {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
    if (!server->listen(QHostAddress::Any, 12345)) {
        qWarning() << "❌ Failed to start server:" << server->errorString();
    } else {
        qDebug() << "✅ GPS TCP Server running on port 12345";
    }
}

void TcpServer::onNewConnection() {
    QTcpSocket *socket = server->nextPendingConnection();
    qDebug() << "🔌 New TCP connection established from" << socket->peerAddress().toString();

    connect(socket, &QTcpSocket::readyRead, [=]() {
        QByteArray data = socket->readAll();
        qDebug() << "📩 Received raw data:" << data;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "❌ JSON parse error:" << parseError.errorString();
            return;
        }

        QVariantList list;
        QJsonArray arr;

        // Accept either nested JSON object with "detections" field or raw array
        if (doc.isObject()) {
            arr = doc.object().value("detections").toArray();
        } else if (doc.isArray()) {
            arr = doc.array();
        }

        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            QVariantMap m;
            m["class_id"] = obj["class_id"].toString();
            m["lat"] = obj["lat"].toDouble();
            m["lon"] = obj["lon"].toDouble();

            qDebug() << "👉 Raw JSON object:" << obj;
            qDebug() << "✅ class_id:" << obj["class_id"].toString();
            qDebug() << "✅ lat:" << obj["lat"].toDouble();
            qDebug() << "✅ lon:" << obj["lon"].toDouble();
            list.append(m);
            qDebug() << "📍 Parsed GPS:" << m;
        }

        // Store data in member variable for QML access
        m_gpsList = list;
        emit gpsListChanged(); // Notify QML of property change
        qDebug() << "📤 Updated gpsList with" << list.size() << "items.";
    });

    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}
