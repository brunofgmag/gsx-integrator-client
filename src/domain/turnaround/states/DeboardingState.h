#ifndef GSX_INTEGRATOR_CLIENT_DEBOARDINGSTATE_H
#define GSX_INTEGRATOR_CLIENT_DEBOARDINGSTATE_H

#include "TurnaroundState.h"

class DeboardingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Deboarding;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static void DeboardProgressively(TurnaroundContext& ctx);
};

#endif //GSX_INTEGRATOR_CLIENT_DEBOARDINGSTATE_H
