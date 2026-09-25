#include "LoadingState.h"

#include <cmath>
#include <format>
#include <string>
#include "BoardingTrack.h"
#include "RefuelingTrack.h"
#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kBoardingRetryTicks = 10;
    constexpr int kReadyToPushDelayTicks = 60;
    constexpr double kBoardingFuelPercent = 75.0;
    constexpr double kEarlyBoardingFuelPercent = 1.0;
    constexpr double kShortRefuelSeconds = 60.0;

    GsxStateStatus BoardingStatus(const TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->GetStateStatus(GsxState::Boarding);
    }

    bool HasRefuelEnded(const TurnaroundContext& ctx)
    {
        return ctx.data.refuelFinished || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling);
    }

    double FuelToLoadKg(const TurnaroundContext& ctx)
    {
        return std::abs(ctx.data.plannedFuelKg - ctx.data.initialFuelKg);
    }

    bool IsShortRefuel(const TurnaroundContext& ctx)
    {
        if (ctx.aircraft->GetRefuelMethod() != RefuelBy::Client && !RefuelingTrack::HasFuelMoved(ctx))
        {
            return false;
        }

        return FuelToLoadKg(ctx) <= kShortRefuelSeconds * ctx.settings->EffectiveFuelRateKgs();
    }

    double BoardingFuelPercent(const TurnaroundContext& ctx)
    {
        return ctx.settings->callBoardingEarly ? kEarlyBoardingFuelPercent : kBoardingFuelPercent;
    }

    std::string BoardingTrigger(const TurnaroundContext& ctx)
    {
        if (HasRefuelEnded(ctx))
        {
            return "the refueling ended";
        }

        if (ctx.data.fuelProgress >= BoardingFuelPercent(ctx))
        {
            return std::format("the refueling reached {:.0f}%", ctx.data.fuelProgress);
        }

        if (IsShortRefuel(ctx))
        {
            return std::format("only {:.0f} kg of fuel is loading, under {:.0f} s at {:.0f} kg/s",
                               FuelToLoadKg(ctx), kShortRefuelSeconds, ctx.settings->EffectiveFuelRateKgs());
        }

        return {};
    }

    void NoteBoardingConfirmation(TurnaroundContext& ctx)
    {
        if (BoardingStatus(ctx) >= GsxStateStatus::Requested
            || ctx.gsxGateway->WasStateCompleted(GsxState::Boarding))
        {
            ctx.data.boardingConfirmed = true;
        }
    }

    void LogWhenBoardingIsUnconfirmed(const TurnaroundContext& ctx)
    {
        if (ctx.data.boardingConfirmed)
        {
            return;
        }

        ctx.logger->LogInfo(std::format(
            "Loading: the refueling finished before GSX confirmed the boarding (boarding state {})",
            static_cast<int>(BoardingStatus(ctx))));
    }

    bool HasBoardingStarted(TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->GetBoardedPassengers() > 0 || ctx.gsxGateway->GetBoardingCargoPercent() > 0.0;
    }

    void RequestBoardingWhenDue(TurnaroundContext& ctx)
    {
        auto& data = ctx.data;

        if (data.boardingBaselined || data.boardingFinished || !data.loadingStartNotified)
        {
            return;
        }

        if (BoardingStatus(ctx) != GsxStateStatus::Callable)
        {
            return;
        }

        if (data.boardingRequested)
        {
            if (!HasBoardingStarted(ctx) && ctx.TickCondition(kBoardingRetryTicks))
            {
                data.boardingRequested = false;
            }

            return;
        }

        const std::string trigger = BoardingTrigger(ctx);
        if (trigger.empty())
        {
            return;
        }

        ctx.logger->LogInfo(std::format("Loading: requesting boarding because {}", trigger));
        ctx.menuGateway->RequestBoarding();
        data.boardingRequested = true;
    }
}

std::optional<TurnaroundTransition> LoadingState::EvaluatePhase(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const bool refuelDropped = !data.refuelFinished && RefuelingTrack::IsInterrupted(ctx);
    const bool boardingDropped = !data.boardingFinished && BoardingTrack::IsInterrupted(ctx);
    NoteServiceInterruption(ctx, refuelDropped ? "refueling" : "boarding", refuelDropped || boardingDropped);

    NoteBoardingConfirmation(ctx);

    if (!data.refuelFinished && RefuelingTrack::Advance(ctx))
    {
        data.refuelFinished = true;
        LogWhenBoardingIsUnconfirmed(ctx);
    }

    if (!data.boardingFinished)
    {
        data.boardingFinished = BoardingTrack::Advance(ctx);
    }

    RequestBoardingWhenDue(ctx);

    if (!data.refuelFinished || !data.boardingFinished)
    {
        return std::nullopt;
    }

    return TurnaroundTransition{TurnaroundPhase::WaitingReadyToPush, kReadyToPushDelayTicks};
}
