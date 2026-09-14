#include "BoardingState.h"

#include <algorithm>
#include <cmath>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kBoardingStallTicks = 90;
    constexpr int kBoardingRetryTicks = 30;
    constexpr int kLoaderDoorNoticeTicks = 30;
    constexpr int kLoaderDoorGiveUpTicks = 120;
}

std::optional<TurnaroundTransition> BoardingState::EvaluatePhase(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus boardingState = ctx.gsxGateway->GetStateStatus(GsxState::Boarding);
    const bool isCompleted = boardingState == GsxStateStatus::Completed
        || ctx.gsxGateway->WasStateCompleted(GsxState::Boarding);
    NoteServiceInterruption(ctx, "boarding", boardingState, data.boardingBaselined, isCompleted);

    if (boardingState != GsxStateStatus::Active && !isCompleted)
    {
        data.loaderHoldingBoarding = CargoLoader::None;

        return std::nullopt;
    }

    EnsureBaseline(ctx);
    NoteLoaderAwaitingDoor(ctx);

    if (isCompleted && !IsCargoPending(ctx))
    {
        FinishBoarding(ctx);
        return TurnaroundTransition{TurnaroundPhase::WaitingReadyToPush, 60};
    }

    data.boardedPassengers = ctx.gsxGateway->GetBoardedPassengers();

    AdvanceBoardingBar(ctx);
    if (ctx.aircraft->GetBoardMethod() == BoardBy::Client)
    {
        ctx.aircraft->SetCurrentZfwKg(data.loadedZfwKg);
    }

    data.boardingProgress = turnaround::ProgressPercent(
        data.initialZfwKg,
        data.loadedZfwKg,
        data.plannedZfwKg);

    if (IsCargoPending(ctx))
    {
        data.boardingProgress = std::min(data.boardingProgress, 99.0);
    }


    MaybeForceCompletion(ctx);

    return std::nullopt;
}

bool BoardingState::IsBarFull(const TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->GetBoardingCargoPercent() < 100.0)
    {
        return false;
    }

    return ctx.aircraft->IsCargoVariant()
        || ctx.data.boardedPassengers >= ctx.data.plannedPassengers;
}

void BoardingState::MaybeForceCompletion(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const bool heldBehindTheStairs = IsCargoHeldBehindTheStairs(ctx);
    const bool abandonedLoader = HasGivenUpOnTheLoader(ctx);
    const bool nothingToForce = IsCargoPending(ctx)
        || (!IsBarFull(ctx) && !heldBehindTheStairs && !abandonedLoader);

    if (nothingToForce)
    {
        data.boardingStallTicks = 0;
        data.boardingCompletionAttempts = 0;

        return;
    }

    const int ticksBeforeAsking = data.boardingCompletionAttempts == 0
                                      ? kBoardingStallTicks
                                      : kBoardingRetryTicks;

    if (++data.boardingStallTicks >= ticksBeforeAsking)
    {
        data.boardingStallTicks = 0;
        ++data.boardingCompletionAttempts;
        if (heldBehindTheStairs && data.boardingCompletionAttempts == 1)
        {
            ctx.logger->LogInfo(
                "Boarding: every passenger is aboard and the loaders are held behind the stairs; asking GSX to complete");
        }
        ctx.menuGateway->CompleteBoarding();
    }
}

bool BoardingState::IsCargoHeldBehindTheStairs(const TurnaroundContext& ctx)
{
    if (!ctx.menuGateway->WereStairsKeptForPassengers() || ctx.aircraft->IsCargoVariant())
    {
        return false;
    }

    const auto& data = ctx.data;

    return data.plannedPassengers > 0
        && data.boardedPassengers >= data.plannedPassengers
        && ctx.gsxGateway->GetBoardingCargoPercent() <= 0.0;
}

void BoardingState::NoteLoaderAwaitingDoor(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    const CargoLoader awaiting = ctx.gsxGateway->GetLoaderWaitingForDoor();

    if (awaiting != data.loaderAwaitingDoor)
    {
        data.loaderAwaitingDoor = awaiting;
        data.loaderDoorWaitTicks = 0;
        data.loaderHoldingBoarding = CargoLoader::None;
    }

    if (awaiting == CargoLoader::None)
    {
        return;
    }

    ++data.loaderDoorWaitTicks;
    if (data.loaderDoorWaitTicks >= kLoaderDoorNoticeTicks)
    {
        data.loaderHoldingBoarding = awaiting;
    }
}

bool BoardingState::HasGivenUpOnTheLoader(const TurnaroundContext& ctx)
{
    return ctx.data.loaderAwaitingDoor != CargoLoader::None
        && ctx.data.loaderDoorWaitTicks >= kLoaderDoorGiveUpTicks;
}

bool BoardingState::IsCargoPending(const TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsLoadingCargo())
    {
        return true;
    }

    return ctx.data.loaderAwaitingDoor != CargoLoader::None && !HasGivenUpOnTheLoader(ctx);
}

void BoardingState::EnsureBaseline(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.boardingBaselined)
    {
        return;
    }

    data.boardingBaselined = true;
    data.initialZfwKg = std::min(ctx.aircraft->GetEmptyZfwKg(), data.plannedZfwKg);
    if (ctx.aircraft->GetBoardMethod() == BoardBy::Self)
    {
        ctx.aircraft->SetCurrentZfwKg(data.plannedZfwKg);
    }
}

void BoardingState::FinishBoarding(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    data.boardedPassengers = data.plannedPassengers;
    data.loadedZfwKg = data.plannedZfwKg;
    ctx.aircraft->SetCurrentZfwKg(data.plannedZfwKg);
    data.boardingProgress = 100.0;
    ctx.aircraft->HoldDoorsClosed(true);
}

void BoardingState::AdvanceBoardingBar(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double cargoPercent = ctx.gsxGateway->GetBoardingCargoPercent();
    const double safePassengers = data.plannedPassengers <= 0 ? 1.0 : static_cast<double>(data.plannedPassengers);
    const double passengerPercent = data.boardedPassengers / safePassengers * 100.0;

    const double progress = ctx.aircraft->IsCargoVariant()
                                ? cargoPercent
                                : std::abs((cargoPercent + passengerPercent) / 2.0);

    data.loadedZfwKg = std::clamp(
        data.initialZfwKg + (data.plannedZfwKg - data.initialZfwKg) * (progress / 100.0),
        0.0,
        data.plannedZfwKg);
}
