#include "BoardingState.h"

#include <algorithm>
#include <cmath>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kBoardingStallTicks = 90;
    constexpr int kBoardingRetryTicks = 30;
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
        return std::nullopt;
    }

    EnsureBaseline(ctx);

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

    if (IsCargoPending(ctx) || !IsBarFull(ctx))
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
        ctx.menuGateway->CompleteBoarding();
    }
}

bool BoardingState::IsCargoPending(const TurnaroundContext& ctx)
{
    return ctx.gsxGateway->IsLoadingCargo() || ctx.gsxGateway->IsLoaderWaitingForDoor();
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
