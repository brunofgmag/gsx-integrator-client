#include "WaitingReadyToPushState.h"

#include "../PilotUnlock.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingReadyToPushState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->IsReadyToPush() || !ctx.aircraft->IsParkingBrakeSet())
    {
        return std::nullopt;
    }

    if (ctx.aircraft->GetDoorStatus() == DoorStatus::AnyOpen)
    {
        if (!PilotUnlock::Accepts(Phase()) || !ctx.ConsumePilotTouch())
        {
            return std::nullopt;
        }

        return TurnaroundTransition{TurnaroundPhase::WaitCatering, 0, TransitionOrigin::Pilot};
    }

    return TurnaroundTransition{TurnaroundPhase::WaitCatering};
}
