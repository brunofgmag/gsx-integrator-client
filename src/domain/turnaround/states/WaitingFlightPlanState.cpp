#include "WaitingFlightPlanState.h"

#include <cmath>
#include <format>
#include <string>
#include "../TurnaroundContext.h"
#include "../TurnaroundMath.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../ports/DomainLogger.h"

namespace
{
    constexpr int kRetryTicks = 10;
}

std::optional<TurnaroundTransition> WaitingFlightPlanState::EvaluatePhase(TurnaroundContext& ctx)
{
    ctx.aircraft->SetCurrentZfwKg(ctx.aircraft->GetEmptyZfwKg());

    if (!ctx.aircraft->IsFlightPlanLoaded())
    {
        return std::nullopt;
    }

    bool& flightPlanRequested = ctx.data.flightPlanRequested;
    const bool simbriefLoaded = ctx.gsxGateway->IsSimbriefLoaded();

    if (!simbriefLoaded && !flightPlanRequested)
    {
        ctx.menuGateway->RequestSimbriefLoad();
        flightPlanRequested = true;
    }

    if (simbriefLoaded)
    {
        flightPlanRequested = false;
    }

    if (flightPlanRequested)
    {
        if (ctx.TickCondition(kRetryTicks))
        {
            if (const std::string refusal = ctx.gsxGateway->GetSimbriefRefusal(); !refusal.empty())
            {
                ctx.logger->LogInfo(std::format("GSX refused the SimBrief plan: {}", refusal));
            }
            else
            {
                ctx.logger->LogInfo(std::format("GSX Simbrief plan not loaded after {} seconds", kRetryTicks));
            }
            flightPlanRequested = false;
        }

        return std::nullopt;
    }

    CaptureFlightPlan(ctx);

    return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
}

void WaitingFlightPlanState::CaptureFlightPlan(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    data.plannedFuelKg = ctx.aircraft->GetPlannedFuelKg();
    data.plannedZfwKg = ctx.aircraft->GetPlannedZfwKg();
    data.plannedPassengers = ctx.aircraft->GetPlannedPassengers();
    data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
    data.initialFuelKg = data.loadedFuelKg;
    data.loadedZfwKg = ctx.aircraft->GetCurrentZfwKg();
    data.initialZfwKg = ctx.aircraft->GetEmptyZfwKg();

    ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);

    if (data.plannedPassengers == 0)
    {
        data.plannedPassengers = ctx.gsxGateway->GetPlannedPassengers();
    }

    NoteCrewLeftOutOfThePlan(ctx);
}

void WaitingFlightPlanState::NoteCrewLeftOutOfThePlan(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double crewKg = ctx.aircraft->GetCrewOnBoardKg();
    const double plannedOperatingEmptyKg = ctx.aircraft->GetPlannedOperatingEmptyKg();

    data.planOmitsCrew = crewKg > 0.0
        && std::abs(plannedOperatingEmptyKg - data.initialZfwKg) <= turnaround::kWeightEpsilonKg;
    data.omittedCrewKg = data.planOmitsCrew ? crewKg : 0.0;
    data.operatingEmptyWithCrewKg = data.planOmitsCrew ? plannedOperatingEmptyKg + crewKg : 0.0;

    if (data.planOmitsCrew)
    {
        ctx.logger->LogInfo(std::format(
            "The plan's operating empty weight of {:.0f} kg leaves out {:.0f} kg of crew; "
            "the ZFW target is {:.0f} kg, and SimBrief needs {:.0f} kg to count the crew",
            plannedOperatingEmptyKg, crewKg, data.plannedZfwKg, data.operatingEmptyWithCrewKg));
    }
}
