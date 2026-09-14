#include "FakeGsxRemoteApiClient.h"

#include "../../src/infrastructure/gsx/GsxRemoteApiClient.h"

GsxRemoteApiClient::GsxRemoteApiClient(QObject* parent) : QObject(parent)
{
}

GsxRemoteApiClient::~GsxRemoteApiClient() = default;

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
