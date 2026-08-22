#include "WaitingReadyToPushState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingReadyToPushState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->IsReadyToPush() || !ctx.aircraft->IsParkingBrakeSet())
    {
        return std::nullopt;
    }

    if (ctx.aircraft->GetDoorStatus() == DoorStatus::AnyOpen)
    {
        return std::nullopt;
    }

    return TurnaroundTransition{TurnaroundPhase::WaitCatering};
}
