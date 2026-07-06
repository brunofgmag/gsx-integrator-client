#include "RefuelingState.h"

#include <algorithm>
#include <cmath>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../model/AutomationSettings.h"

std::optional<TurnaroundTransition> RefuelingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    const bool gsxReady = ctx.gsxGateway->IsFuelHoseConnected()
        || refuelingState == GsxStateStatus::Active
        || refuelingState == GsxStateStatus::Completed;
    if (!gsxReady)
    {
        return std::nullopt;
    }

    if (data.fuelProgress < 100.0)
    {
        if (ctx.aircraft->SupportsProgressiveFuel())
        {
            RefuelProgressively(ctx);
        }
        else
        {
            data.loadedFuelKg = data.plannedFuelKg;
            ctx.aircraft->SetCurrentFuelKg(data.plannedFuelKg);
        }
    }

    data.fuelProgress = turnaround::ProgressPercent(
        data.initialFuelKg,
        data.loadedFuelKg,
        data.plannedFuelKg);

    const bool weightDone = std::abs(data.plannedFuelKg - data.loadedFuelKg) <= turnaround::kWeightEpsilonKg;
    if (!weightDone || ctx.gsxGateway->IsFuelHoseConnected())
    {
        return std::nullopt;
    }

    data.loadedFuelKg = data.plannedFuelKg;
    ctx.aircraft->SetCurrentFuelKg(data.plannedFuelKg);
    data.fuelProgress = 100.0;

    const bool isCompleted = ctx.gsxGateway->GetStateStatus(GsxState::Refueling) == GsxStateStatus::Completed;
    const bool wasCompleted = ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    if (isCompleted || wasCompleted)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestBoarding, 30};
    }

    return std::nullopt;
}

void RefuelingState::RefuelProgressively(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double step = ctx.settings->EffectiveFuelRateKgs() * turnaround::kTickSeconds;
    if (data.loadedFuelKg < data.plannedFuelKg)
    {
        data.loadedFuelKg = std::min(data.plannedFuelKg, data.loadedFuelKg + step);
    }
    else if (data.loadedFuelKg > data.plannedFuelKg)
    {
        data.loadedFuelKg = std::max(data.plannedFuelKg, data.loadedFuelKg - step);
    }

    ctx.aircraft->SetCurrentFuelKg(data.loadedFuelKg);
}
