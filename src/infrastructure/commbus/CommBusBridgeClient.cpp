#include "CommBusBridgeClient.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kTxAreaName = "GSXI.CommBus.Tx";
    constexpr auto kRxAreaName = "GSXI.CommBus.Rx";
    constexpr SIMCONNECT_CLIENT_DATA_ID kTxAreaId = 0x47535801;
    constexpr SIMCONNECT_CLIENT_DATA_ID kRxAreaId = 0x47535802;
    constexpr SIMCONNECT_CLIENT_DATA_DEFINITION_ID kTxDefId = 0x47535803;
    constexpr SIMCONNECT_CLIENT_DATA_DEFINITION_ID kRxDefId = 0x47535804;
    constexpr SIMCONNECT_DATA_REQUEST_ID kRxRequestId = 0x47535805;
    constexpr SIMCONNECT_CLIENT_DATA_ID kReadyAreaId = 0x47535806;
    constexpr SIMCONNECT_CLIENT_DATA_DEFINITION_ID kReadyDefId = 0x47535807;
    constexpr SIMCONNECT_DATA_REQUEST_ID kReadyRequestId = 0x47535808;
    constexpr DWORD kAreaSize = 8192;

    constexpr auto kReadyAreaName = "GSXI.CommBus.Ready";
    constexpr double kMinProtocol = 2.0;

    constexpr auto kConnectionName = "GsxIntegratorCommBusBridge";

    std::string ToStd(const QString& value)
    {
        return value.toStdString();
    }
}

CommBusBridgeClient::CommBusBridgeClient() = default;

CommBusBridgeClient::~CommBusBridgeClient()
{
    session_.Close();
}

std::string CommBusBridgeClient::BuildCallEnvelope(const std::string& channel,
                                                   const int flag,
                                                   const std::string& payload)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("cmd"), QStringLiteral("call"));
    envelope.insert(QStringLiteral("channel"), QString::fromStdString(channel));
    envelope.insert(QStringLiteral("flag"), flag);
    envelope.insert(QStringLiteral("payload"), QString::fromStdString(payload));

    return ToStd(QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)));
}

std::string CommBusBridgeClient::BuildSubscribeEnvelope(const std::string& channel, const int flag)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("cmd"), QStringLiteral("subscribe"));
    envelope.insert(QStringLiteral("channel"), QString::fromStdString(channel));
    envelope.insert(QStringLiteral("flag"), flag);

    return ToStd(QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)));
}

std::string CommBusBridgeClient::BuildUnsubscribeEnvelope(const std::string& channel)
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("cmd"), QStringLiteral("unsubscribe"));
    envelope.insert(QStringLiteral("channel"), QString::fromStdString(channel));

    return ToStd(QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)));
}

bool CommBusBridgeClient::ParseInbound(const std::string& json,
                                       std::string& channelOut,
                                       std::string& payloadOut)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!document.isObject())
    {
        return false;
    }

    const QJsonObject object = document.object();
    const QJsonValue channel = object.value(QStringLiteral("channel"));
    if (!channel.isString())
    {
        return false;
    }

    channelOut = ToStd(channel.toString());
    payloadOut = ToStd(object.value(QStringLiteral("payload")).toString());

    return true;
}

void CommBusBridgeClient::Setup()
{
    if (connected_)
    {
        return;
    }

    if (!session_.Open(kConnectionName))
    {
        LOG_WARN("CommBus bridge: SimConnect open failed; toolbar and tablet features disabled");

        return;
    }

    const bool tx = session_.MapClientDataArea(kTxAreaName, kTxAreaId, kTxDefId, kAreaSize);
    const bool rx = session_.RequestClientDataArea(kRxAreaName, kRxAreaId, kRxDefId, kRxRequestId, kAreaSize,
                                                   SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME,
                                                   SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
                                                   [this](const void* data, const DWORD size)
                                                   {
                                                       if (data == nullptr || size == 0)
                                                       {
                                                           return;
                                                       }

                                                       const auto* bytes = static_cast<const char*>(data);
                                                       const std::size_t length = strnlen(bytes, size);
                                                       OnRxMessage(std::string(bytes, length));
                                                   });

    const bool ready = session_.RequestClientDataArea(kReadyAreaName, kReadyAreaId, kReadyDefId, kReadyRequestId,
                                                      kAreaSize,
                                                      SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME,
                                                      SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
                                                      [this](const void* data, const DWORD size)
                                                      {
                                                          OnReadyData(data, size);
                                                      });

    connected_ = tx && rx && ready;
    if (!connected_)
    {
        session_.Close();

        return;
    }

    for (const auto& [channel, subscription] : handlers_)
    {
        SendEnvelope(BuildSubscribeEnvelope(channel, subscription.flag));
    }
}

void CommBusBridgeClient::Shutdown()
{
    session_.Close();
    connected_ = false;
    available_ = false;
}

void CommBusBridgeClient::Poll()
{
    session_.Dispatch();
}

bool CommBusBridgeClient::IsAvailable() const
{
    return available_;
}

void CommBusBridgeClient::OnReadyData(const void* data, const DWORD size)
{
    if (available_ || data == nullptr || size < sizeof(double))
    {
        return;
    }

    double value = 0.0;
    std::memcpy(&value, data, sizeof(double));
    if (value >= kMinProtocol)
    {
        available_ = true;
        LOG_INFO("CommBus bridge available (protocol %d)", static_cast<int>(value));
    }
}

void CommBusBridgeClient::SendEnvelope(const std::string& envelope) const
{
    if (!connected_)
    {
        return;
    }

    if (envelope.size() > kAreaSize - 1)
    {
        LOG_WARN("CommBus bridge: dropping oversize envelope (%zu bytes)", envelope.size());

        return;
    }

    std::vector buffer(kAreaSize, '\0');
    std::memcpy(buffer.data(), envelope.data(), envelope.size());

    session_.WriteClientData(kTxAreaId, kTxDefId, kAreaSize, buffer.data());
}

void CommBusBridgeClient::Call(const std::string& channel, const int flag, const std::string& payload)
{
    if (!IsAvailable())
    {
        return;
    }

    SendEnvelope(BuildCallEnvelope(channel, flag, payload));
}

void CommBusBridgeClient::Subscribe(const std::string& channel, const int flag, Handler handler)
{
    handlers_[channel] = {flag, std::move(handler)};
    SendEnvelope(BuildSubscribeEnvelope(channel, flag));
}

void CommBusBridgeClient::Unsubscribe(const std::string& channel)
{
    if (handlers_.erase(channel) > 0)
    {
        SendEnvelope(BuildUnsubscribeEnvelope(channel));
    }
}

void CommBusBridgeClient::OnRxMessage(const std::string& json)
{
    std::string channel;
    std::string payload;
    if (!ParseInbound(json, channel, payload))
    {
        return;
    }

    if (const auto it = handlers_.find(channel); it != handlers_.end() && it->second.handler)
    {
        it->second.handler(payload);
    }
}
