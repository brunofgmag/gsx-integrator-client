#ifndef GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H
#define GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H

#include "TurnaroundState.h"

class CabinServicesState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::CabinServices;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    static bool RequestNextService(TurnaroundContext& ctx);
};

#endif //GSX_INTEGRATOR_CLIENT_CABINSERVICESSTATE_H
