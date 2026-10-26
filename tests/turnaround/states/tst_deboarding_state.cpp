#include <QtTest/QTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/states/DeboardingState.h"

class DeboardingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilGsxActive();
    static void deboardSelfUnloadsPayloadOnceAndAnimatesBar();
    static void deboardPassengersProgressively();
    static void deboardCargoProgressively();
    static void snapsToInitialWhenGsxCountersFallShort();
    static void theDeboardedPassengersFollowTheGsxCounter();
    static void aDeboardingWithoutAPlanUnloadsTheLoadOnBoardProgressively();
    static void aDeboardingWithoutAPlanLeavesTheSelfBoardingAircraftEmpty();
    static void aDeboardingWithoutAPlanNeverWritesBelowTheLoadOnBoard();
    static void aDeboardingWithoutPlannedPassengersFollowsTheCargoPercentAlone();
};

void DeboardingStateTest::aDeboardingWithoutAPlanUnloadsTheLoadOnBoardProgressively()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.aircraft.currentZfwKg = 140000.0;
    f.gsxService.plannedPassengers = 100;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardedPassengers = 0;
    f.gsxService.deboardingCargoPercent = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.plannedZfwKg, 140000.0);
    QCOMPARE(f.ctx.data.initialZfwKg, 100000.0);
    QCOMPARE(f.ctx.data.plannedPassengers, 100);
    QCOMPARE(f.aircraft.currentZfwKg, 140000.0);

    f.gsxService.deboardedPassengers = 50;
    f.gsxService.deboardingCargoPercent = 50.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 120000.0);
    QCOMPARE(f.ctx.data.deboardingProgress, 50.0);

    f.gsxService.deboardingState = GsxStateStatus::Completed;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 100000.0);
}

void DeboardingStateTest::aDeboardingWithoutAPlanLeavesTheSelfBoardingAircraftEmpty()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.aircraft.emptyZfwKg = 100000.0;
    f.aircraft.currentZfwKg = 140000.0;
    f.gsxService.deboardingState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 100000.0);
    QCOMPARE(f.ctx.data.plannedZfwKg, 140000.0);
}

void DeboardingStateTest::aDeboardingWithoutAPlanNeverWritesBelowTheLoadOnBoard()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.aircraft.currentZfwKg = 90000.0;
    f.gsxService.plannedPassengers = 100;
    f.gsxService.deboardingState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.plannedZfwKg, 90000.0);
    QCOMPARE(f.ctx.data.initialZfwKg, 90000.0);
    QCOMPARE(f.aircraft.currentZfwKg, 90000.0);

    f.gsxService.deboardedPassengers = 50;
    f.gsxService.deboardingCargoPercent = 50.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 90000.0);

    f.gsxService.deboardingState = GsxStateStatus::Completed;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 90000.0);
}

void DeboardingStateTest::aDeboardingWithoutPlannedPassengersFollowsTheCargoPercentAlone()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.aircraft.currentZfwKg = 140000.0;
    f.gsxService.plannedPassengers = 0;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardedPassengers = 1;
    f.gsxService.deboardingCargoPercent = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.plannedPassengers, 0);
    QCOMPARE(f.ctx.data.deboardingProgress, 0.0);
    QCOMPARE(f.aircraft.currentZfwKg, 140000.0);

    f.gsxService.deboardingCargoPercent = 50.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.deboardingProgress, 50.0);
    QCOMPARE(f.aircraft.currentZfwKg, 120000.0);
}

void DeboardingStateTest::holdsUntilGsxActive()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.gsxService.deboardingState = GsxStateStatus::Callable;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
}

void DeboardingStateTest::deboardSelfUnloadsPayloadOnceAndAnimatesBar()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.aircraft.currentZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardedPassengers = 0;
    f.gsxService.deboardingCargoPercent = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 130000.0);
    QCOMPARE(f.ctx.data.deboardingProgress, 0.0);

    f.gsxService.deboardedPassengers = 100;
    f.gsxService.deboardingCargoPercent = 100.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.currentZfwKg, 130000.0);
    QCOMPARE(f.ctx.data.deboardingProgress, 75.0);
}

void DeboardingStateTest::deboardPassengersProgressively()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100;
    f.gsxService.boardedPassengers = 100;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.deboardedPassengers += 5;
        f.gsxService.deboardingCargoPercent += 5.0;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.deboardingState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CabinServices);
    QCOMPARE(f.aircraft.currentZfwKg, 100000.0);
    QCOMPARE(f.gsxService.deboardedPassengers, 100);
    QCOMPARE(f.gsxService.deboardingCargoPercent, 100.0);
}

void DeboardingStateTest::deboardCargoProgressively()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = true;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.currentZfwKg = 180000.0;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 3;
    f.gsxService.deboardingState = GsxStateStatus::Active;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.deboardingCargoPercent += 5.0;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }
    f.gsxService.deboardingState = GsxStateStatus::Completed;
    f.gsxService.deboardedPassengers = 3;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CabinServices);
    QCOMPARE(f.aircraft.currentZfwKg, 130000.0);
}

void DeboardingStateTest::snapsToInitialWhenGsxCountersFallShort()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.currentZfwKg = 200000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.deboardedPassengers = 195;
    f.gsxService.deboardingCargoPercent = 0.0;
    f.gsxService.deboardingState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CabinServices);
    QCOMPARE(f.aircraft.currentZfwKg, 100000.0);
    QCOMPARE(f.ctx.data.deboardingProgress, 100.0);
}

void DeboardingStateTest::theDeboardedPassengersFollowTheGsxCounter()
{
    TurnaroundStateFixture f;
    DeboardingState state;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.ctx.data.initialZfwKg = 50000.0;
    f.ctx.data.plannedZfwKg = 65000.0;
    f.ctx.data.plannedPassengers = 169;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardingCargoPercent = 100.0;
    f.gsxService.deboardedPassengers = 0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.deboardingProgress, 50.0);
    QCOMPARE(f.ctx.data.deboardedPassengers, 0);

    f.gsxService.deboardedPassengers = 30;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.deboardedPassengers, 30);

    f.gsxService.deboardingState = GsxStateStatus::Completed;
    f.gsxService.deboardedPassengers = 165;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.deboardedPassengers, 169);
}

QTEST_APPLESS_MAIN(DeboardingStateTest)

#include "tst_deboarding_state.moc"
