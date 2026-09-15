#include <QtTest>

#include <string>
#include <vector>
#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/ports/AircraftRule.h"
#include "src/domain/turnaround/states/TurnaroundState.h"

namespace
{
    constexpr int kHoldTicks = 3;
    constexpr int kLongerHoldTicks = 6;

    class HoldingRule final : public AircraftRule
    {
    public:
        bool holds = true;
        int evaluateCalls = 0;
        int actCalls = 0;

        [[nodiscard]] const char* Name() const override
        {
            return "holding-rule";
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            ++evaluateCalls;

            return holds ? RuleVerdict::Hold(kHoldTicks, "held by the test rule") : RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
            ++actCalls;
        }
    };

    class SlowRule final : public AircraftRule
    {
    public:
        int evaluateCalls = 0;
        int actCalls = 0;
        bool holds = false;

        [[nodiscard]] const char* Name() const override
        {
            return "slow-rule";
        }

        [[nodiscard]] RuleCadence Cadence() const override
        {
            return RuleCadence::Slow;
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            ++evaluateCalls;

            return holds ? RuleVerdict::Hold(5, "slow rule holds") : RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
            ++actCalls;
        }
    };

    class JournalingRule final : public AircraftRule
    {
    public:
        JournalingRule(std::vector<std::string>& journal, const char* name, const RuleCadence cadence)
            : journal_(&journal), name_(name), cadence_(cadence)
        {
        }

        [[nodiscard]] const char* Name() const override
        {
            return name_;
        }

        [[nodiscard]] RuleCadence Cadence() const override
        {
            return cadence_;
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            journal_->push_back(std::string(name_) + " evaluates");

            return RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
            journal_->push_back(std::string(name_) + " acts");
        }

    private:
        std::vector<std::string>* journal_;
        const char* name_;
        RuleCadence cadence_;
    };

    class DeadlineRule final : public AircraftRule
    {
    public:
        DeadlineRule(const char* reason, const int ticksAllowed) : reason_(reason), ticksAllowed_(ticksAllowed)
        {
        }

        [[nodiscard]] const char* Name() const override
        {
            return reason_;
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            return RuleVerdict::Hold(ticksAllowed_, reason_);
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
        }

    private:
        const char* reason_;
        int ticksAllowed_;
    };

    class CountingState final : public TurnaroundState
    {
    public:
        int evaluateCalls = 0;
        bool needsPassengerAccess = false;

        [[nodiscard]] TurnaroundPhase Phase() const override
        {
            return TurnaroundPhase::CallServices;
        }

        [[nodiscard]] PhaseNeeds Needs() const override
        {
            return {needsPassengerAccess};
        }

    protected:
        [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext&) override
        {
            ++evaluateCalls;

            return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
        }
    };
}

class TurnaroundRulesTest final : public QObject
{
    Q_OBJECT

private slots:
    static void aHoldingRuleStopsThePhaseFromEvaluating();
    static void thePhaseEvaluatesAgainOnceTheHoldDeadlineExpires();
    static void aPilotTouchBeatsTheHold();
    static void thePhaseReceivesItsOwnNeedsInTheRuleContext();
    static void aPassingRuleLetsThePhaseEvaluate();
    static void aSlowRuleNeverRunsWhenThePhaseEvaluates();
    static void theSlowPathRunsOnlyTheSlowRules();
    static void observingTheSlowPathNeverLetsARuleAct();
    static void observingLogsEachVerdictOnceUntilItChanges();
    static void eachRuleActsRightAfterItIsEvaluatedWhenThePhaseEvaluates();
    static void eachRuleActsRightAfterItIsEvaluatedOnTheActionPass();
    static void theFirstHoldingRuleSetsTheDeadlineAndTheReason();
};

void TurnaroundRulesTest::aHoldingRuleStopsThePhaseFromEvaluating()
{
    TurnaroundStateFixture f;
    HoldingRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(state.evaluateCalls, 0);
    QCOMPARE(rule.evaluateCalls, 1);
    QCOMPARE(rule.actCalls, 1);
}

void TurnaroundRulesTest::thePhaseEvaluatesAgainOnceTheHoldDeadlineExpires()
{
    TurnaroundStateFixture f;
    HoldingRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};

    for (int tick = 0; tick < kHoldTicks; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(state.evaluateCalls, 0);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(state.evaluateCalls, 1);
}

void TurnaroundRulesTest::aPilotTouchBeatsTheHold()
{
    TurnaroundStateFixture f;
    HoldingRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};
    f.ctx.pilotTouched = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(state.evaluateCalls, 1);
    QVERIFY(f.ctx.ConsumePilotTouch());
}

void TurnaroundRulesTest::thePhaseReceivesItsOwnNeedsInTheRuleContext()
{
    TurnaroundStateFixture f;
    CountingState state;

    class NeedRecordingRule final : public AircraftRule
    {
    public:
        PhaseNeeds seen;
        TurnaroundPhase seenPhase = TurnaroundPhase::OnFlight;

        [[nodiscard]] const char* Name() const override
        {
            return "need-recording-rule";
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override
        {
            seen = context.needs;
            seenPhase = context.phase;

            return RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
        }
    };

    NeedRecordingRule rule;
    state.needsPassengerAccess = true;
    f.aircraft.rules = {&rule};

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(rule.seen.passengerAccess);
    QCOMPARE(rule.seenPhase, TurnaroundPhase::CallServices);
}

void TurnaroundRulesTest::aPassingRuleLetsThePhaseEvaluate()
{
    TurnaroundStateFixture f;
    HoldingRule rule;
    CountingState state;

    rule.holds = false;
    f.aircraft.rules = {&rule};

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(state.evaluateCalls, 1);
}

void TurnaroundRulesTest::aSlowRuleNeverRunsWhenThePhaseEvaluates()
{
    TurnaroundStateFixture f;
    SlowRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(rule.evaluateCalls, 0);
    QCOMPARE(rule.actCalls, 0);
}

void TurnaroundRulesTest::theSlowPathRunsOnlyTheSlowRules()
{
    TurnaroundStateFixture f;
    SlowRule slowRule;
    HoldingRule fastRule;
    CountingState state;

    f.aircraft.rules = {&slowRule, &fastRule};

    state.ActOnRules(f.ctx, RuleCadence::Slow);

    QCOMPARE(slowRule.evaluateCalls, 1);
    QCOMPARE(slowRule.actCalls, 1);
    QCOMPARE(fastRule.evaluateCalls, 0);
    QCOMPARE(fastRule.actCalls, 0);
}

void TurnaroundRulesTest::observingTheSlowPathNeverLetsARuleAct()
{
    TurnaroundStateFixture f;
    SlowRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};

    for (int tick = 0; tick < 3; ++tick)
    {
        state.ObserveRules(f.ctx, RuleCadence::Slow);
    }

    QCOMPARE(rule.evaluateCalls, 3);
    QCOMPARE(rule.actCalls, 0);
    QCOMPARE(f.variableWriter.setLVarCalls + f.variableWriter.setAVarCalls, 0);
}

void TurnaroundRulesTest::observingLogsEachVerdictOnceUntilItChanges()
{
    TurnaroundStateFixture f;
    SlowRule rule;
    CountingState state;

    f.aircraft.rules = {&rule};

    for (int tick = 0; tick < 5; ++tick)
    {
        state.ObserveRules(f.ctx, RuleCadence::Slow);
    }

    QCOMPARE(f.logger.messages.size(), static_cast<std::size_t>(1));
    QVERIFY(f.logger.messages.front().find("would pass") != std::string::npos);

    rule.holds = true;

    for (int tick = 0; tick < 5; ++tick)
    {
        state.ObserveRules(f.ctx, RuleCadence::Slow);
    }

    QCOMPARE(f.logger.messages.size(), static_cast<std::size_t>(2));
    QVERIFY(f.logger.messages.back().find("would hold") != std::string::npos);

    rule.holds = false;
    state.ObserveRules(f.ctx, RuleCadence::Slow);

    QCOMPARE(f.logger.messages.size(), static_cast<std::size_t>(3));
}

void TurnaroundRulesTest::eachRuleActsRightAfterItIsEvaluatedWhenThePhaseEvaluates()
{
    TurnaroundStateFixture f;
    std::vector<std::string> journal;
    JournalingRule first(journal, "first", RuleCadence::Fast);
    JournalingRule second(journal, "second", RuleCadence::Fast);
    CountingState state;

    f.aircraft.rules = {&first, &second};

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(journal, (std::vector<std::string>{"first evaluates", "first acts", "second evaluates", "second acts"}));
}

void TurnaroundRulesTest::eachRuleActsRightAfterItIsEvaluatedOnTheActionPass()
{
    TurnaroundStateFixture f;
    std::vector<std::string> journal;
    JournalingRule first(journal, "first", RuleCadence::Slow);
    JournalingRule second(journal, "second", RuleCadence::Slow);
    CountingState state;

    f.aircraft.rules = {&first, &second};

    state.ActOnRules(f.ctx, RuleCadence::Slow);

    QCOMPARE(journal, (std::vector<std::string>{"first evaluates", "first acts", "second evaluates", "second acts"}));
}

void TurnaroundRulesTest::theFirstHoldingRuleSetsTheDeadlineAndTheReason()
{
    TurnaroundStateFixture f;
    DeadlineRule shorter("the shorter hold", kHoldTicks);
    DeadlineRule longer("the longer hold", kLongerHoldTicks);
    CountingState state;

    f.aircraft.rules = {&shorter, &longer};

    for (int tick = 0; tick < kHoldTicks; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.logger.messages.back(), std::string("Rule hold expired: the shorter hold"));
}

QTEST_MAIN(TurnaroundRulesTest)
#include "tst_turnaround_rules.moc"
