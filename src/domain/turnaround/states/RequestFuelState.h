#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H

#include "TurnaroundState.h"

class RequestFuelState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RequestFuel;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    static void WarnWhenPlanExceedsCapacity(TurnaroundContext& ctx);
    static void TrackStalledRequest(TurnaroundContext& ctx, bool gsxStillOffersService);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTFUELSTATE_H
