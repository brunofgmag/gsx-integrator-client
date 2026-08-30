#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTPUSHBACKSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTPUSHBACKSTATE_H

#include "TurnaroundState.h"

class RequestPushbackState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RequestPushback;
    }

protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REQUESTPUSHBACKSTATE_H
