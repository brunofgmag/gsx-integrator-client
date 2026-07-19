#include "RequestDeboardingState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RequestDeboardingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus deboardingState = ctx.gsxGateway->GetStateStatus(GsxState::Deboarding);
    if (ctx.aircraft->IsReadyToDeboard()
        && deboardingState == GsxStateStatus::Callable && !ctx.data.deboardingRequested
        && ctx.menuGateway->IsMenuSettled())
    {
        ctx.data.deboardingRequested = ctx.menuGateway->RequestDeboarding();
    }

    const bool hasPassengersDeboarding = ctx.gsxGateway->GetDeboardedPassengers() > 0;
    const bool hasCargoDeboarding = ctx.gsxGateway->GetDeboardingCargoPercent() > 0.0;
    const bool hasDeboardingStarted = hasPassengersDeboarding || hasCargoDeboarding;

    const bool gsxReady = deboardingState == GsxStateStatus::Active;
    if (hasDeboardingStarted && gsxReady)
    {
        return TurnaroundTransition{TurnaroundPhase::Deboarding};
    }

    if (deboardingState == GsxStateStatus::Completed || ctx.gsxGateway->WasStateCompleted(GsxState::Deboarding))
    {
        return TurnaroundTransition{TurnaroundPhase::Deboarding};
    }

    const bool deboardingCallable = deboardingState == GsxStateStatus::Callable;
    if (deboardingCallable && !hasDeboardingStarted && data.deboardingRequested
        && ctx.TickCondition(kRetryTicks))
    {
        data.deboardingRequested = false;
    }

    return std::nullopt;
}
