#include "FenixA32xDisarmRefuelWhenDoneRule.h"

#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/ports/GsxGateway.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-disarm-refuel-when-done";

    constexpr auto kThirdPartyRefuelLVar = "S_THIRD_PARTY_REFUELG";
    constexpr double kRefuelArmed = 1.0;
    constexpr double kRefuelDisarmed = 0.0;

    constexpr int kConsecutiveCallableTicksBeforeTheRefuelCountsAsEnded = 60;
}

FenixA32xDisarmRefuelWhenDoneRule::FenixA32xDisarmRefuelWhenDoneRule(VariableReader& variables)
    : variables_(&variables)
{
}

const char* FenixA32xDisarmRefuelWhenDoneRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xDisarmRefuelWhenDoneRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xDisarmRefuelWhenDoneRule::ResumeLoading(const bool refuelFinished, const bool gsxRestartedSinceSave)
{
    loadingSeen_ = true;
    armed_ = true;
    resumed_ = true;
    refuelFinishedBeforeTheResume_ = refuelFinished;
    gsxRestartedSinceSave_ = gsxRestartedSinceSave;
    callableTicks_ = 0;
}

void FenixA32xDisarmRefuelWhenDoneRule::Act(const RuleContext& context, VariableWriter& writer)
{
    if (!context.needs.loading)
    {
        loadingSeen_ = false;
        resumed_ = false;
        callableTicks_ = 0;

        return;
    }

    if (!loadingSeen_)
    {
        loadingSeen_ = true;
        armed_ = true;
        writer.SetLVar(kThirdPartyRefuelLVar, kRefuelArmed);

        LOG_INFO("Fenix third-party refueling armed: GSX can connect the fuel hose");

        return;
    }

    if (armed_ && resumed_)
    {
        CountTheCallableReading();
    }

    if (!armed_ || !RefuelHasEnded())
    {
        return;
    }

    const std::optional<bool> hoseArmed = resumed_ ? ReadTheHoseArmed() : std::optional<bool>{true};
    if (!hoseArmed.has_value())
    {
        return;
    }

    armed_ = false;

    if (*hoseArmed)
    {
        writer.SetLVar(kThirdPartyRefuelLVar, kRefuelDisarmed);
    }
}

bool FenixA32xDisarmRefuelWhenDoneRule::RefuelHasEnded() const
{
    if (variables_->GetLVar(gsx::lvars::kRefuelingState, 0.0) == static_cast<double>(GsxStateStatus::Completed))
    {
        return true;
    }

    return resumed_ && (refuelFinishedBeforeTheResume_ || GsxLeftTheFinishedServiceBehind());
}

bool FenixA32xDisarmRefuelWhenDoneRule::GsxLeftTheFinishedServiceBehind() const
{
    return !gsxRestartedSinceSave_
        && callableTicks_ >= kConsecutiveCallableTicksBeforeTheRefuelCountsAsEnded;
}

bool FenixA32xDisarmRefuelWhenDoneRule::RefuelServiceReadsCallable() const
{
    return variables_->HasReceivedLVar(gsx::lvars::kRefuelingState)
        && variables_->GetLVar(gsx::lvars::kRefuelingState, 0.0) == static_cast<double>(GsxStateStatus::Callable);
}

void FenixA32xDisarmRefuelWhenDoneRule::CountTheCallableReading()
{
    callableTicks_ = RefuelServiceReadsCallable() ? callableTicks_ + 1 : 0;
}

std::optional<bool> FenixA32xDisarmRefuelWhenDoneRule::ReadTheHoseArmed() const
{
    if (!variables_->HasReceivedLVar(kThirdPartyRefuelLVar))
    {
        return std::nullopt;
    }

    return variables_->GetLVar(kThirdPartyRefuelLVar, kRefuelDisarmed) > kRefuelDisarmed;
}
