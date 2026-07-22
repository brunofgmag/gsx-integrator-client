#include "SimbriefClient.h"

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "SimbriefOfpParser.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/AutomationSettings.h"
#include "../logging/LogMacros.h"

SimbriefClient::SimbriefClient(AutomationStatus* status,
                               const AutomationSettings* settings,
                               QObject* parent)
    : QObject(parent), automationStatus_(status), settings_(settings)
{
}

void SimbriefClient::Poll()
{
    if (status_ == FlightPlanStatus::Idle)
    {
        FetchData();
        return;
    }

    if (status_ != FlightPlanStatus::Fetching || !pending_)
    {
        return;
    }

    pending_ = false;

    if (HasHttpError())
    {
        LOG_ERROR("Simbrief fetch failed, error code %d", lastError_);

        SetStatus(FlightPlanStatus::Error);

        return;
    }

    const auto flightPlan = ParseSimbriefOfp(responseBody_);
    if (!flightPlan)
    {
        LOG_ERROR("Simbrief OFP parse failed (%zu bytes)", responseBody_.size());

        SetStatus(FlightPlanStatus::Error);

        return;
    }

    ApplyFlightPlan(*flightPlan);
}

bool SimbriefClient::HasHttpError() const
{
    return lastError_ < 200 || lastError_ >= 300;
}

void SimbriefClient::ApplyFlightPlan(const FlightPlan& flightPlan)
{
    automationStatus_->plannedFuelKg = flightPlan.fuelKg;
    automationStatus_->plannedZfwKg = flightPlan.zfwKg;
    automationStatus_->plannedPassengers = flightPlan.passengers;
    automationStatus_->simbriefUnit = flightPlan.unit;

    LOG_INFO("SimBrief OFP loaded: fuel=%.0fkg zfw=%.0fkg pax=%d",
             flightPlan.fuelKg, flightPlan.zfwKg, flightPlan.passengers);

    SetStatus(FlightPlanStatus::Ready);
}

void SimbriefClient::Reset()
{
    if (reply_ != nullptr)
    {
        (void)reply_->disconnect(this);
        reply_->abort();
        reply_->deleteLater();
        reply_ = nullptr;
    }

    ClearResponse();
    SetStatus(FlightPlanStatus::Idle);
}

void SimbriefClient::ClearResponse()
{
    pending_ = false;
    lastError_ = 0;
    responseBody_.clear();
}

bool SimbriefClient::Reload()
{
    Reset();
    return FetchData();
}

bool SimbriefClient::FetchData()
{
    const int pilotId = settings_->simbriefPilotId;
    if (pilotId <= 0)
    {
        return false;
    }

    ClearResponse();

    network_.setTransferTimeout(30000);

    const QUrl url(QStringLiteral("https://www.simbrief.com/api/xml.fetcher.php?userid=%1").arg(pilotId));
    reply_ = network_.get(QNetworkRequest(url));
    if (reply_ == nullptr)
    {
        LOG_ERROR("Failed to issue Simbrief HTTP request.");

        SetStatus(FlightPlanStatus::Error);

        return false;
    }

    connect(reply_, &QNetworkReply::finished, this, &SimbriefClient::OnHttpFinished);

    LOG_INFO("Fetching Simbrief OFP for pilot id %d", pilotId);
    SetStatus(FlightPlanStatus::Fetching);

    return true;
}

void SimbriefClient::SetStatus(const FlightPlanStatus status)
{
    status_ = status;
    automationStatus_->flightPlanStatus = status_;
}

void SimbriefClient::OnHttpFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply == nullptr)
    {
        return;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    lastError_ = (reply->error() == QNetworkReply::NoError) ? (httpStatus > 0 ? httpStatus : 200) : httpStatus;
    responseBody_.clear();

    if (lastError_ >= 200 && lastError_ < 300)
    {
        const QByteArray body = reply->readAll();
        responseBody_.assign(body.constData(), static_cast<std::size_t>(body.size()));
    }

    pending_ = true;

    reply->deleteLater();
    if (reply_ == reply)
    {
        reply_ = nullptr;
    }
}
