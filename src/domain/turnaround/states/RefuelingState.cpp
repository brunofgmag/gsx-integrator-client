#include "RefuelingState.h"

#include <algorithm>
#include <cmath>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../model/AutomationSettings.h"

namespace
{
    constexpr int kRefuelStallTicks = 60;

    bool IsWeightDone(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        if (ctx.aircraft->IsRefueledExternally() || ctx.aircraft->LoadsViaUplink())
        {
            return refuelingState == GsxStateStatus::Completed
                || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
        }

        return std::abs(ctx.data.plannedFuelKg - ctx.data.loadedFuelKg) <= turnaround::kWeightEpsilonKg;
    }
}

std::optional<TurnaroundTransition> RefuelingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    const bool gsxReady = ctx.gsxGateway->IsFuelHoseConnected()
        || refuelingState == GsxStateStatus::Active
        || refuelingState == GsxStateStatus::Completed
        || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    if (!gsxReady)
    {
        return std::nullopt;
    }

    if (!data.refuelBaselined)
    {
        data.refuelBaselined = true;
        data.initialFuelKg = ctx.aircraft->GetCurrentFuelKg();
        data.loadedFuelKg = data.initialFuelKg;
    }

    if (!data.loadingStartNotified)
    {
        data.loadingStartNotified = true;
        ctx.aircraft->OnLoadingStarted();
    }

    if (data.fuelProgress < 100.0)
    {
        if (ctx.aircraft->IsRefueledExternally())
        {
            data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
        }
        else if (ctx.aircraft->LoadsViaUplink())
        {
            const double pumpedKg =
                ctx.gsxGateway->GetRefuelCounterGallons() * turnaround::kJetFuelKgPerUsGallon;
            if (pumpedKg > 0.0)
            {
                data.loadedFuelKg = data.plannedFuelKg >= data.initialFuelKg
                                        ? std::min(data.initialFuelKg + pumpedKg, data.plannedFuelKg)
                                        : std::max(data.initialFuelKg - pumpedKg, data.plannedFuelKg);
            }
        }
        else if (ctx.aircraft->SupportsProgressiveFuel())
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

    if (data.fuelProgress > 95.0 && !data.refuelCompletionForced)
    {
        if (std::abs(data.loadedFuelKg - data.refuelStallSampleKg) > turnaround::kWeightEpsilonKg)
        {
            data.refuelStallSampleKg = data.loadedFuelKg;
            data.refuelStallTicks = 0;
        }
        else if (++data.refuelStallTicks >= kRefuelStallTicks)
        {
            data.refuelCompletionForced = true;
            (void)ctx.menuGateway->CompleteRefuel();
        }
    }

    if (!IsWeightDone(ctx, refuelingState) || ctx.gsxGateway->IsFuelHoseConnected())
    {
        return std::nullopt;
    }

    if (ctx.aircraft->IsRefueledExternally())
    {
        data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
    }
    else if (ctx.aircraft->LoadsViaUplink())
    {
        data.loadedFuelKg = data.plannedFuelKg;
    }
    else
    {
        data.loadedFuelKg = data.plannedFuelKg;
        ctx.aircraft->SetCurrentFuelKg(data.plannedFuelKg);
    }

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
