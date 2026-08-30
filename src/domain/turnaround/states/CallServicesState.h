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


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    static std::optional<TurnaroundTransition> ResolveJetwayOrStairs(TurnaroundContext& ctx);
};

#endif //GSX_INTEGRATOR_CLIENT_CALLSERVICESSTATE_H
