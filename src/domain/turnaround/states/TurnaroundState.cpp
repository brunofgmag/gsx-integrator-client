#include "TurnaroundState.h"

#include <format>
#include <optional>
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
        context.needs.loading = ctx.data.loadingStartNotified;
        context.phaseTickCount = ctx.data.stateTickCount;

        return context;
    }

    template <typename OnVerdict>
    void EvaluateRules(const TurnaroundState& state, const TurnaroundContext& ctx, const RuleCadence cadence,
                       OnVerdict onVerdict)
    {
        if (ctx.aircraft == nullptr)
        {
            return;
        }

        const RuleContext ruleContext = BuildRuleContext(state, ctx);

        for (AircraftRule* const rule : ctx.aircraft->Rules())
        {
            if (rule == nullptr || rule->Cadence() != cadence)
            {
                continue;
            }

            onVerdict(*rule, ruleContext, rule->Evaluate(ruleContext));
        }
    }

    void Act(AircraftRule& rule, const RuleContext& ruleContext, const TurnaroundContext& ctx)
    {
        if (ctx.variableWriter != nullptr)
        {
            rule.Act(ruleContext, *ctx.variableWriter);
        }
    }
}

void TurnaroundState::NoteServiceInterruption(TurnaroundContext& ctx, const char* serviceName,
                                              const GsxStateStatus state, const bool started,
                                              const bool completed)
{
    const bool interrupted = started && !completed && state < GsxStateStatus::Requested;
    if (interrupted == ctx.data.serviceInterrupted)
    {
        return;
    }

    ctx.data.serviceInterrupted = interrupted;

    if (interrupted && ctx.logger != nullptr)
    {
        ctx.logger->LogInfo(std::format(
            "GSX dropped the {} it had already started; the flow is waiting and will not re-request it",
            serviceName));
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

void TurnaroundState::ActOnRules(TurnaroundContext& ctx, const RuleCadence cadence)
{
    EvaluateRules(*this, ctx, cadence, [&ctx](AircraftRule& rule, const RuleContext& ruleContext, const RuleVerdict&)
    {
        Act(rule, ruleContext, ctx);
    });
}

bool TurnaroundState::AnyRuleHolds(TurnaroundContext& ctx)
{
    std::optional<RuleVerdict> hold;

    EvaluateRules(*this, ctx, RuleCadence::Fast,
                  [&ctx, &hold](AircraftRule& rule, const RuleContext& ruleContext, const RuleVerdict& verdict)
                  {
                      if (verdict.holds && !hold.has_value())
                      {
                          hold = verdict;
                      }

                      Act(rule, ruleContext, ctx);
                  });

    if (!hold.has_value() || ctx.pilotTouched)
    {
        holdTicks_ = 0;

        return false;
    }

    ++holdTicks_;

    if (holdTicks_ > hold->holdTicksAllowed)
    {
        holdTicks_ = 0;

        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo(std::format("Rule hold expired: {}", hold->reason));
        }

        return false;
    }

    return true;
}

void TurnaroundState::ObserveRules(TurnaroundContext& ctx, const RuleCadence cadence)
{
    if (ctx.logger == nullptr)
    {
        return;
    }

    EvaluateRules(*this, ctx, cadence,
                  [this, &ctx](const AircraftRule& rule, const RuleContext&, const RuleVerdict& verdict)
                  {
                      std::string message = std::format("Rule {} would {}{}", rule.Name(),
                                                        verdict.holds ? "hold" : "pass",
                                                        verdict.holds
                                                            ? std::format(": {}", verdict.reason)
                                                            : std::string{});

                      std::string& lastLogged = observedVerdicts_[rule.Name()];
                      if (lastLogged == message)
                      {
                          return;
                      }

                      lastLogged = std::move(message);
                      ctx.logger->LogInfo(lastLogged);
                  });
}
