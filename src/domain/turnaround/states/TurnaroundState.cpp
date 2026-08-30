#include "TurnaroundState.h"

#include <format>
#include <string>

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/AircraftRule.h"
#include "../../ports/DomainLogger.h"

namespace
{
    RuleContext BuildRuleContext(const TurnaroundState& state, const TurnaroundContext& ctx)
    {
        RuleContext context;
        context.phase = state.Phase();
        context.needs = state.Needs();
        context.phaseTickCount = ctx.data.stateTickCount;

        return context;
    }
}

std::optional<TurnaroundTransition> TurnaroundState::Evaluate(TurnaroundContext& ctx)
{
    if (AnyRuleHolds(ctx))
    {
        return std::nullopt;
    }

    return EvaluatePhase(ctx);
}

TurnaroundState::RuleOutcome TurnaroundState::RunRules(TurnaroundContext& ctx, const RuleCadence cadence)
{
    RuleOutcome outcome;

    if (ctx.aircraft == nullptr)
    {
        return outcome;
    }

    const RuleContext ruleContext = BuildRuleContext(*this, ctx);

    for (AircraftRule* const rule : ctx.aircraft->Rules())
    {
        if (rule == nullptr || rule->Cadence() != cadence)
        {
            continue;
        }

        const RuleVerdict verdict = rule->Evaluate(ruleContext);
        if (verdict.holds && !outcome.holds)
        {
            outcome.holds = true;
            outcome.ticksAllowed = verdict.holdTicksAllowed;
            outcome.reason = verdict.reason;
        }

        if (ctx.variableWriter != nullptr)
        {
            rule->Act(ruleContext, *ctx.variableWriter);
        }
    }

    return outcome;
}

void TurnaroundState::ActOnRules(TurnaroundContext& ctx, const RuleCadence cadence)
{
    RunRules(ctx, cadence);
}

bool TurnaroundState::AnyRuleHolds(TurnaroundContext& ctx)
{
    if (ctx.aircraft == nullptr)
    {
        holdTicks_ = 0;

        return false;
    }

    const RuleOutcome outcome = RunRules(ctx, RuleCadence::Fast);

    if (!outcome.holds)
    {
        holdTicks_ = 0;

        return false;
    }

    if (ctx.pilotTouched)
    {
        holdTicks_ = 0;

        return false;
    }

    ++holdTicks_;

    if (holdTicks_ > outcome.ticksAllowed)
    {
        holdTicks_ = 0;

        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo(std::format("Rule hold expired: {}", outcome.reason));
        }

        return false;
    }

    return true;
}

void TurnaroundState::ObserveRules(TurnaroundContext& ctx, const RuleCadence cadence)
{
    if (ctx.aircraft == nullptr || ctx.logger == nullptr)
    {
        return;
    }

    const RuleContext ruleContext = BuildRuleContext(*this, ctx);

    for (AircraftRule* const rule : ctx.aircraft->Rules())
    {
        if (rule == nullptr || rule->Cadence() != cadence)
        {
            continue;
        }

        const RuleVerdict verdict = rule->Evaluate(ruleContext);

        ctx.logger->LogInfo(std::format("Rule {} would {}{}", rule->Name(),
                                        verdict.holds ? "hold" : "pass",
                                        verdict.holds ? std::format(": {}", verdict.reason) : std::string{}));
    }
}
