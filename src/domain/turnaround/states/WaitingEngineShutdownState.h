#ifndef GSX_INTEGRATOR_CLIENT_WAITINGENGINESHUTDOWNSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGENGINESHUTDOWNSTATE_H

#include "TurnaroundState.h"

class WaitingEngineShutdownState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingEngineShutdown;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGENGINESHUTDOWNSTATE_H
