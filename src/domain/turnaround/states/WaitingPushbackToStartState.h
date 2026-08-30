#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_WAITPUSHBACKSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_WAITPUSHBACKSTATE_H

#include "TurnaroundState.h"

class WaitingPushbackToStartState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingPushbackToStart;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WAITPUSHBACKSTATE_H
