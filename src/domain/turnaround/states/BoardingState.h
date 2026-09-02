#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H

#include "TurnaroundState.h"

class BoardingState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::Boarding;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    [[nodiscard]] static bool IsCargoPending(const TurnaroundContext& ctx);
    [[nodiscard]] static bool IsBarFull(const TurnaroundContext& ctx);
    [[nodiscard]] static bool IsCargoHeldBehindTheStairs(const TurnaroundContext& ctx);
    static void MaybeForceCompletion(TurnaroundContext& ctx);
    static void EnsureBaseline(TurnaroundContext& ctx);
    static void FinishBoarding(TurnaroundContext& ctx);
    static void AdvanceBoardingBar(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDSTATE_H
