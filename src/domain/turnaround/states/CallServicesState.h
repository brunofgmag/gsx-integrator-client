#ifndef GSX_INTEGRATOR_CLIENT_CALLSERVICESSTATE_H
#define GSX_INTEGRATOR_CLIENT_CALLSERVICESSTATE_H

#include "TurnaroundState.h"

class CallServicesState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::CallServices;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static bool RequestNextGroundService(TurnaroundContext& ctx);
};

#endif //GSX_INTEGRATOR_CLIENT_CALLSERVICESSTATE_H
