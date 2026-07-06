#include "OnFlightState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"

std::optional<TurnaroundTransition> OnFlightState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsAircraftOnGround())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingEngineShutdown};
    }

    return std::nullopt;
}
