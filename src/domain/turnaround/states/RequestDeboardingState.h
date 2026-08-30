#ifndef GSX_INTEGRATOR_CLIENT_REQUESTDEBOARDING_H
#define GSX_INTEGRATOR_CLIENT_REQUESTDEBOARDING_H

#include "TurnaroundState.h"

class RequestDeboardingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RequestDeboarding;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_REQUESTDEBOARDING_H
