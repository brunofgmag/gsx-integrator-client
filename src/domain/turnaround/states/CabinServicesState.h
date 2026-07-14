#ifndef GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H
#define GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H

#include "TurnaroundState.h"
#include "../../ports/GsxGateway.h"

class CabinServicesState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::CabinServices;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static bool DispatchNextService(TurnaroundContext& ctx);
    static void UpdateActiveSeen(TurnaroundContext& ctx);
    static bool AllEnabledCompleted(const TurnaroundContext& ctx);
    static bool IsServiceDone(const TurnaroundContext& ctx, bool enabled, bool activeSeen, GroundService service);
};

#endif //GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H
