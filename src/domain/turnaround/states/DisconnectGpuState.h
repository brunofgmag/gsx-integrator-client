#ifndef GSX_INTEGRATOR_CLIENT_DISCONNECTGPUSTATE_H
#define GSX_INTEGRATOR_CLIENT_DISCONNECTGPUSTATE_H

#include "TurnaroundState.h"

class DisconnectGpuState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::DisconnectGpu;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_DISCONNECTGPUSTATE_H
