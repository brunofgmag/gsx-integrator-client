#include "RefuelingTrack.h"

#include <algorithm>
#include <cmath>
#include <format>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/DomainLogger.h"

namespace
{
    constexpr int kRefuelStallTicks = 60;
    constexpr double kFuelShortfallToleranceKg = 100.0;

    GsxStateStatus RefuelingStatus(const TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    }

    bool IsGsxRefuelDone(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        return refuelingState == GsxStateStatus::Completed
            || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    }

    bool IsGsxRefuelReady(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        if (!ctx.gsxGateway->IsRemoteApiConnected())
        {
            return false;
        }

        return ctx.gsxGateway->IsFuelHoseConnected()
            || refuelingState == GsxStateStatus::Active
            || IsGsxRefuelDone(ctx, refuelingState);
    }

    bool IsWeightDone(const TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        if (ctx.aircraft->GetRefuelMethod() != RefuelBy::Client)
        {
            return IsGsxRefuelDone(ctx, refuelingState);
        }

        return std::abs(ctx.data.plannedFuelKg - ctx.data.loadedFuelKg) <= turnaround::kWeightEpsilonKg;
    }

    double ApplyPumpedFuel(const double initialKg, const double plannedKg, const double pumpedKg)
    {
        return plannedKg >= initialKg
                   ? std::min(initialKg + pumpedKg, plannedKg)
                   : std::max(initialKg - pumpedKg, plannedKg);
    }

    void EnsureBaseline(TurnaroundContext& ctx)
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

    void NotifyLoadingStarted(TurnaroundContext& ctx)
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

    void RefuelProgressively(TurnaroundContext& ctx)
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

    void AccumulateFuel(TurnaroundContext& ctx)
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

    void MaybeForceCompletion(TurnaroundContext& ctx, const GsxStateStatus refuelingState)
    {
        auto& data = ctx.data;
        if (data.fuelProgress <= 95.0)
        {
            return;
        }

        if (IsGsxRefuelDone(ctx, refuelingState) || !ctx.gsxGateway->IsFuelHoseConnected())
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
            data.refuelStallTicks = 0;
            ctx.menuGateway->CompleteRefuel();
        }
    }

    void SnapToPlanned(TurnaroundContext& ctx)
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

    void WarnWhenFuelDidNotStay(TurnaroundContext& ctx)
    {
        auto& data = ctx.data;

        if (data.fuelStayChecked)
        {
            return;
        }

        const double settledKg = ctx.aircraft->GetCurrentFuelKg();
        if (settledKg <= 0.0)
        {
            return;
        }

        data.fuelStayChecked = true;
        data.settledFuelKg = settledKg;

        const double capacityKg = ctx.aircraft->GetFuelCapacityKg();
        const double writtenKg = capacityKg > 0.0
                                     ? std::min(data.plannedFuelKg, capacityKg)
                                     : data.plannedFuelKg;

        const double shortfallKg = writtenKg - settledKg;
        data.fuelDidNotStay = shortfallKg > kFuelShortfallToleranceKg;
        data.fuelShortfallKg = data.fuelDidNotStay ? shortfallKg : 0.0;

        if (data.fuelDidNotStay)
        {
            ctx.logger->LogInfo(std::format(
                "The client wrote {1:.0f} kg of fuel but the tanks hold {0:.0f} kg; {2:.0f} kg did not stay",
                settledKg, writtenKg, shortfallKg));
        }
    }
}

bool RefuelingTrack::HasFuelMoved(const TurnaroundContext& ctx)
{
    return ctx.data.refuelBaselined
        && std::abs(ctx.data.loadedFuelKg - ctx.data.initialFuelKg) > turnaround::kWeightEpsilonKg;
}

bool RefuelingTrack::IsInterrupted(const TurnaroundContext& ctx)
{
    const GsxStateStatus refuelingState = RefuelingStatus(ctx);

    return ctx.data.refuelBaselined
        && !IsGsxRefuelDone(ctx, refuelingState)
        && refuelingState < GsxStateStatus::Requested;
}

bool RefuelingTrack::Advance(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus refuelingState = RefuelingStatus(ctx);

    if (!IsGsxRefuelReady(ctx, refuelingState))
    {
        return false;
    }

    EnsureBaseline(ctx);
    NotifyLoadingStarted(ctx);

    if (data.fuelProgress < 100.0)
    {
        AccumulateFuel(ctx);
    }

    data.fuelProgress = turnaround::ProgressPercent(data.initialFuelKg, data.loadedFuelKg, data.plannedFuelKg);

    MaybeForceCompletion(ctx, refuelingState);

    if (!IsWeightDone(ctx, refuelingState) || ctx.gsxGateway->IsFuelHoseConnected())
    {
        return false;
    }

    SnapToPlanned(ctx);
    data.fuelProgress = 100.0;
    WarnWhenFuelDidNotStay(ctx);

    return IsGsxRefuelDone(ctx, refuelingState);
}
