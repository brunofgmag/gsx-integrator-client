#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H

#include <map>
#include <optional>
#include <string>
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

    void ActOnRules(TurnaroundContext& ctx, RuleCadence cadence) const;

    void ObserveRules(TurnaroundContext& ctx, RuleCadence cadence);

    void ForgetObservedVerdicts() { observedVerdicts_.clear(); }

protected:
    [[nodiscard]] virtual std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) = 0;

    static void NoteServiceInterruption(TurnaroundContext& ctx, const char* serviceName,
                                        GsxStateStatus state, bool started, bool completed);

    static void NoteServiceInterruption(TurnaroundContext& ctx, const char* serviceName, bool interrupted);

private:
    [[nodiscard]] bool AnyRuleHolds(TurnaroundContext& ctx) const;

    std::map<std::string, std::string> observedVerdicts_;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
