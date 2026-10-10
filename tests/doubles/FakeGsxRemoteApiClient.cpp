#include "FakeGsxRemoteApiClient.h"

#include "../../src/infrastructure/gsx/GsxRemoteApiClient.h"

void FakeGsxRemoteApi::AnnounceConnection(const bool connected)
{
    connectionUp = connected;
    if (liveClient != nullptr)
    {
        emit liveClient->ConnectionChanged(connected);
    }
}

void FakeGsxRemoteApi::Receive(const QJsonObject& message)
{
    if (liveClient == nullptr)
    {
        return;
    }

    if (!connectionUp)
    {
        AnnounceConnection(true);
    }

    const QString type = message.value("type").toString();
    if (type == "snapshot")
    {
        emit liveClient->SnapshotReceived(message);
    }
    else if (type == "patch")
    {
        emit liveClient->PatchReceived(message.value("path").toString(), message.value("value"));
    }
}

GsxRemoteApiClient::GsxRemoteApiClient(QObject* parent) : QObject(parent)
{
    FakeGsxRemoteApi::connectionUp = false;
    FakeGsxRemoteApi::liveClient = this;
}

GsxRemoteApiClient::~GsxRemoteApiClient()
{
    if (FakeGsxRemoteApi::liveClient == this)
    {
        FakeGsxRemoteApi::liveClient = nullptr;
    }
}

void GsxRemoteApiClient::Start()
{
    ++FakeGsxRemoteApi::startCalls;
}

void GsxRemoteApiClient::Stop()
{
    ++FakeGsxRemoteApi::stopCalls;
}

bool GsxRemoteApiClient::SendCommand(const QString& verb, const QJsonObject&)
{
    FakeGsxRemoteApi::commandVerbs.push_back(verb.toStdString());

    return !FakeGsxRemoteApi::refuseSends;
}

void GsxRemoteApiClient::OnConnected()
{
}

void GsxRemoteApiClient::OnDisconnected()
{
}

void GsxRemoteApiClient::OnTextMessage(const QString&)
{
}

void GsxRemoteApiClient::OnReconnect()
{
}

void GsxRemoteApiClient::OnHandshakeTimeout()
{
}
