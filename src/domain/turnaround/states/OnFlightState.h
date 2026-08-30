#ifndef GSX_INTEGRATOR_CLIENT_ONFLIGHTSTATE_H
#define GSX_INTEGRATOR_CLIENT_ONFLIGHTSTATE_H

#include "TurnaroundState.h"

class OnFlightState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::OnFlight;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_ONFLIGHTSTATE_H
