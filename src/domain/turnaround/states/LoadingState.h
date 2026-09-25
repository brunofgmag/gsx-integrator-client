#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_LOADINGSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_LOADINGSTATE_H

#include "TurnaroundState.h"

class LoadingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Loading;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_LOADINGSTATE_H
