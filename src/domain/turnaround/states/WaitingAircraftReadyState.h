#ifndef GSX_INTEGRATOR_CLIENT_WAITINGAIRCRAFTREADYSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGAIRCRAFTREADYSTATE_H

#include "TurnaroundState.h"

class WaitingAircraftReadyState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingAircraftReady;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGAIRCRAFTREADYSTATE_H
