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
    [[nodiscard]] static bool AwaitsLatestFlightPlan(TurnaroundContext& ctx);
    [[nodiscard]] static bool GsxServesTheLatestPlan(const TurnaroundContext& ctx);
    static void AwaitSimbriefLoad(TurnaroundContext& ctx);
    static void FetchTheLatestPlanWhileTheAircraftPlanDiffers(TurnaroundContext& ctx);
    static void CaptureFlightPlan(TurnaroundContext& ctx);
    static void NoteCrewLeftOutOfThePlan(TurnaroundContext& ctx);
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_WAITFORFLIGHTPLANSTATE_H
