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


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGFORSUPPORTEDAIRCRAFTSTATE_H
