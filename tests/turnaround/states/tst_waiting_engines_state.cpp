#include <QtTest/QTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/states/WaitingEnginesState.h"

class WaitingEnginesStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhenEnginesOffEvenWithSmartSwitch();
    static void holdsWhenGsxAsksWithoutSmartSwitch();
    static void holdsWhenGsxNotWaitingEvenWithEverythingElse();
    static void holdsWhenParkingBrakeReleased();
    static void holdsWhenConfirmPickFails();
    static void proceedsAfterDeferredConfirmationCompletes();
    static void confirmsAndProceedsWhenAllConditionsMet();
    static void proceedsWhenConfirmationNotRequired();
    static void proceedsWhenPushbackFinished();
};

namespace
{
    void ArmConfirmationScenario(TurnaroundStateFixture& f)
    {
        f.gsxService.goodEngineStartConfirmation = true;
        f.gsxService.waitingForEngines = true;
        f.aircraft.engineRunning = true;
        f.aircraft.parkingBrakeSet = true;
        f.ctx.smartSwitchPressed = true;
    }
}

void WaitingEnginesStateTest::holdsWhenEnginesOffEvenWithSmartSwitch()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.aircraft.engineRunning = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
    QVERIFY(f.ctx.smartSwitchPressed);
}

void WaitingEnginesStateTest::holdsWhenGsxAsksWithoutSmartSwitch()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.ctx.smartSwitchPressed = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
}

void WaitingEnginesStateTest::holdsWhenGsxNotWaitingEvenWithEverythingElse()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.gsxService.waitingForEngines = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
    QVERIFY(f.ctx.smartSwitchPressed);
}

void WaitingEnginesStateTest::holdsWhenParkingBrakeReleased()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.aircraft.parkingBrakeSet = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
    QVERIFY(f.ctx.smartSwitchPressed);
}

void WaitingEnginesStateTest::holdsWhenConfirmPickFails()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.menuGateway.confirmGoodEnginesResult = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 1);
}

void WaitingEnginesStateTest::proceedsAfterDeferredConfirmationCompletes()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);
    f.menuGateway.confirmGoodEnginesResult = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.waitingForEngines = false;
    f.gsxService.pushbackFinished = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 1);
}

void WaitingEnginesStateTest::confirmsAndProceedsWhenAllConditionsMet()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    ArmConfirmationScenario(f);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 1);
    QVERIFY(!f.ctx.smartSwitchPressed);
}

void WaitingEnginesStateTest::proceedsWhenConfirmationNotRequired()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    f.gsxService.goodEngineStartConfirmation = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
}

void WaitingEnginesStateTest::proceedsWhenPushbackFinished()
{
    TurnaroundStateFixture f;
    WaitingEnginesState state;

    f.gsxService.goodEngineStartConfirmation = true;
    f.gsxService.pushbackFinished = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.confirmGoodEnginesCalls, 0);
}

QTEST_APPLESS_MAIN(WaitingEnginesStateTest)

#include "tst_waiting_engines_state.moc"
