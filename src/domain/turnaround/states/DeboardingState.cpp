#include "DeboardingState.h"

#include <algorithm>
#include <cmath>

#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    double DeboardedPassengersPercent(const TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->GetDeboardedPassengers() / static_cast<double>(ctx.data.plannedPassengers) * 100.0;
    }
}

std::optional<TurnaroundTransition> DeboardingState::EvaluatePhase(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus deboardingState = ctx.gsxGateway->GetStateStatus(GsxState::Deboarding);
    const bool isCompleted = deboardingState == GsxStateStatus::Completed
        || ctx.gsxGateway->WasStateCompleted(GsxState::Deboarding);
    NoteServiceInterruption(ctx, "deboarding", deboardingState, data.deboardingBaselined, isCompleted);

    if (deboardingState != GsxStateStatus::Active && !isCompleted)
    {
        return std::nullopt;
    }

    EnsureBaseline(ctx);

    if (isCompleted)
    {
        FinishDeboarding(ctx);
        return TurnaroundTransition{TurnaroundPhase::CabinServices};
    }

    data.deboardedPassengers = ctx.gsxGateway->GetDeboardedPassengers();

    AdvanceDeboardingBar(ctx);
    if (ctx.aircraft->GetBoardMethod() == BoardBy::Client)
    {
        ctx.aircraft->SetCurrentZfwKg(data.loadedZfwKg);
    }

    data.deboardingProgress = turnaround::ProgressPercent(
        data.plannedZfwKg,
        data.loadedZfwKg,
        data.initialZfwKg);

    return std::nullopt;
}

void DeboardingState::EnsureBaseline(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.deboardingBaselined)
    {
        return;
    }

    data.deboardingBaselined = true;
    AdoptTheLoadOnBoardWithoutAPlan(ctx);
    if (ctx.aircraft->GetBoardMethod() == BoardBy::Self)
    {
        ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);
    }
}

void DeboardingState::AdoptTheLoadOnBoardWithoutAPlan(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (data.plannedZfwKg > 0.0)
    {
        return;
    }

    data.plannedZfwKg = ctx.aircraft->GetCurrentZfwKg();
    data.initialZfwKg = std::min(ctx.aircraft->GetEmptyZfwKg(), data.plannedZfwKg);
    data.plannedPassengers = ctx.gsxGateway->GetPlannedPassengers();
}

void DeboardingState::FinishDeboarding(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    data.deboardedPassengers = data.plannedPassengers;
    data.loadedZfwKg = data.initialZfwKg;
    ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);
    data.deboardingProgress = 100.0;
}

void DeboardingState::AdvanceDeboardingBar(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double cargoPercent = ctx.gsxGateway->GetDeboardingCargoPercent();
    const bool countsPassengers = !ctx.aircraft->IsCargoVariant() && data.plannedPassengers > 0;

    const double progress = countsPassengers
                                ? std::abs((cargoPercent + DeboardedPassengersPercent(ctx)) / 2.0)
                                : cargoPercent;

    data.loadedZfwKg = std::clamp(
        data.plannedZfwKg - ((data.plannedZfwKg - data.initialZfwKg) * (progress / 100.0)),
        data.initialZfwKg,
        data.plannedZfwKg);
}
