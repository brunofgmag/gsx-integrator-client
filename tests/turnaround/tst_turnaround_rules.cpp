#include <QtTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/ports/AircraftRule.h"
#include "src/domain/turnaround/states/TurnaroundState.h"

namespace
{
    constexpr int kHoldTicks = 3;

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

QTEST_MAIN(TurnaroundRulesTest)
#include "tst_turnaround_rules.moc"
