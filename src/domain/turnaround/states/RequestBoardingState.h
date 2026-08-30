#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTBOARDINGSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTBOARDINGSTATE_H

#include "TurnaroundState.h"

class RequestBoardingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RequestBoarding;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTBOARDINGSTATE_H
