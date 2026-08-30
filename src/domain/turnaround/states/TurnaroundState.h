#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H

#include <optional>
#include "../TurnaroundTransition.h"
#include "../TurnaroundPhase.h"
#include "../rules/PhaseNeeds.h"

struct TurnaroundContext;

class TurnaroundState
{
public:
    virtual ~TurnaroundState() = default;

    [[nodiscard]] virtual TurnaroundPhase Phase() const = 0;
    [[nodiscard]] virtual PhaseNeeds Needs() const { return {}; }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx);

    void ObserveRules(TurnaroundContext& ctx);

protected:
    [[nodiscard]] virtual std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) = 0;

private:
    [[nodiscard]] bool AnyRuleHolds(TurnaroundContext& ctx);

    int holdTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
