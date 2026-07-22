#include "Pmdg777TabletClient.h"

#include <utility>
#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include "../commbus/CommBusBridgeGateway.h"

namespace
{
    constexpr auto kChannelToPlane = "TabletToPlane";
    constexpr auto kChannelToTablet = "PlaneToTablet";

    constexpr auto kTagWbPayload = "wb_payload";
    constexpr auto kTagGroundConn = "ground_conn";
    constexpr auto kTagSimbriefFetchResult = "simbrief_fetch_result";

    std::string BuildEnvelope(const char* tag, const QJsonObject& data)
    {
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), data);
        envelope.insert(QStringLiteral("message_tag"), QString::fromLatin1(tag));
        envelope.insert(QStringLiteral("tablet_side"), QStringLiteral("CA"));

        return QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)).toStdString();
    }
}

Pmdg777TabletClient::Pmdg777TabletClient(CommBusBridgeGateway* bridge) : bridge_(bridge)
{
}

Pmdg777TabletClient::Pmdg777TabletClient(std::unique_ptr<CommBusBridgeGateway> bridge)
    : ownedBridge_(std::move(bridge)),
      bridge_(ownedBridge_.get())
{
}

Pmdg777TabletClient::~Pmdg777TabletClient()
{
    if (subscribed_ && ownedBridge_ == nullptr)
    {
        bridge_->Unsubscribe(kChannelToTablet);
    }
}

std::string Pmdg777TabletClient::BuildWbPayload(const std::string& field, const int value)
{
    QJsonObject data;
    data.insert(QString::fromStdString(field), value);

    return BuildEnvelope(kTagWbPayload, data);
}

std::string Pmdg777TabletClient::BuildGroundConn(const std::string& key)
{
    QJsonObject data;
    data.insert(QString::fromStdString(key), 1);

    return BuildEnvelope(kTagGroundConn, data);
}

void Pmdg777TabletClient::Poll()
{
    if (ownedBridge_ != nullptr)
    {
        if (!bridgeSetup_)
        {
            ownedBridge_->Setup();
            bridgeSetup_ = true;
        }
        ownedBridge_->Poll();
    }

    if (!subscribed_)
    {
        bridge_->Subscribe(kChannelToTablet, CommBusFlag::kJs,
                           [this](const std::string& payload) { OnInbound(payload); });
        subscribed_ = true;
    }
}

bool Pmdg777TabletClient::IsAvailable() const
{
    return bridge_->IsAvailable();
}

bool Pmdg777TabletClient::EfbPlanImported() const
{
    return efbPlanImported_;
}

bool Pmdg777TabletClient::IsSimbriefFetchSuccess(const std::string& json)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!document.isObject())
    {
        return false;
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("message_tag")).toString() != QLatin1String(kTagSimbriefFetchResult))
    {
        return false;
    }

    return object.value(QStringLiteral("data")).toObject()
                 .value(QStringLiteral("result")).toVariant().toString() == QLatin1String("200");
}

void Pmdg777TabletClient::OnInbound(const std::string& payload)
{
    if (IsSimbriefFetchSuccess(payload))
    {
        efbPlanImported_ = true;
    }
}

void Pmdg777TabletClient::SendWbPayload(const std::string& field, const int value) const
{
    if (!IsAvailable())
    {
        return;
    }

    bridge_->Call(kChannelToPlane, CommBusFlag::kWasm, BuildWbPayload(field, value));
}

void Pmdg777TabletClient::SendFuelTotalLbs(const int lbs)
{
    SendWbPayload("fuel_total_lbs", lbs);
}

void Pmdg777TabletClient::SendPaxTotal(const int count)
{
    SendWbPayload("pax_count_total", count);
}

void Pmdg777TabletClient::SendCargoTotalLbs(const int lbs)
{
    SendWbPayload("cargo_weight_total", lbs);
}

void Pmdg777TabletClient::RequestGroundConn(const std::string& key)
{
    if (!IsAvailable())
    {
        return;
    }

    bridge_->Call(kChannelToPlane, CommBusFlag::kWasm, BuildGroundConn(key));
}
