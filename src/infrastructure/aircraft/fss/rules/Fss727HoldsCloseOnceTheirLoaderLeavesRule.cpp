#include "Fss727HoldsCloseOnceTheirLoaderLeavesRule.h"

#include <array>
#include <cstddef>
#include <QtCore/QString>

#include "../Fss727.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-holds-close-once-their-loader-leaves";

    constexpr auto kPercentOver100Unit = "percent over 100";
    constexpr double kHoldGoalClosed = 0.0;

    struct Hold
    {
        const char* name;
        const char* goal;
        const char* loaderLVar;
    };

    constexpr std::array kHolds = {
        Hold{"forward", "INTERACTIVE POINT GOAL:2", gsx::lvars::kBaggageLoaderFrontState},
        Hold{"aft", "INTERACTIVE POINT GOAL:3", gsx::lvars::kBaggageLoaderRearState}
    };
}

Fss727HoldsCloseOnceTheirLoaderLeavesRule::Fss727HoldsCloseOnceTheirLoaderLeavesRule(
    VariableReader& variables, const Fss727& aircraft, const GsxDoorSync& doors)
    : variables_(&variables), aircraft_(&aircraft), doors_(&doors)
{
}

const char* Fss727HoldsCloseOnceTheirLoaderLeavesRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727HoldsCloseOnceTheirLoaderLeavesRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727HoldsCloseOnceTheirLoaderLeavesRule::Act(const RuleContext&, VariableWriter& writer)
{
    const int requests = aircraft_->HoldCloseRequests();

    for (std::size_t index = 0; index < kHolds.size(); ++index)
    {
        const Hold& hold = kHolds[index];
        int& served = servedRequests_[index];

        if (served == requests || !HasItsLoaderLeft(hold.loaderLVar))
        {
            continue;
        }

        probe::Line(probe::Channel::Writes, QStringLiteral("write hold %1 %2=0").arg(QLatin1String(hold.name), QLatin1String(hold.goal)));
        writer.SetAVar(hold.goal, kPercentOver100Unit, kHoldGoalClosed);
        served = requests;

        LOG_INFO("FSS 727 %s hold commanded closed: its loader has left", hold.name);
    }
}

bool Fss727HoldsCloseOnceTheirLoaderLeavesRule::HasItsLoaderLeft(const char* loaderLVar) const
{
    return variables_->HasReceivedLVar(loaderLVar)
        && doors_->VehicleState(loaderLVar, 0.0) < gsx::states::kVehicleDispatched;
}
