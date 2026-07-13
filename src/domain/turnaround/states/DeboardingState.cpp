#include "DeboardingState.h"

#include <algorithm>
#include <cmath>

#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> DeboardingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus deboardingState = ctx.gsxGateway->GetStateStatus(GsxState::Deboarding);
    const bool isCompleted = deboardingState == GsxStateStatus::Completed
        || ctx.gsxGateway->WasStateCompleted(GsxState::Deboarding);
    if (deboardingState != GsxStateStatus::Active && !isCompleted)
    {
        return std::nullopt;
    }

    if (!data.deboardingBaselined)
    {
        data.deboardingBaselined = true;
        if (ctx.aircraft->BoardMethod() == BoardBy::Self)
        {
            ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);
        }
    }

    if (isCompleted)
    {
        data.loadedZfwKg = data.initialZfwKg;
        ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);
        data.deboardingProgress = 100.0;

        return TurnaroundTransition{TurnaroundPhase::WaitingNewFlight, 60};
    }

    AdvanceDeboardingBar(ctx);
    if (ctx.aircraft->BoardMethod() == BoardBy::Client)
    {
        ctx.aircraft->SetCurrentZfwKg(data.loadedZfwKg);
    }

    data.deboardingProgress = turnaround::ProgressPercent(
        data.plannedZfwKg,
        data.loadedZfwKg,
        data.initialZfwKg);

    return std::nullopt;
}

void DeboardingState::AdvanceDeboardingBar(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double cargoPercent = ctx.gsxGateway->GetDeboardingCargoPercent();
    const double safePassengers = data.plannedPassengers <= 0 ? 1.0 : static_cast<double>(data.plannedPassengers);
    const double passengerPercent = ctx.gsxGateway->GetDeboardedPassengers() / safePassengers * 100.0;

    const double progress = ctx.aircraft->IsCargoVariant()
                                ? cargoPercent
                                : std::abs((cargoPercent + passengerPercent) / 2.0);

    data.loadedZfwKg = std::clamp(
        data.plannedZfwKg - (data.plannedZfwKg - data.initialZfwKg) * (progress / 100.0),
        data.initialZfwKg,
        data.plannedZfwKg);
}
