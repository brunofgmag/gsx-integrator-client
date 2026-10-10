#include "SimbriefClient.h"

#include <format>
#include <string>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "SimbriefOfpParser.h"
#include "../../domain/model/AutomationStatus.h"
#include "../../domain/model/AutomationSettings.h"
#include "../../domain/model/PlanConversion.h"
#include "../logging/LogMacros.h"

namespace
{
    constexpr int kFirstSuccessStatus = 200;
    constexpr int kFirstRedirectStatus = 300;
    constexpr int kTransferTimeoutMs = 30000;

    int ReportedStatus(const bool networkFailed, const int httpStatus)
    {
        if (networkFailed || httpStatus > 0)
        {
            return httpStatus;
        }

        return kFirstSuccessStatus;
    }

    bool IsSuccessStatus(const int status)
    {
        return status >= kFirstSuccessStatus && status < kFirstRedirectStatus;
    }
}

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

        Fail(FlightPlanFailure::Http, lastError_);

        return;
    }

    const auto flightPlan = ParseSimbriefOfp(responseBody_);
    if (!flightPlan)
    {
        LOG_ERROR("Simbrief OFP parse failed (%zu bytes)", responseBody_.size());

        Fail(FlightPlanFailure::Parse, 0);

        return;
    }

    ApplyFlightPlan(*flightPlan);
}

bool SimbriefClient::HasHttpError() const
{
    return !IsSuccessStatus(lastError_);
}

void SimbriefClient::ApplyFlightPlan(const FlightPlan& flightPlan)
{
    turnaround::ApplyPlan(*automationStatus_, flightPlan);

    const std::string payloadText = flightPlan.payloadKg.has_value()
                                        ? std::format("{:.0f}kg", *flightPlan.payloadKg)
                                        : std::string("absent");

    LOG_INFO("SimBrief OFP loaded: fuel=%.0fkg zfw=%.0fkg oew=%.0fkg payload=%s pax=%d",
             flightPlan.fuelKg, flightPlan.zfwKg, flightPlan.operatingEmptyKg, payloadText.c_str(),
             flightPlan.passengers);

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

void SimbriefClient::Adopt(const FlightPlan& plan)
{
    Reset();
    ApplyFlightPlan(plan);
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

    network_.setTransferTimeout(kTransferTimeoutMs);

    const QUrl url(QStringLiteral("https://www.simbrief.com/api/xml.fetcher.php?userid=%1").arg(pilotId));
    reply_ = network_.get(QNetworkRequest(url));
    if (reply_ == nullptr)
    {
        LOG_ERROR("Failed to issue Simbrief HTTP request.");

        Fail(FlightPlanFailure::NotSent, 0);

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

    if (status_ != FlightPlanStatus::Error)
    {
        automationStatus_->flightPlanFailure = FlightPlanFailure::None;
        automationStatus_->flightPlanHttpStatus = 0;
    }
}

void SimbriefClient::Fail(const FlightPlanFailure failure, const int httpStatus)
{
    SetStatus(FlightPlanStatus::Error);

    automationStatus_->flightPlanFailure = failure;
    automationStatus_->flightPlanHttpStatus = httpStatus;
}

void SimbriefClient::OnHttpFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply == nullptr)
    {
        return;
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    lastError_ = ReportedStatus(reply->error() != QNetworkReply::NoError, httpStatus);
    responseBody_.clear();

    if (IsSuccessStatus(lastError_))
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
