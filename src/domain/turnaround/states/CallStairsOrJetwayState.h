#ifndef GSX_INTEGRATOR_CLIENT_CALLSTAIRSORJETWAY_H
#define GSX_INTEGRATOR_CLIENT_CALLSTAIRSORJETWAY_H

#include "TurnaroundState.h"

class CallStairsOrJetwayState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::CallStairsOrJetway;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;

private:
    const int retries_ = 0;
};

#endif //GSX_INTEGRATOR_CLIENT_CALLSTAIRSORJETWAY_H
