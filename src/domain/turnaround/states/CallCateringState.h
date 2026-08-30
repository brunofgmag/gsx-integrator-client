#ifndef GSX_INTEGRATOR_CLIENT_CALLCATERINGSTATE_H
#define GSX_INTEGRATOR_CLIENT_CALLCATERINGSTATE_H

#include "TurnaroundState.h"

class CallCateringState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::CallCatering;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    static bool RequestNextGroundService(TurnaroundContext& ctx);
    static bool DispatchCatering(TurnaroundContext& ctx);
};

#endif //GSX_INTEGRATOR_CLIENT_CALLCATERINGSTATE_H
