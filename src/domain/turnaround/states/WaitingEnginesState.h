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

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(
        TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORENGINESSTATE_H
