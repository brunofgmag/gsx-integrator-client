#include <QtTest/QTest>

#include "../../../tests/turnaround/TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RefuelingTrack.h"

class RefuelingTrackTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilGsxIsReady();
    static void refuelsProgressively();
    static void refuelsProgressivelyOddValues();
    static void defuelsProgressively();
    static void progressiveRampSurvivesGsxFinishingEarly();
    static void keepsRampingThroughATruckSwap();
    static void stopsWritingFuelWhileTheRemoteApiIsDown();
    static void selfAircraftStaysFlatUntilGsxPours();
    static void selfAircraftCompletesWhenGsxFinishes();
    static void selfFollowsGsxFuelCounter();
    static void selfDefuelFollowsFuelCounterWithoutJumping();
    static void selfHoldsLoadedWhenCounterZeroes();
    static void forcesCompleteRefuelWhenStalledAbove95();
    static void forcesCompleteRefuelAgainWhileStillStalled();
    static void doesNotForceCompleteRefuelBelow95();
    static void doesNotForceCompleteRefuelJustUnder95();
    static void skipsTheForceWhileTheTruckIsAlreadyLeaving();
    static void skipsTheForceWhenGsxAlreadyCompleted();
    static void notifiesAircraftOnceWhenGsxStartsWatching();
    static void externallyRefueledAircraftMirrorsSimFuel();
    static void externallyRefueledCompletesOnGsxEvenOffTarget();
    static void externallyDefueledAircraftMirrorsSimFuel();
    static void rebaselinesInitialFuelWhenCapturedBeforeSimData();
    static void staysQuietWhenThePlanSimplyDidNotFit();
    static void warnsWhenTheAircraftRefusedFuelThatWasWritten();
};

void RefuelingTrackTest::holdsUntilGsxIsReady()
{
    TurnaroundStateFixture f;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
}

void RefuelingTrackTest::refuelsProgressively()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
        QCOMPARE(f.ctx.data.fuelProgress, (tick + 1) * 10.0);
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 200.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 200.0);
}

void RefuelingTrackTest::refuelsProgressivelyOddValues()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 200.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 200.0);
}

void RefuelingTrackTest::defuelsProgressively()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 700.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 700.0);
}

void RefuelingTrackTest::progressiveRampSurvivesGsxFinishingEarly()
{
    TurnaroundStateFixture f;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.refuelMethod = RefuelBy::Client;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 700.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 990.0);

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.gsxService.hoseConnected = false;
    f.gsxService.refuelingCompleted = true;

    for (int tick = 0; tick < 23; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 760.0);

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 700.0);
    QCOMPARE(f.ctx.data.loadedFuelKg, 700.0);
}

void RefuelingTrackTest::keepsRampingThroughATruckSwap()
{
    TurnaroundStateFixture f;

    f.settings.fuelRateKgs = 10.0;
    f.aircraft.refuelMethod = RefuelBy::Client;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 2000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 10; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 1100.0);

    f.gsxService.hoseConnected = false;

    for (int tick = 0; tick < 74; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
        QCOMPARE(f.aircraft.currentFuelKg, 1110.0 + tick * 10.0);
    }

    f.gsxService.hoseConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 1850.0);
}

void RefuelingTrackTest::stopsWritingFuelWhileTheRemoteApiIsDown()
{
    TurnaroundStateFixture f;

    f.settings.fuelRateKgs = 20.0;
    f.aircraft.refuelMethod = RefuelBy::Client;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 10; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.aircraft.currentFuelKg, 1200.0);

    f.gsxService.remoteApiConnected = false;

    for (int tick = 0; tick < 28; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 1200.0);
    QCOMPARE(f.aircraft.currentFuelKg, 1200.0);

    f.gsxService.remoteApiConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentFuelKg, 1220.0);
}

void RefuelingTrackTest::selfAircraftStaysFlatUntilGsxPours()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loadedFuelKg, 1000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);
    QCOMPARE(f.aircraft.currentFuelKg, 2000.0);
}

void RefuelingTrackTest::selfAircraftCompletesWhenGsxFinishes()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 2000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
    QCOMPARE(f.aircraft.currentFuelKg, 2000.0);
}

void RefuelingTrackTest::selfFollowsGsxFuelCounter()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 4040.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 500.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 2520.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);
    QCOMPARE(f.aircraft.currentFuelKg, 4040.0);

    f.gsxService.refuelCounterGallons = 1100.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 4040.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingTrackTest::selfDefuelFollowsFuelCounterWithoutJumping()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 60000.0;
    f.ctx.data.plannedFuelKg = 29600.0;
    f.ctx.data.initialFuelKg = 60000.0;
    f.ctx.data.loadedFuelKg = 60000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 1000.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 56960.0);
    QCOMPARE(f.ctx.data.fuelProgress, 10.0);

    f.gsxService.refuelCounterGallons = 11000.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 29600.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
    QCOMPARE(f.aircraft.currentFuelKg, 29600.0);
}

void RefuelingTrackTest::selfHoldsLoadedWhenCounterZeroes()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 60000.0;
    f.ctx.data.plannedFuelKg = 29600.0;
    f.ctx.data.initialFuelKg = 60000.0;
    f.ctx.data.loadedFuelKg = 60000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    f.gsxService.refuelCounterGallons = 5000.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));

    const double tracked = f.ctx.data.loadedFuelKg;

    QVERIFY(tracked < 60000.0);

    f.gsxService.refuelCounterGallons = 0.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, tracked);
}

void RefuelingTrackTest::forcesCompleteRefuelWhenStalledAbove95()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.fuelProgress > 95.0, true);
    QCOMPARE(f.menuGateway.completeRefuelCalls, 1);
}

void RefuelingTrackTest::forcesCompleteRefuelAgainWhileStillStalled()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeRefuelCalls, 1);

    for (int tick = 0; tick < 60; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeRefuelCalls, 2);
}

void RefuelingTrackTest::skipsTheForceWhileTheTruckIsAlreadyLeaving()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.refuelBaselined = true;
    f.ctx.data.loadingStartNotified = true;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = false;
    f.gsxService.refuelCounterGallons = 3200.0;

    for (int tick = 0; tick < 70; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.fuelProgress > 95.0, true);
    QCOMPARE(f.menuGateway.completeRefuelCalls, 0);
}

void RefuelingTrackTest::skipsTheForceWhenGsxAlreadyCompleted()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.refuelBaselined = true;
    f.ctx.data.loadingStartNotified = true;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = true;
    f.gsxService.refuelCounterGallons = 3200.0;

    for (int tick = 0; tick < 70; ++tick)
    {
        static_cast<void>(RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeRefuelCalls, 0);
}

void RefuelingTrackTest::doesNotForceCompleteRefuelBelow95()
{
    TurnaroundStateFixture f;

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
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeRefuelCalls, 0);
}

void RefuelingTrackTest::doesNotForceCompleteRefuelJustUnder95()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.ctx.data.plannedFuelKg = 10000.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.refuelBaselined = true;
    f.ctx.data.loadingStartNotified = true;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;
    f.gsxService.refuelCounterGallons = 3108.5;

    for (int tick = 0; tick < 70; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QVERIFY(f.ctx.data.fuelProgress > 94.0);
    QVERIFY(f.ctx.data.fuelProgress < 95.0);
    QCOMPARE(f.menuGateway.completeRefuelCalls, 0);
}

void RefuelingTrackTest::notifiesAircraftOnceWhenGsxStartsWatching()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Self;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 2000.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.onLoadingStartedCalls, 0);

    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    for (int tick = 0; tick < 3; ++tick)
    {
        QVERIFY(!RefuelingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.aircraft.onLoadingStartedCalls, 1);
    QCOMPARE(f.ctx.data.initialFuelKg, 1000.0);
}

void RefuelingTrackTest::externallyRefueledAircraftMirrorsSimFuel()
{
    TurnaroundStateFixture f;

    f.settings.fuelRateKgs = 1000.0;
    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 1000.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 1000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 1000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);

    f.aircraft.currentFuelKg = 3500.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 3500.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);

    f.aircraft.currentFuelKg = 5990.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingTrackTest::externallyRefueledCompletesOnGsxEvenOffTarget()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 5800.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.ctx.data.initialFuelKg = 1000.0;
    f.ctx.data.loadedFuelKg = 5800.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
}

void RefuelingTrackTest::externallyDefueledAircraftMirrorsSimFuel()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 8000.0;
    f.ctx.data.plannedFuelKg = 6000.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.initialFuelKg, 8000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);

    f.aircraft.currentFuelKg = 7000.0;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 7000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 50.0);

    f.aircraft.currentFuelKg = 6010.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loadedFuelKg, 6010.0);
    QCOMPARE(f.ctx.data.fuelProgress, 100.0);
}

void RefuelingTrackTest::rebaselinesInitialFuelWhenCapturedBeforeSimData()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.currentFuelKg = 5000.0;
    f.ctx.data.plannedFuelKg = 6151.0;
    f.ctx.data.initialFuelKg = 0.0;
    f.ctx.data.loadedFuelKg = 0.0;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    QVERIFY(!RefuelingTrack::Advance(f.ctx));

    QCOMPARE(f.ctx.data.initialFuelKg, 5000.0);
    QCOMPARE(f.ctx.data.fuelProgress, 0.0);
}

void RefuelingTrackTest::staysQuietWhenThePlanSimplyDidNotFit()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.fuelCapacityKg = 9418.0;
    f.aircraft.currentFuelKg = 9418.0;
    f.ctx.data.plannedFuelKg = 10360.0;
    f.ctx.data.initialFuelKg = 2000.0;
    f.ctx.data.loadedFuelKg = 9418.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QVERIFY(!f.ctx.data.fuelDidNotStay);
    QCOMPARE(f.ctx.data.fuelShortfallKg, 0.0);
}

void RefuelingTrackTest::warnsWhenTheAircraftRefusedFuelThatWasWritten()
{
    TurnaroundStateFixture f;

    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.fuelCapacityKg = 10360.0;
    f.aircraft.currentFuelKg = 9418.0;
    f.ctx.data.plannedFuelKg = 10360.0;
    f.ctx.data.initialFuelKg = 2000.0;
    f.ctx.data.loadedFuelKg = 9418.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;

    QVERIFY(RefuelingTrack::Advance(f.ctx));
    QVERIFY(f.ctx.data.fuelDidNotStay);
    QCOMPARE(f.ctx.data.fuelShortfallKg, 942.0);
}

QTEST_APPLESS_MAIN(RefuelingTrackTest)

#include "tst_refueling_track.moc"
