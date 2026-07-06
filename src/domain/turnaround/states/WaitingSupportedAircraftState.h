#ifndef GSX_INTEGRATOR_CLIENT_WAITINGFORSUPPORTEDAIRCRAFTSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGFORSUPPORTEDAIRCRAFTSTATE_H

#include "TurnaroundState.h"

class WaitingSupportedAircraftState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingSupportedAircraft;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGFORSUPPORTEDAIRCRAFTSTATE_H
