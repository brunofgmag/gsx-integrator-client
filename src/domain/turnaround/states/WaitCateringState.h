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


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITCATERINGSTATE_H
