#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingPushbackToStartState.h"

class WaitingPushbackToStartStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhilePushbackNotStarted();
    static void advancesToEnginesWhenPushbackStarts();
    static void advancesToDepartureWhenPushbackFinished();
    static void advancesToDepartureWhenPushbackWasCompleted();
    static void neverConsumesSmartSwitch();
};

void WaitingPushbackToStartStateTest::holdsWhilePushbackNotStarted()
{
    TurnaroundStateFixture f;
    WaitingPushbackToStartState state;

    f.gsxService.departureState = GsxStateStatus::Requested;
    f.gsxService.pushbackStarted = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
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

QTEST_APPLESS_MAIN(WaitingPushbackToStartStateTest)

#include "tst_waiting_pushback_to_start_state.moc"
