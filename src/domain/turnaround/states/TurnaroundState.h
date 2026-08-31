#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H

#include <optional>
#include "../TurnaroundTransition.h"
#include "../TurnaroundPhase.h"
#include "../rules/PhaseNeeds.h"
#include "../rules/RuleCadence.h"
#include "../../ports/GsxGateway.h"

struct TurnaroundContext;

class TurnaroundState
{
public:
    virtual ~TurnaroundState() = default;

    [[nodiscard]] virtual TurnaroundPhase Phase() const = 0;
    [[nodiscard]] virtual PhaseNeeds Needs() const { return {}; }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx);

    void ActOnRules(TurnaroundContext& ctx, RuleCadence cadence);

    void ObserveRules(TurnaroundContext& ctx, RuleCadence cadence);

protected:
    [[nodiscard]] virtual std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) = 0;

    static void NoteServiceInterruption(TurnaroundContext& ctx, const char* serviceName,
                                        GsxStateStatus state, bool started, bool completed);

private:
    struct RuleOutcome
    {
        bool holds = false;
        int ticksAllowed = 0;
        const char* reason = "";
    };

    [[nodiscard]] RuleOutcome RunRules(TurnaroundContext& ctx, RuleCadence cadence);
    [[nodiscard]] bool AnyRuleHolds(TurnaroundContext& ctx);

    int holdTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
