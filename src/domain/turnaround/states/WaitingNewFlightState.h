#ifndef GSX_INTEGRATOR_CLIENT_WAITINGNEWFLIGHTSTATE_H
#define GSX_INTEGRATOR_CLIENT_WAITINGNEWFLIGHTSTATE_H

#include "TurnaroundState.h"

class WaitingNewFlightState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::WaitingNewFlight;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_WAITINGNEWFLIGHTSTATE_H
