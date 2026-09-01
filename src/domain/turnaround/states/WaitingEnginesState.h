#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORENGINESSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORENGINESSTATE_H

#include "TurnaroundState.h"

class WaitingEnginesState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingForEngines;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORENGINESSTATE_H
