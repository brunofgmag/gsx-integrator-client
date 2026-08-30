#ifndef GSX_INTEGRATOR_CLIENT_REPOSITIONAIRCRAFTSTATE_H
#define GSX_INTEGRATOR_CLIENT_REPOSITIONAIRCRAFTSTATE_H

#include "TurnaroundState.h"

class RepositionAircraftState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RepositionAircraft;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_REPOSITIONAIRCRAFTSTATE_H
