#include <QtTest/QTest>

#include <algorithm>
#include <string>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingPushbackToStartState.h"

namespace
{
    constexpr double kTaxiKnots = 12.0;

    bool LoggedTheDroppedPushback(const TurnaroundStateFixture& f)
    {
        return std::ranges::any_of(f.logger.messages, [](const std::string& message)
        {
            return message.find("GSX dropped the pushback it had already started") != std::string::npos;
        });
    }

    void EvaluateWithThePushbackPending(TurnaroundStateFixture& f, WaitingPushbackToStartState& state)
    {
        f.gsxService.departureInProgress = true;
        (void)state.Evaluate(f.ctx);
    }
}

class WaitingPushbackToStartStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhilePushbackNotStarted();
    static void opensThePushbackPanelWhileItWaits();
    static void neverOpensThePushbackPanelOnceThePushMoves();
    static void advancesToEnginesWhenPushbackStarts();
    static void advancesToDepartureWhenPushbackFinished();
    static void advancesToDepartureWhenPushbackWasCompleted();
    static void neverConsumesSmartSwitch();
    static void warnsWhenGsxWentDownWithThePushbackPendingAndDropsItAfterwards();
    static void warnsWhenTheDropArrivesOnTheTickGsxIsBack();
    static void staysQuietWhenThePushbackIsWithdrawnWhileGsxRuns();
    static void aGsxRestartAfterThePushbackWasWithdrawnRaisesNoWarning();
    static void clearsTheWarningOnceThePushbackIsRequestedAgain();
    static void movesOnToTheDepartureWhenTheAircraftTaxisUnderItsOwnPower();
    static void holdsWhileTheEnginesRunWithTheAircraftStill();
    static void holdsWhileTheAircraftMovesWithTheEnginesOff();
    static void movesOnToTheDepartureOnceTheAircraftIsAirborne();
    static void aPushbackThatStartedStillGoesThroughTheEngines();
};

void WaitingPushbackToStartStateTest::holdsWhilePushbackNotStarted()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.departureState = GsxStateStatus::Requested;
    f.gsxService.pushbackStarted = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingPushbackToStartStateTest::opensThePushbackPanelWhileItWaits()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.departureState = GsxStateStatus::Requested;
    f.gsxService.pushbackStarted = false;

    (void)state.Evaluate(f.ctx);

    QCOMPARE(f.menuGateway.openPushbackPanelCalls, 1);
}

void WaitingPushbackToStartStateTest::neverOpensThePushbackPanelOnceThePushMoves()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.departureState = GsxStateStatus::Active;
    f.gsxService.pushbackStarted = true;

    (void)state.Evaluate(f.ctx);

    QCOMPARE(f.menuGateway.openPushbackPanelCalls, 0);
}

void WaitingPushbackToStartStateTest::advancesToEnginesWhenPushbackStarts()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.departureState = GsxStateStatus::Active;
    f.gsxService.pushbackStarted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingForEngines);
}

void WaitingPushbackToStartStateTest::advancesToDepartureWhenPushbackFinished()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.pushbackFinished = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
}

void WaitingPushbackToStartStateTest::advancesToDepartureWhenPushbackWasCompleted()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.pushbackCompleted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
}

void WaitingPushbackToStartStateTest::neverConsumesSmartSwitch()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.aircraft.smartSwitchActivated = true;
    f.aircraft.engineRunning = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.departureState = GsxStateStatus::Active;
    f.gsxService.pushbackStarted = true;

    (void)state.Evaluate(f.ctx);

    QCOMPARE(f.aircraft.consumeSmartSwitchCalls, 0);
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
}

void WaitingPushbackToStartStateTest::warnsWhenGsxWentDownWithThePushbackPendingAndDropsItAfterwards()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    EvaluateWithThePushbackPending(f, state);

    f.gsxService.gsxDownSinceLastObserve = true;
    (void)state.Evaluate(f.ctx);

    QVERIFY(!f.ctx.data.serviceInterrupted);

    f.gsxService.gsxDownSinceLastObserve = false;
    f.gsxService.departureInProgress = false;

    for (int tick = 0; tick < 120; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QVERIFY(f.ctx.data.serviceInterrupted);
    QVERIFY(LoggedTheDroppedPushback(f));
    QCOMPARE(f.menuGateway.pushbackCalls, 0);
}

void WaitingPushbackToStartStateTest::warnsWhenTheDropArrivesOnTheTickGsxIsBack()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    EvaluateWithThePushbackPending(f, state);

    f.gsxService.gsxDownSinceLastObserve = true;
    f.gsxService.departureInProgress = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.serviceInterrupted);
}

void WaitingPushbackToStartStateTest::staysQuietWhenThePushbackIsWithdrawnWhileGsxRuns()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    EvaluateWithThePushbackPending(f, state);

    f.gsxService.departureInProgress = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.serviceInterrupted);
    QVERIFY(!LoggedTheDroppedPushback(f));
}

void WaitingPushbackToStartStateTest::aGsxRestartAfterThePushbackWasWithdrawnRaisesNoWarning()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    EvaluateWithThePushbackPending(f, state);

    f.gsxService.departureInProgress = false;
    (void)state.Evaluate(f.ctx);

    f.gsxService.gsxDownSinceLastObserve = true;
    (void)state.Evaluate(f.ctx);

    f.gsxService.gsxDownSinceLastObserve = false;
    (void)state.Evaluate(f.ctx);

    QVERIFY(!f.ctx.data.serviceInterrupted);
}

void WaitingPushbackToStartStateTest::clearsTheWarningOnceThePushbackIsRequestedAgain()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    EvaluateWithThePushbackPending(f, state);

    f.gsxService.gsxDownSinceLastObserve = true;
    f.gsxService.departureInProgress = false;
    (void)state.Evaluate(f.ctx);

    f.gsxService.gsxDownSinceLastObserve = false;
    (void)state.Evaluate(f.ctx);

    QVERIFY(f.ctx.data.serviceInterrupted);

    EvaluateWithThePushbackPending(f, state);

    QVERIFY(!f.ctx.data.serviceInterrupted);

    f.gsxService.departureInProgress = false;
    (void)state.Evaluate(f.ctx);

    QVERIFY(!f.ctx.data.serviceInterrupted);
}

void WaitingPushbackToStartStateTest::movesOnToTheDepartureWhenTheAircraftTaxisUnderItsOwnPower()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.aircraft.engineRunning = true;
    f.gsxService.groundSpeedKnots = kTaxiKnots;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
}

void WaitingPushbackToStartStateTest::holdsWhileTheEnginesRunWithTheAircraftStill()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.aircraft.engineRunning = true;
    f.gsxService.groundSpeedKnots = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingPushbackToStartStateTest::holdsWhileTheAircraftMovesWithTheEnginesOff()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.aircraft.engineRunning = false;
    f.gsxService.groundSpeedKnots = kTaxiKnots;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingPushbackToStartStateTest::movesOnToTheDepartureOnceTheAircraftIsAirborne()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.onGround = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
}

void WaitingPushbackToStartStateTest::aPushbackThatStartedStillGoesThroughTheEngines()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.aircraft.engineRunning = true;
    f.gsxService.groundSpeedKnots = kTaxiKnots;
    f.gsxService.pushbackStarted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingForEngines);
}

QTEST_APPLESS_MAIN(WaitingPushbackToStartStateTest)

#include "tst_waiting_pushback_to_start_state.moc"
