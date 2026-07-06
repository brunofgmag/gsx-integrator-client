#ifndef GSX_INTEGRATOR_CLIENT_WAITINGDEPARTURESTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGDEPARTURESTATE_H

#include "TurnaroundState.h"

class WaitingDepartureState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingDeparture;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGDEPARTURESTATE_H
