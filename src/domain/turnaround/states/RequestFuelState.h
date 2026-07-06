#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H

#include "TurnaroundState.h"

class RequestFuelState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RequestFuel;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H
