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

    bool IsGsxRefuelReady(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        return ctx.gsxGateway->IsFuelHoseConnected()
            || refuelingState == GsxStateStatus::Active
            || refuelingState == GsxStateStatus::Completed
            || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    }

    bool IsWeightDone(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        if (ctx.aircraft->GetRefuelMethod() != RefuelBy::Client)
        {
            return refuelingState == GsxStateStatus::Completed
                || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
        }

        return std::abs(ctx.data.plannedFuelKg - ctx.data.loadedFuelKg) <= turnaround::kWeightEpsilonKg;
    }

    bool IsRefuelCompleted(const TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->GetStateStatus(GsxState::Refueling) == GsxStateStatus::Completed
            || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    }

    double ApplyPumpedFuel(const double initialKg, const double plannedKg, const double pumpedKg)
    {
        return plannedKg >= initialKg
                   ? std::min(initialKg + pumpedKg, plannedKg)
                   : std::max(initialKg - pumpedKg, plannedKg);
    }
}

std::optional<TurnaroundTransition> RefuelingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    if (!IsGsxRefuelReady(ctx, refuelingState))
    {
        return std::nullopt;
    }

    EnsureBaseline(ctx);
    NotifyLoadingStarted(ctx);

    if (data.fuelProgress < 100.0)
    {
        AccumulateFuel(ctx);
    }

    data.fuelProgress = turnaround::ProgressPercent(data.initialFuelKg, data.loadedFuelKg, data.plannedFuelKg);

    MaybeForceCompletion(ctx);

    if (!IsWeightDone(ctx, refuelingState) || ctx.gsxGateway->IsFuelHoseConnected())
    {
        return std::nullopt;
    }

    SnapToPlanned(ctx);
    data.fuelProgress = 100.0;

    if (IsRefuelCompleted(ctx))
    {
        return TurnaroundTransition{TurnaroundPhase::RequestBoarding, 30};
    }

    return std::nullopt;
}

void RefuelingState::EnsureBaseline(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.refuelBaselined)
    {
        return;
    }

    data.refuelBaselined = true;
    data.initialFuelKg = ctx.aircraft->GetCurrentFuelKg();
    data.loadedFuelKg = data.initialFuelKg;
}

void RefuelingState::NotifyLoadingStarted(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.loadingStartNotified)
    {
        return;
    }

    data.loadingStartNotified = true;
    ctx.aircraft->OnLoadingStarted();
    if (ctx.aircraft->GetRefuelMethod() == RefuelBy::Self)
    {
        ctx.aircraft->SetCurrentFuelKg(data.plannedFuelKg);
    }
}

void RefuelingState::AccumulateFuel(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    switch (ctx.aircraft->GetRefuelMethod())
    {
    case RefuelBy::Gsx:
        data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
        break;
    case RefuelBy::Self:
        {
            const double pumpedKg =
                ctx.gsxGateway->GetRefuelCounterGallons() * turnaround::kJetFuelKgPerUsGallon;
            if (pumpedKg > 0.0)
            {
                data.loadedFuelKg = ApplyPumpedFuel(data.initialFuelKg, data.plannedFuelKg, pumpedKg);
            }
            break;
        }
    case RefuelBy::Client:
        RefuelProgressively(ctx);
        break;
    }
}

void RefuelingState::MaybeForceCompletion(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.fuelProgress <= 95.0 || data.refuelCompletionForced)
    {
        return;
    }

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

void RefuelingState::SnapToPlanned(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    switch (ctx.aircraft->GetRefuelMethod())
    {
    case RefuelBy::Gsx:
        data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
        break;
    case RefuelBy::Self:
        data.loadedFuelKg = data.plannedFuelKg;
        break;
    case RefuelBy::Client:
        data.loadedFuelKg = data.plannedFuelKg;
        ctx.aircraft->SetCurrentFuelKg(data.plannedFuelKg);
        break;
    }
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
