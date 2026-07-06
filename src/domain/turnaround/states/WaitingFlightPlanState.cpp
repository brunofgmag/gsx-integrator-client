#include "WaitingFlightPlanState.h"

#include <format>
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../ports/DomainLogger.h"

namespace
{
    constexpr int kRetryTicks = 10;
}

std::optional<TurnaroundTransition> WaitingFlightPlanState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->IsFlightPlanLoaded())
    {
        return std::nullopt;
    }

    bool& flightPlanRequested = ctx.data.flightPlanRequested;

    if (!ctx.gsxGateway->IsSimbriefLoaded() && !flightPlanRequested)
    {
        flightPlanRequested = ctx.menuGateway->RequestSimbriefLoad();
    }

    if (ctx.gsxGateway->IsSimbriefLoaded())
    {
        flightPlanRequested = false;
    }

    if (flightPlanRequested && !ctx.gsxGateway->IsSimbriefLoaded())
    {
        if (ctx.TickCondition(kRetryTicks))
        {
            ctx.logger->LogInfo(std::format("GSX Simbrief plan not loaded after {} seconds", kRetryTicks));
            flightPlanRequested = false;
        }

        return std::nullopt;
    }

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

    return TurnaroundTransition{TurnaroundPhase::RepositionAircraft};
}
