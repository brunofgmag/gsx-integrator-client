#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H

#include "TurnaroundState.h"
#include "../../ports/GsxGateway.h"

class RefuelingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Refueling;
    }



protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    static void EnsureBaseline(TurnaroundContext& ctx);
    static void NotifyLoadingStarted(TurnaroundContext& ctx);
    static void AccumulateFuel(TurnaroundContext& ctx);
    static void MaybeForceCompletion(TurnaroundContext& ctx, GsxStateStatus refuelingState);
    static void SnapToPlanned(TurnaroundContext& ctx);
    static void WarnWhenFuelDidNotStay(TurnaroundContext& ctx);
    static void RefuelProgressively(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H
