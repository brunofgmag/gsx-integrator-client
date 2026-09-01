#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORFLIGHTPLANSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORFLIGHTPLANSTATE_H

#include "TurnaroundState.h"

class WaitingFlightPlanState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingFlightPlan;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    static void CaptureFlightPlan(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORFLIGHTPLANSTATE_H
