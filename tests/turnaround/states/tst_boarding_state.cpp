#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/BoardingState.h"

class BoardingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilGsxActive();
    static void boardSelfLoadsPayloadOnceAndAnimatesBar();
    static void boardPassengersProgressively();
    static void holdsDoorsClosedOnceBoardingFinishes();
    static void doesNotHoldDoorsWhileCargoStillPending();
    static void boardCargoProgressively();
    static void snapsToPlannedWhenGsxCountersFallShort();
    static void rebaselinesInitialZfwWhenCapturedBeforeSimData();
    static void clampsRebaselineToPlannedZfw();
    static void asksGsxToCompleteBoardingStalledAtOneHundred();
    static void asksAgainWhenGsxSwallowsTheForcedCompletion();
    static void stopsAskingOnceTheServiceCloses();
    static void doesNotAskGsxToCompleteWhileTheLoaderIsStillWorking();
    static void doesNotAskGsxToCompleteWhilePassengersAreMissing();
};

void BoardingStateTest::holdsUntilGsxActive()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.gsxService.boardingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void BoardingStateTest::boardSelfLoadsPayloadOnceAndAnimatesBar()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.aircraft.emptyZfwKg = 130000.0;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 0;
    f.gsxService.cargoPercent = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 0.0);

    f.gsxService.boardedPassengers = 100;
    f.gsxService.cargoPercent = 100.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 75.0);
}

void BoardingStateTest::boardPassengersProgressively()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 0;
    f.gsxService.boardedPassengers = 0;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.boardedPassengers += 5;
        f.gsxService.cargoPercent += 5.0;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.boardingState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(f.aircraft.currentZfwKg, 200000.0);
    QCOMPARE(f.gsxService.boardedPassengers, 100);
    QCOMPARE(f.gsxService.cargoPercent, 100.0);
    QCOMPARE(f.ctx.data.boardedPassengers, 100);
}

void BoardingStateTest::holdsDoorsClosedOnceBoardingFinishes()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.aircraft.doorsHeldClosed);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(f.aircraft.doorsHeldClosed);
}

void BoardingStateTest::doesNotHoldDoorsWhileCargoStillPending()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Completed;
    f.gsxService.loadingCargo = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.aircraft.doorsHeldClosed);
    QCOMPARE(f.aircraft.holdDoorsClosedCalls, 0);
}

void BoardingStateTest::boardCargoProgressively()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = true;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.currentZfwKg = 130000.0;
    f.aircraft.emptyZfwKg = 130000.0;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 3;
    f.gsxService.boardingState = GsxStateStatus::Active;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.cargoPercent += 5.0;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }
    f.gsxService.boardingState = GsxStateStatus::Completed;
    f.gsxService.boardedPassengers = 3;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
}

void BoardingStateTest::snapsToPlannedWhenGsxCountersFallShort()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.boardedPassengers = 195;
    f.gsxService.cargoPercent = 0.0;
    f.gsxService.boardingState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(f.aircraft.currentZfwKg, 200000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 100.0);
    QCOMPARE(f.ctx.data.boardedPassengers, 200);
}

void BoardingStateTest::rebaselinesInitialZfwWhenCapturedBeforeSimData()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 45000.0;
    f.ctx.data.initialZfwKg = 0.0;
    f.ctx.data.plannedZfwKg = 65000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 50;
    f.gsxService.cargoPercent = 50.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.ctx.data.initialZfwKg, 45000.0);
    QCOMPARE(f.aircraft.currentZfwKg, 55000.0);
}

void BoardingStateTest::clampsRebaselineToPlannedZfw()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 42000.0;
    f.ctx.data.initialZfwKg = 0.0;
    f.ctx.data.plannedZfwKg = 20000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.ctx.data.initialZfwKg, 20000.0);
}

void BoardingStateTest::asksGsxToCompleteBoardingStalledAtOneHundred()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingStateTest::asksAgainWhenGsxSwallowsTheForcedCompletion()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    for (int tick = 0; tick < 29; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.completeBoardingCalls, 2);

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 3);
}

void BoardingStateTest::stopsAskingOnceTheServiceCloses()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);
    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingReadyToPush);

    for (int tick = 0; tick < 120; ++tick)
    {
        (void)state.Evaluate(f.ctx);
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingStateTest::doesNotAskGsxToCompleteWhileTheLoaderIsStillWorking()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;
    f.gsxService.loaderWaitingForDoor = true;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
}

void BoardingStateTest::doesNotAskGsxToCompleteWhilePassengersAreMissing()
{
    TurnaroundStateFixture f;
    BoardingState state;

    f.aircraft.cargo = false;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 174;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;
    f.gsxService.boardedPassengers = 173;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    f.gsxService.boardedPassengers = 174;
    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

QTEST_APPLESS_MAIN(BoardingStateTest)

#include "tst_boarding_state.moc"
