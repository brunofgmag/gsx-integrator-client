#ifndef GSX_INTEGRATOR_CLIENT_WAITINGREADYTOPUSHSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGREADYTOPUSHSTATE_H

#include "TurnaroundState.h"

class WaitingReadyToPushState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingReadyToPush;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGREADYTOPUSHSTATE_H
