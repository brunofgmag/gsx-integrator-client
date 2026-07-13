#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H

#include "TurnaroundState.h"

class BoardingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Boarding;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static void AdvanceBoardingBar(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H
