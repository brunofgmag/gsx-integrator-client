#include "RequestBoardingState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RequestBoardingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus boardingState = ctx.gsxGateway->GetStateStatus(GsxState::Boarding);
    if (boardingState == GsxStateStatus::Callable && !data.boardingRequested)
    {
        data.boardingRequested = ctx.menuGateway->RequestBoarding();
    }

    const bool hasPassengersBoarding = ctx.gsxGateway->GetBoardedPassengers() > 0;
    const bool hasCargoBoarding = ctx.gsxGateway->GetBoardingCargoPercent() > 0.0;
    const bool hasBoardingStarted = ctx.aircraft->IsCargoVariant() ? hasCargoBoarding : hasPassengersBoarding;

    const bool gsxReady = boardingState == GsxStateStatus::Active;
    if (hasBoardingStarted && gsxReady)
    {
        return TurnaroundTransition{TurnaroundPhase::Boarding};
    }

    if (boardingState == GsxStateStatus::Completed || ctx.gsxGateway->WasStateCompleted(GsxState::Boarding))
    {
        return TurnaroundTransition{TurnaroundPhase::Boarding};
    }

    const bool departureCallable = boardingState == GsxStateStatus::Callable;
    if (departureCallable && !hasBoardingStarted && data.boardingRequested
        && ctx.TickCondition(kRetryTicks))
    {
        data.boardingRequested = false;
    }

    return std::nullopt;
}
