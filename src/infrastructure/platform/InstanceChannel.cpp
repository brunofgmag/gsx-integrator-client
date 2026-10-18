#include "InstanceChannel.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtNetwork/QLocalSocket>

namespace
{
    constexpr auto kChannelName = "gsx-integrator-client.single-instance";
    constexpr auto kShowCommand = "show";

    void ReadCommands(QLocalSocket* socket, InstanceServer* server)
    {
        while (socket->canReadLine())
        {
            const QList<QByteArray> words = socket->readLine().trimmed().split(' ');
            if (words.first() == QByteArray(kShowCommand))
            {
                emit server->ShowRequested(words.size() > 1 ? QString::fromUtf8(words.at(1)) : QString());
            }
        }
    }
}

QString InstanceChannelName()
{
    return QLatin1String(kChannelName);
}

bool NotifyRunningInstance(const QString& name, const bool show, const QString& activationToken, const int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(name);
    if (!socket.waitForConnected(timeoutMs))
    {
        return false;
    }

    if (show)
    {
        QByteArray command(kShowCommand);
        if (!activationToken.isEmpty())
        {
            command += ' ' + activationToken.toUtf8();
        }

        socket.write(command + '\n');
        socket.waitForBytesWritten(timeoutMs);
    }

    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState)
    {
        socket.waitForDisconnected(timeoutMs);
    }

    return true;
}

InstanceServer::InstanceServer(QObject* parent)
    : QObject(parent)
{
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&server_, &QLocalServer::newConnection, this, &InstanceServer::OnNewConnection);
}

bool InstanceServer::Listen(const QString& name)
{
    return server_.listen(name);
}

void InstanceServer::OnNewConnection()
{
    while (QLocalSocket* socket = server_.nextPendingConnection())
    {
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { ReadCommands(socket, this); });
        ReadCommands(socket, this);
    }
}
