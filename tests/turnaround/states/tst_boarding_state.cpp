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
    static void boardCargoProgressively();
    static void snapsToPlannedWhenGsxCountersFallShort();
    static void rebaselinesInitialZfwWhenCapturedBeforeSimData();
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

QTEST_APPLESS_MAIN(BoardingStateTest)

#include "tst_boarding_state.moc"
