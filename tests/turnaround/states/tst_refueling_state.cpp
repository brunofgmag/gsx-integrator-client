#include <QtTest/QTest>

#include "../../../tests/turnaround/TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RefuelingState.h"

class RefuelingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilGsxIsReady();
    static void refuelsProgressively();
    static void refuelsProgressivelyOddValues();
    static void nonProgressiveSetsTargetImmediately();
    static void defuelsProgressively();
    static void defuelsAtOnce();
};

void RefuelingStateTest::holdsUntilGsxIsReady()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void RefuelingStateTest::refuelsProgressively()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.flightPlanLoaded = true;
    f.aircraft.progressiveFuel = true;
    f.aircraft.plannedFuelKg = 200.0;
    f.aircraft.currentFuelKg = 100.0;
    f.ctx.data.plannedFuelKg = 200.0;
    f.ctx.data.initialFuelKg = 100.0;
    f.ctx.data.loadedFuelKg = 100.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 10; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
        QCOMPARE(f.ctx.data.fuelProgress, (tick + 1) * 10.0);
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(transition->delayTicks, 30);
    QCOMPARE(f.aircraft.currentFuelKg, 200.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 200.0);
}

void RefuelingStateTest::refuelsProgressivelyOddValues()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 7.0;
    f.aircraft.flightPlanLoaded = true;
    f.aircraft.progressiveFuel = true;
    f.aircraft.plannedFuelKg = 200.0;
    f.aircraft.currentFuelKg = 200.0;
    f.ctx.data.plannedFuelKg = 200.0;
    f.ctx.data.initialFuelKg = 100.0;
    f.ctx.data.loadedFuelKg = 100.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 15; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(transition->delayTicks, 30);
    QCOMPARE(f.aircraft.currentFuelKg, 200.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 200.0);
}

void RefuelingStateTest::nonProgressiveSetsTargetImmediately()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.progressiveFuel = false;
    f.aircraft.currentFuelKg = 5000.0;
    f.ctx.data.plannedFuelKg = 12000.0;
    f.ctx.data.initialFuelKg = 5000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.currentFuelKg, 12000.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 12000.0);
}

void RefuelingStateTest::defuelsProgressively()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.progressiveFuel = true;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 700.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.aircraft.currentFuelKg, 700.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 700.0);
}

void RefuelingStateTest::defuelsAtOnce()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.progressiveFuel = false;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 700.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.aircraft.currentFuelKg, 700.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 700.0);
}

QTEST_APPLESS_MAIN(RefuelingStateTest)

#include "tst_refueling_state.moc"
