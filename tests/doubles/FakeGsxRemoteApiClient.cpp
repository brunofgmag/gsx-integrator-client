#include "FakeGsxRemoteApiClient.h"

#include "../../src/infrastructure/gsx/GsxRemoteApiClient.h"

void FakeGsxRemoteApi::AnnounceConnection(const bool connected)
{
    if (liveClient != nullptr)
    {
        emit liveClient->ConnectionChanged(connected);
    }
}

GsxRemoteApiClient::GsxRemoteApiClient(QObject* parent) : QObject(parent)
{
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

    return false;
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
