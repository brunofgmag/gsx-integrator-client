#ifndef GSX_INTEGRATOR_CLIENT_WAITINGPOWERONSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGPOWERONSTATE_H

#include "TurnaroundState.h"

class WaitingPowerOnState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingPowerOn;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGPOWERONSTATE_H
