#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CallStairsOrJetwayState.h"

class CallStairsOrJetwayStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void advancesWhenStairsAreAvailable();
    static void advancesWhenJetwayIsAvailable();
    static void prefersJetwayWhenBothAreAvailable();
    static void callStairsWhenJetwayFailsToComplete();
    static void skipsWhenAircraftDoesNotSupportStairsOrJetways();
    static void advancesImmediatelyWhenJetwayAlreadyInPlace();
    static void givesUpAfterTwoAttempts();
};

void CallStairsOrJetwayStateTest::advancesWhenStairsAreAvailable()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.stairsAvailable = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.stairsInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallStairsOrJetwayStateTest::advancesWhenJetwayIsAvailable()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.jetwayAvailable = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.jetwayInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallStairsOrJetwayStateTest::prefersJetwayWhenBothAreAvailable()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;
    f.menuGateway.callStairsResult = true;
    f.menuGateway.callJetwayResult = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.jetwayInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallStairsOrJetwayStateTest::callStairsWhenJetwayFailsToComplete()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;
    f.menuGateway.callStairsResult = true;
    f.menuGateway.callJetwayResult = true;

    for (int tick = 0; tick < 121; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
}

void CallStairsOrJetwayStateTest::skipsWhenAircraftDoesNotSupportStairsOrJetways()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.aircraft.supportsStairsOrJetways = false;
    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 0);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
}

void CallStairsOrJetwayStateTest::advancesImmediatelyWhenJetwayAlreadyInPlace()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.jetwayInPlace = true;
    f.gsxService.stairsAvailable = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 0);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallStairsOrJetwayStateTest::givesUpAfterTwoAttempts()
{
    TurnaroundStateFixture f;
    CallStairsOrJetwayState state;

    f.gsxService.jetwayAvailable = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 240 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
    QCOMPARE(f.ctx.data.jetwayOrStairsAttempts, 2);
}

QTEST_APPLESS_MAIN(CallStairsOrJetwayStateTest)

#include "tst_call_stairs_or_jetway_state.moc"
