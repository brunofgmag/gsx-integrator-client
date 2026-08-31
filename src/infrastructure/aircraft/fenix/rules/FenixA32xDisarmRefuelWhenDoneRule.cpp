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

void FenixA32xDisarmRefuelWhenDoneRule::Act(const RuleContext& context, VariableWriter& writer)
{
    if (!context.needs.loading)
    {
        loadingSeen_ = false;

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

    if (!armed_ || variables_->GetLVar(gsx::lvars::kRefuelingState, 0.0)
        != static_cast<double>(GsxStateStatus::Completed))
    {
        return;
    }

    armed_ = false;
    writer.SetLVar(kThirdPartyRefuelLVar, kRefuelDisarmed);
}
