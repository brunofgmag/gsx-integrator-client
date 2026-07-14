#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H

#include "TurnaroundState.h"

class RefuelingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Refueling;
    }


    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static void EnsureBaseline(TurnaroundContext& ctx);
    static void NotifyLoadingStarted(TurnaroundContext& ctx);
    static void AccumulateFuel(TurnaroundContext& ctx);
    static void MaybeForceCompletion(TurnaroundContext& ctx);
    static void SnapToPlanned(TurnaroundContext& ctx);
    static void RefuelProgressively(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELSTATE_H
