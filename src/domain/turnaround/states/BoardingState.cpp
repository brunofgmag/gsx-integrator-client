#include "BoardingState.h"

#include <algorithm>
#include <cmath>
#include "../TurnaroundMath.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"

std::optional<TurnaroundTransition> BoardingState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus boardingState = ctx.gsxGateway->GetStateStatus(GsxState::Boarding);
    const bool isCompleted = boardingState == GsxStateStatus::Completed
        || ctx.gsxGateway->WasStateCompleted(GsxState::Boarding);
    if (boardingState != GsxStateStatus::Active && !isCompleted)
    {
        return std::nullopt;
    }

    if (!data.boardingBaselined)
    {
        data.boardingBaselined = true;
        data.initialZfwKg = ctx.aircraft->GetEmptyZfwKg();
        if (ctx.aircraft->BoardMethod() == BoardBy::Self)
        {
            ctx.aircraft->SetCurrentZfwKg(data.plannedZfwKg);
        }
    }

    if (isCompleted)
    {
        data.boardedPassengers = data.plannedPassengers;
        data.loadedZfwKg = data.plannedZfwKg;
        if (ctx.aircraft->BoardMethod() != BoardBy::Gsx)
        {
            ctx.aircraft->SetCurrentZfwKg(data.plannedZfwKg);
        }
        data.boardingProgress = 100.0;

        return TurnaroundTransition{TurnaroundPhase::WaitingReadyToPush, 60};
    }

    data.boardedPassengers = ctx.gsxGateway->GetBoardedPassengers();

    AdvanceBoardingBar(ctx);
    if (ctx.aircraft->BoardMethod() == BoardBy::Client)
    {
        ctx.aircraft->SetCurrentZfwKg(data.loadedZfwKg);
    }

    data.boardingProgress = turnaround::ProgressPercent(
        data.initialZfwKg,
        data.loadedZfwKg,
        data.plannedZfwKg);

    return std::nullopt;
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
