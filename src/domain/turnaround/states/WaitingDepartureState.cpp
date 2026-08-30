#include "WaitingDepartureState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"

std::optional<TurnaroundTransition> WaitingDepartureState::EvaluatePhase(TurnaroundContext& ctx)
{
    const bool isAircraftOnGround = ctx.gsxGateway->IsAircraftOnGround();
    if (!isAircraftOnGround)
    {
        return TurnaroundTransition{TurnaroundPhase::OnFlight, 30};
    }

    return std::nullopt;
}
