#include "FssEJetGpuFollowsRequestRule.h"

#include <QtCore/QString>

#include "../FssEJet.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/model/GroundPowerStatus.h"

namespace
{
    constexpr auto kRuleName = "fss-ejet-gpu-follows-request";

    constexpr auto kToggleLVar = "FSS_EXX_TOGGLE_CGPU";
    constexpr double kTogglePulse = 1.0;

    constexpr int kMinTicksBetweenPulses = 2;
    constexpr int kMaxAttempts = 3;
}

FssEJetGpuFollowsRequestRule::FssEJetGpuFollowsRequestRule(const FssEJet& aircraft)
    : aircraft_(&aircraft),
      ticksSincePulse_(kMinTicksBetweenPulses)
{
}

const char* FssEJetGpuFollowsRequestRule::Name() const
{
    return kRuleName;
}

RuleVerdict FssEJetGpuFollowsRequestRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FssEJetGpuFollowsRequestRule::ServeNewRequest()
{
    const int requests = aircraft_->GroundPowerRequests();
    if (requests == servedGroundPowerRequests_)
    {
        return;
    }

    servedGroundPowerRequests_ = requests;

    const std::optional<bool> requested = aircraft_->RequestedGroundPower();
    if (!requested.has_value() || desired_ == requested)
    {
        return;
    }

    desired_ = requested;
    attempts_ = 0;
    ticksSincePulse_ = kMinTicksBetweenPulses;
}

void FssEJetGpuFollowsRequestRule::Act(const RuleContext&, VariableWriter& writer)
{
    ServeNewRequest();

    if (!desired_.has_value())
    {
        return;
    }

    const std::optional<GroundPowerStatus> status = aircraft_->GetGroundPowerStatus();
    const GroundPowerStatus target = *desired_ ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;

    if (status == target)
    {
        desired_.reset();
        attempts_ = 0;

        return;
    }

    if (!status.has_value() || status == GroundPowerStatus::Unknown)
    {
        return;
    }

    ++ticksSincePulse_;
    if (ticksSincePulse_ < kMinTicksBetweenPulses || attempts_ >= kMaxAttempts)
    {
        return;
    }

    ticksSincePulse_ = 0;
    ++attempts_;

    probe::Line(probe::Channel::Writes, QStringLiteral("write gpu toggle request=%1").arg(*desired_ ? 1 : 0));
    writer.SetLVar(kToggleLVar, kTogglePulse);

    LOG_INFO("FSS E-Jet ground power toggle pulsed: requested %s, read %s",
             *desired_ ? "connected" : "disconnected",
             *status == GroundPowerStatus::Connected ? "connected" : "disconnected");
}
