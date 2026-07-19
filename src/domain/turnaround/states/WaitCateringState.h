#ifndef GSX_INTEGRATOR_CLIENT_WAITCATERINGSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITCATERINGSTATE_H

#include "TurnaroundState.h"

class WaitCateringState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitCatering;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITCATERINGSTATE_H
