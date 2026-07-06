#include "GsxRemoteApiClient.h"

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QtWebSockets/QWebSocket>

#include "../logging/LogMacros.h"

namespace
{
    constexpr int kMaxBackoffMs = 15000;
    constexpr int kSupportedProtocol = 1;
}

GsxRemoteApiClient::GsxRemoteApiClient(QObject* parent) : QObject(parent)
{
    socket_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    reconnectTimer_ = new QTimer(this);
    reconnectTimer_->setSingleShot(true);

    connect(socket_, &QWebSocket::connected, this, &GsxRemoteApiClient::OnConnected);
    connect(socket_, &QWebSocket::disconnected, this, &GsxRemoteApiClient::OnDisconnected);
    connect(socket_, &QWebSocket::textMessageReceived, this, &GsxRemoteApiClient::OnTextMessage);
    connect(reconnectTimer_, &QTimer::timeout, this, &GsxRemoteApiClient::OnReconnect);
}

GsxRemoteApiClient::~GsxRemoteApiClient() = default;

void GsxRemoteApiClient::Start()
{
    stopping_ = false;
    OnReconnect();
}

void GsxRemoteApiClient::Stop()
{
    stopping_ = true;
    reconnectTimer_->stop();
    socket_->close();
}

void GsxRemoteApiClient::OnReconnect()
{
    port_ = ResolvePort();

    const QUrl url(QStringLiteral("ws://127.0.0.1:%1").arg(port_));

    LOG_INFO("GSX RemoteAPI: connecting to %s", url.toString().toUtf8().constData());

    socket_->open(url);
}

void GsxRemoteApiClient::OnConnected()
{
    connected_ = true;
    backoffMs_ = 1000;

    SendSubscribe();
}


void GsxRemoteApiClient::OnDisconnected()
{
    connected_ = false;

    ScheduleReconnect();
}

void GsxRemoteApiClient::ScheduleReconnect()
{
    if (stopping_)
    {
        return;
    }

    reconnectTimer_->start(backoffMs_);
    backoffMs_ = std::min(backoffMs_ * 2, kMaxBackoffMs);
}

void GsxRemoteApiClient::SendSubscribe()
{
    const QJsonObject sub{
        {"type", "subscribe"},
        {"channels", QJsonArray{"state", "prompts", "toasts"}},
    };

    socket_->sendTextMessage(QString::fromUtf8(QJsonDocument(sub).toJson(QJsonDocument::Compact)));
}

bool GsxRemoteApiClient::SendCommand(const QString& verb, const QJsonObject& args)
{
    if (!connected_)
    {
        LOG_WARN("GSX RemoteAPI: command '%s' dropped (offline)", verb.toUtf8().constData());
        return false;
    }

    QJsonObject cmd{{"type", "command"}, {"verb", verb}};

    if (!args.isEmpty()) cmd.insert("args", args);
    {
        const auto numBytes =
            socket_->sendTextMessage(QString::fromUtf8(QJsonDocument(cmd).toJson(QJsonDocument::Compact)));
        return numBytes != -1;
    }
}

void GsxRemoteApiClient::OnTextMessage(const QString& text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (!doc.isObject())
    {
        return;
    }

    const QJsonObject msg = doc.object();
    const QString type = msg.value("type").toString();

    if (type == "hello")
    {
        const int protocol = msg.value("protocol").toInt(kSupportedProtocol);
        if (protocol != kSupportedProtocol)
        {
            LOG_WARN("GSX RemoteAPI: protocol %d difere do suportado %d; leitura best-effort.",
                     protocol, kSupportedProtocol);
        }
    }
    else if (type == "snapshot")
    {
        emit SnapshotReceived(msg);
    }
    else if (type == "patch")
    {
        emit PatchReceived(msg.value("path").toString(), msg.value("value"));
    }
    else if (type == "result")
    {
        const bool ok = msg.value("ok").toBool();
        if (!ok)
        {
            const QJsonObject error = msg.value("error").toObject();
            const QString code = error.value("code").toString();
            LOG_WARN("GSX RemoteAPI: command rejected (%s): %s",
                     code.toUtf8().constData(),
                     error.value("message").toString().toUtf8().constData());
            emit ResultReceived(false, code);
        }
        else
        {
            emit ResultReceived(true, QString());
        }
    }
}

quint16 GsxRemoteApiClient::ResolvePort()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString ini = QDir(appData).filePath("../../Virtuali/CouatlAddons.ini");
    if (QFileInfo::exists(ini))
    {
        const QSettings cfg(ini, QSettings::IniFormat);
        const int p = cfg.value("gsx/remote_server_port", 0).toInt();
        if (p > 0 && p < 65536)
        {
            return static_cast<quint16>(p);
        }
    }

    return 8744;
}
