#include "FslabsGroundClient.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"

namespace
{
    constexpr auto kReadBaseUrl = "http://127.0.0.1:8080";
    constexpr auto kCommandBaseUrl = "http://127.0.0.1:23032";
    constexpr auto kGroundPath = "/Maintenance/Ground_Get";
    constexpr auto kToggleHoldPathFormat = "/EFB-API/SET-PAGE-GROUND?%1=1";
    constexpr auto kValueField = "Value";
    constexpr auto kForwardHoldField = "RH_FWD_CARGO";
    constexpr auto kAftHoldField = "RH_AFT_CARGO";
    constexpr int kTransferTimeoutMs = 2000;
    constexpr int kPollFailuresBeforeUnavailable = 2;
    constexpr int kNotAHoldState = -1;
    constexpr int kLastHoldState = static_cast<int>(FslabsHoldState::Closing);

    const char* FieldOf(const FslabsHold hold)
    {
        return hold == FslabsHold::Forward ? kForwardHoldField : kAftHoldField;
    }

    std::optional<FslabsHoldState> HoldStateOf(const QJsonValue& field)
    {
        const int raw = field.toInt(kNotAHoldState);
        if (raw < static_cast<int>(FslabsHoldState::Closed) || raw > kLastHoldState)
        {
            return std::nullopt;
        }

        return static_cast<FslabsHoldState>(raw);
    }

    QUrl ToggleHoldUrl(const QString& baseUrl, const FslabsHold hold)
    {
        return QUrl(baseUrl + QString::fromLatin1(kToggleHoldPathFormat).arg(QLatin1String(FieldOf(hold))));
    }

    std::optional<FslabsGroundReading> ReadingIn(QNetworkReply& reply)
    {
        if (reply.error() != QNetworkReply::NoError)
        {
            return std::nullopt;
        }

        return FslabsGroundClient::ParseGroundResponse(reply.readAll());
    }
}

FslabsGroundClient::FslabsGroundClient()
    : readUrl_(QString::fromLatin1(kReadBaseUrl)),
      commandUrl_(QString::fromLatin1(kCommandBaseUrl))
{
}

FslabsGroundClient::~FslabsGroundClient() = default;

std::optional<FslabsGroundReading> FslabsGroundClient::ParseGroundResponse(const QByteArray& body)
{
    const QJsonObject fields = QJsonDocument::fromJson(body).object().value(QLatin1String(kValueField)).toObject();
    const std::optional<FslabsHoldState> forwardHold = HoldStateOf(fields.value(QLatin1String(kForwardHoldField)));
    const std::optional<FslabsHoldState> aftHold = HoldStateOf(fields.value(QLatin1String(kAftHoldField)));
    if (!forwardHold.has_value() || !aftHold.has_value())
    {
        return std::nullopt;
    }

    return FslabsGroundReading{*forwardHold, *aftHold};
}

void FslabsGroundClient::Poll()
{
    if (groundReply_ != nullptr)
    {
        return;
    }

    QNetworkAccessManager& network = EnsureNetwork();
    groundReply_ = network.get(QNetworkRequest(QUrl(readUrl_ + QString::fromLatin1(kGroundPath))));
    if (groundReply_ == nullptr)
    {
        RegisterPollResult(false);

        return;
    }

    QObject::connect(groundReply_, &QNetworkReply::finished, &network, [this] { OnGroundFinished(); });
}

void FslabsGroundClient::OnGroundFinished()
{
    QNetworkReply* reply = groundReply_;
    groundReply_ = nullptr;
    reply->deleteLater();

    const std::optional<FslabsGroundReading> reading = ReadingIn(*reply);
    if (!reading.has_value())
    {
        RegisterPollResult(false);

        return;
    }

    reading_ = reading;
    RegisterPollResult(true);
}

void FslabsGroundClient::RegisterPollResult(const bool succeeded)
{
    if (succeeded)
    {
        if (!available_)
        {
            LOG_INFO("FSLabs ground endpoint reachable: answering at %s", qUtf8Printable(readUrl_));
        }

        consecutivePollFailures_ = 0;
        available_ = true;

        return;
    }

    ++consecutivePollFailures_;
    if (available_ && consecutivePollFailures_ >= kPollFailuresBeforeUnavailable)
    {
        available_ = false;

        LOG_WARN("FSLabs ground endpoint unreachable: waiting until it answers again");
    }
}

void FslabsGroundClient::ToggleHold(const FslabsHold hold)
{
    const QNetworkReply* reply = EnsureNetwork().get(QNetworkRequest(ToggleHoldUrl(commandUrl_, hold)));
    if (reply != nullptr)
    {
        QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }

    ReportToggle(hold);
}

void FslabsGroundClient::ReportToggle(const FslabsHold hold) const
{
    if (!probe::IsOn())
    {
        return;
    }

    probe::Line(probe::Channel::Writes, QStringLiteral("ground %1 toggle").arg(QLatin1String(FieldOf(hold))));
}

bool FslabsGroundClient::IsAvailable() const
{
    return available_;
}

std::optional<FslabsHoldState> FslabsGroundClient::HoldState(const FslabsHold hold) const
{
    if (!reading_.has_value())
    {
        return std::nullopt;
    }

    return hold == FslabsHold::Forward ? reading_->forwardHold : reading_->aftHold;
}

QNetworkAccessManager& FslabsGroundClient::EnsureNetwork()
{
    if (network_ == nullptr)
    {
        network_ = std::make_unique<QNetworkAccessManager>();
        network_->setTransferTimeout(kTransferTimeoutMs);
    }

    return *network_;
}
