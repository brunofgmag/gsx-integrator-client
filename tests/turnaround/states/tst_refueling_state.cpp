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
    static void defuelsProgressively();
    static void progressiveRampSurvivesGsxFinishingEarly();
    static void selfAircraftStaysFlatUntilGsxPours();
    static void selfAircraftCompletesWhenGsxFinishes();
    static void selfFollowsGsxFuelCounter();
    static void selfDefuelFollowsFuelCounterWithoutJumping();
    static void selfHoldsLoadedWhenCounterZeroes();
    static void forcesCompleteRefuelWhenStalledAbove95();
    static void doesNotForceCompleteRefuelBelow95();
    static void notifiesAircraftOnceWhenGsxStartsWatching();
    static void externallyRefueledAircraftMirrorsSimFuel();
    static void externallyRefueledCompletesOnGsxEvenOffTarget();
    static void externallyDefueledAircraftMirrorsSimFuel();
    static void rebaselinesInitialFuelWhenCapturedBeforeSimData();
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
    f.aircraft.refuelMethod = RefuelBy::Client;
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
    f.aircraft.refuelMethod = RefuelBy::Client;
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

void RefuelingStateTest::defuelsProgressively()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.refuelMethod = RefuelBy::Client;
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

void RefuelingStateTest::progressiveRampSurvivesGsxFinishingEarly()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.refuelMethod = RefuelBy::Client;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 700.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 990.0);

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.gsxService.hoseConnected = false;
    f.gsxService.refuelingCompleted = true;

    for (int tick = 0; tick < 23; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 760.0);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.aircraft.currentFuelKg, 700.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 700.0);
}

void RefuelingStateTest::selfAircraftStaysFlatUntilGsxPours()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 2000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;
    f.gsxService.refuelCounterGallons = 0.0;

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 1000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);
    QCOMPARE(f.aircraft.currentFuelKg, 2000.0);
}

void RefuelingStateTest::selfAircraftCompletesWhenGsxFinishes()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 100.0;
    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 2000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 3; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.ctx.data.loadedFuelKg, 2000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
    QCOMPARE(f.aircraft.currentFuelKg, 2000.0);
}

void RefuelingStateTest::selfFollowsGsxFuelCounter()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 4040.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 500.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 2520.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);
    QCOMPARE(f.aircraft.currentFuelKg, 4040.0);

    f.gsxService.refuelCounterGallons = 1100.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 4040.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingStateTest::selfDefuelFollowsFuelCounterWithoutJumping()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 60000.0;
    f.ctx.data.plannedFuelKg = 29600.0;
    f.ctx.data.initialFuelKg = 60000.0;
    f.ctx.data.loadedFuelKg = 60000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 1000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 56960.0);
    QCOMPARE(f.ctx.data.fuelProgress, 10.0);

    f.gsxService.refuelCounterGallons = 11000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 29600.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
    QCOMPARE(f.aircraft.currentFuelKg, 29600.0);
}

void RefuelingStateTest::selfHoldsLoadedWhenCounterZeroes()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 60000.0;
    f.ctx.data.plannedFuelKg = 29600.0;
    f.ctx.data.initialFuelKg = 60000.0;
    f.ctx.data.loadedFuelKg = 60000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 5000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    const double tracked = f.ctx.data.loadedFuelKg;

    QVERIFY(tracked < 60000.0);

    f.gsxService.refuelCounterGallons = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, tracked);
}

void RefuelingStateTest::forcesCompleteRefuelWhenStalledAbove95()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.refuelBaselined = true;
    f.ctx.data.loadingStartNotified = true;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;
    f.gsxService.refuelCounterGallons = 3200.0;

    for (int tick = 0; tick < 70; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.ctx.data.fuelProgress > 95.0, true);
    QCOMPARE(f.menuGateway.completeRefuelCalls, 1);
}

void RefuelingStateTest::doesNotForceCompleteRefuelBelow95()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.refuelBaselined = true;
    f.ctx.data.loadingStartNotified = true;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;
    f.gsxService.refuelCounterGallons = 3000.0;

    for (int tick = 0; tick < 70; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.completeRefuelCalls, 0);
}

void RefuelingStateTest::notifiesAircraftOnceWhenGsxStartsWatching()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 2000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.onLoadingStartedCalls, 0);

    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 3; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.aircraft.onLoadingStartedCalls, 1);
    QCOMPARE(f.ctx.data.initialFuelKg, 1000.0);
}

void RefuelingStateTest::externallyRefueledAircraftMirrorsSimFuel()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.settings.fuelRateKgs = 1000.0;
    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 1000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);

    f.aircraft.currentFuelKg = 3500.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 3500.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);

    f.aircraft.currentFuelKg = 5990.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingStateTest::externallyRefueledCompletesOnGsxEvenOffTarget()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 5800.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 5800.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
}

void RefuelingStateTest::externallyDefueledAircraftMirrorsSimFuel()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 8000.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.initialFuelKg, 8000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);

    f.aircraft.currentFuelKg = 7000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.loadedFuelKg, 7000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);

    f.aircraft.currentFuelKg = 6010.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestBoarding);
    QCOMPARE(f.ctx.data.loadedFuelKg, 6010.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingStateTest::rebaselinesInitialFuelWhenCapturedBeforeSimData()
{
    TurnaroundStateFixture f;
    RefuelingState state;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 5000.0;
    f.ctx.data.plannedFuelKg = 6151.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.loadedFuelKg = 0.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.ctx.data.initialFuelKg, 5000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);
}

QTEST_APPLESS_MAIN(RefuelingStateTest)

#include "tst_refueling_state.moc"
