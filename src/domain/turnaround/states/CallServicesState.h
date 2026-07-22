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
    static std::optional<TurnaroundTransition> ResolveJetwayOrStairs(TurnaroundContext& ctx);
    static void RegisterJetwayOrStairsRequest(TurnaroundContext& ctx, bool requested);
};

#endif //GSX_INTEGRATOR_CLIENT_CALLSERVICESSTATE_H
