#include "PmdgTabletClient.h"

#include <algorithm>
#include <array>
#include <utility>
#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include "../commbus/CommBusBridgeGateway.h"
#include "../probe/ProbeLog.h"

namespace
{
    constexpr auto kChannelToPlane = "TabletToPlane";
    constexpr auto kChannelToTablet = "PlaneToTablet";

    constexpr auto kTagWbPayload = "wb_payload";
    constexpr auto kTagGroundConn = "ground_conn";
    constexpr auto kTagSimbriefFetchResult = "simbrief_fetch_result";
    constexpr auto kTagQueryState = "query_state";
    constexpr auto kTagStateReply = "state_reply";
    constexpr auto kTagPing = "ping";
    constexpr auto kDoorActionClose = "CLOSE";
    constexpr auto kGroundConnState = "ground_conn";
    constexpr auto kPassengerEntryField = "passenger_entry";
    constexpr auto kPassengerEntryJetway = "JETWAY";
    constexpr std::array kDoorMoving = {"OPENING", "CLOSING"};

    std::string BuildEnvelope(const char* tag, const QJsonObject& data)
    {
        QJsonObject envelope;
        envelope.insert(QStringLiteral("data"), data);
        envelope.insert(QStringLiteral("message_tag"), QString::fromLatin1(tag));
        envelope.insert(QStringLiteral("tablet_side"), QStringLiteral("CA"));

        return QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)).toStdString();
    }
}

PmdgTabletClient::PmdgTabletClient(CommBusBridgeGateway* bridge) : bridge_(bridge)
{
}

PmdgTabletClient::PmdgTabletClient(std::unique_ptr<CommBusBridgeGateway> bridge)
    : ownedBridge_(std::move(bridge)),
      bridge_(ownedBridge_.get())
{
}

PmdgTabletClient::~PmdgTabletClient()
{
    if (subscribed_ && ownedBridge_ == nullptr)
    {
        bridge_->Unsubscribe(kChannelToTablet);
    }
}

std::string PmdgTabletClient::BuildWbPayload(const std::string& field, const int value)
{
    QJsonObject data;
    data.insert(QString::fromStdString(field), value);

    return BuildEnvelope(kTagWbPayload, data);
}

std::string PmdgTabletClient::BuildGroundConn(const std::string& key)
{
    QJsonObject data;
    data.insert(QString::fromStdString(key), 1);

    return BuildEnvelope(kTagGroundConn, data);
}

void PmdgTabletClient::Poll()
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

    MaybeProbePress();
}

void PmdgTabletClient::MaybeProbePress()
{
    if (probePressSent_ || !probe::IsOn() || !IsAvailable())
    {
        return;
    }

    const QString conn = qEnvironmentVariable("GSXI_PROBE_GROUND_CONN");
    if (conn.isEmpty())
    {
        return;
    }

    probePressSent_ = true;
    probe::Line(QStringLiteral("probe pmdg tablet pressing %1").arg(conn));
    SendToPlane(BuildGroundConn(conn.toStdString()));
}

bool PmdgTabletClient::IsAvailable() const
{
    return bridge_->IsAvailable();
}

bool PmdgTabletClient::EfbPlanImported() const
{
    return efbPlanImported_;
}

PmdgTabletClient::DoorSnapshot PmdgTabletClient::ParseDoorStates(const std::string& json)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!document.isObject())
    {
        return {};
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("message_tag")).toString() != QLatin1String(kTagStateReply))
    {
        return {};
    }

    const QJsonObject doors = object.value(QStringLiteral("doors")).toObject()
                                    .value(QStringLiteral("individual_doors")).toObject();

    DoorSnapshot snapshot;
    const auto collect = [&snapshot](const QJsonObject& source)
    {
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
        {
            if (!it.value().isString())
            {
                continue;
            }

            const QString action = it.value().toString();
            if (std::ranges::any_of(kDoorMoving, [&action](const char* moving)
                                    { return action == QLatin1String(moving); }))
            {
                snapshot.moving.emplace(it.key().toStdString());

                continue;
            }

            snapshot.settled.emplace(it.key().toStdString(), action == QLatin1String(kDoorActionClose));
        }
    };

    collect(doors);
    collect(doors.value(QStringLiteral("other_doors")).toObject());

    return snapshot;
}

std::optional<bool> PmdgTabletClient::ParsePassengerEntry(const std::string& json)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!document.isObject())
    {
        return std::nullopt;
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("message_tag")).toString() != QLatin1String(kTagStateReply))
    {
        return std::nullopt;
    }

    const QJsonValue entry = object.value(QLatin1String(kGroundConnState)).toObject()
                                   .value(QLatin1String(kPassengerEntryField));
    if (!entry.isString())
    {
        return std::nullopt;
    }

    return entry.toString() == QLatin1String(kPassengerEntryJetway);
}

bool PmdgTabletClient::IsSimbriefFetchSuccess(const std::string& json)
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

void PmdgTabletClient::ReportProbe(const std::string& payload)
{
    if (!probe::IsOn())
    {
        return;
    }

    const QString tag = QJsonDocument::fromJson(QByteArray::fromStdString(payload))
                        .object().value(QStringLiteral("message_tag")).toString();
    if (tag.isEmpty() || tag == QLatin1String(kTagPing))
    {
        return;
    }

    probe::Change("efb." + tag.toStdString(),
                  QStringLiteral("efb   %1").arg(QString::fromStdString(payload)));
}

void PmdgTabletClient::OnInbound(const std::string& payload)
{
    ReportProbe(payload);

    if (IsSimbriefFetchSuccess(payload))
    {
        efbPlanImported_ = true;
    }

    if (const std::optional<bool> viaJetway = ParsePassengerEntry(payload); viaJetway.has_value())
    {
        passengerEntryJetway_ = viaJetway;
    }

    if (auto doors = ParseDoorStates(payload); !doors.settled.empty() || !doors.moving.empty())
    {
        doorOpen_ = std::move(doors.settled);
        doorMoving_ = std::move(doors.moving);
    }
}

bool PmdgTabletClient::DoorMoving(const std::string& key) const
{
    return doorMoving_.contains(key);
}

std::optional<bool> PmdgTabletClient::PassengerEntryViaJetway() const
{
    return passengerEntryJetway_;
}

std::optional<bool> PmdgTabletClient::DoorOpen(const std::string& key) const
{
    const auto it = doorOpen_.find(key);

    return it == doorOpen_.end() ? std::nullopt : std::optional(it->second);
}

void PmdgTabletClient::RequestState()
{
    if (!IsAvailable())
    {
        return;
    }

    QJsonObject data;
    data.insert(QStringLiteral("request"), QStringLiteral("yes"));

    SendToPlane(BuildEnvelope(kTagQueryState, data));
}

void PmdgTabletClient::SendToPlane(const std::string& payload) const
{
    if (probe::IsOn())
    {
        const QString tag = QJsonDocument::fromJson(QByteArray::fromStdString(payload))
                            .object().value(QStringLiteral("message_tag")).toString();
        if (tag != QLatin1String(kTagQueryState))
        {
            probe::Line(QStringLiteral("write efb   %1").arg(QString::fromStdString(payload)));
        }
    }

    bridge_->Call(kChannelToPlane, CommBusFlag::kWasm, payload);
}

void PmdgTabletClient::SendWbPayload(const std::string& field, const int value) const
{
    if (!IsAvailable())
    {
        return;
    }

    SendToPlane(BuildWbPayload(field, value));
}

void PmdgTabletClient::SendFuelTotalLbs(const int lbs)
{
    SendWbPayload("fuel_total_lbs", lbs);
}

void PmdgTabletClient::SendPaxTotal(const int count)
{
    SendWbPayload("pax_count_total", count);
}

void PmdgTabletClient::SendCargoTotalLbs(const int lbs)
{
    SendWbPayload("cargo_weight_total", lbs);
}

void PmdgTabletClient::RequestGroundConn(const std::string& key)
{
    if (!IsAvailable())
    {
        return;
    }

    SendToPlane(BuildGroundConn(key));
}
