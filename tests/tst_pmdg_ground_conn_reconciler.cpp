#include <QtTest/QTest>

#include <algorithm>
#include "doubles/FakePmdgTabletGateway.h"
#include "../src/infrastructure/pmdg/PmdgGroundConnReconciler.h"
#include "../src/infrastructure/pmdg/PmdgGroundSource.h"

namespace
{
    constexpr auto kChocksRequest = "wheel_chocks";
    constexpr auto kGroundPowerRequest = "ground_power";
    constexpr auto kPassengerEntryRequest = "pax_entree";
    constexpr auto kOwnStairsRequest = "stairs_1l";

    class FakeGroundSource final : public PmdgGroundSource
    {
    public:
        bool aircraftPowered = false;
        bool groundPowerPresent = false;
        bool chocksSet = false;
        bool chocksArrived = true;

        [[nodiscard]] bool HasAircraftPower() const override { return aircraftPowered; }
        [[nodiscard]] bool GroundPowerPresent() const override { return groundPowerPresent; }
        [[nodiscard]] bool ChocksSet() const override { return chocksArrived && chocksSet; }
        [[nodiscard]] bool ChocksReadingArrived() const override { return chocksArrived; }
    };

    int RequestCount(const FakePmdgTabletGateway& tablet, const char* key)
    {
        return static_cast<int>(std::ranges::count(tablet.groundConnRequests, key));
    }

    int VehicleRequestCount(const FakePmdgTabletGateway& tablet, const char* key)
    {
        return static_cast<int>(std::ranges::count(tablet.groundVehicleRequests, key));
    }
}

class PmdgGroundConnReconcilerTest final : public QObject
{
    Q_OBJECT

private slots:
    static void quietUntilSomethingIsAsked();
    static void chocksAreRequestedUntilTheyAppear();
    static void chocksStopAtTheAttemptCap();
    static void groundPowerAlreadyPresentIsNeverRequested();
    static void groundPowerIsRequestedUntilItAppears();
    static void groundPowerIsNotPressedAgainWhileConnecting();
    static void groundPowerIsPressedAgainOnceTheTransitEnds();
    static void reversingTheRequestRearmsTheRetry();
    static void passengerEntryIsNotPressedBeforeTheAircraftAnswers();
    static void passengerEntryIsPressedUntilTheAircraftSaysJetway();
    static void passengerEntryStopsAtTheAttemptCap();
    static void theChocksDesireDiesOnceItIsSatisfied();
    static void theGroundPowerDesireDiesOnceItIsSatisfied();
    static void theChocksWaitLongerThanTheTabletPublishPeriod();
    static void ownStairsAreReleasedWhereTheJetwayIsInhibited();
    static void theJetwayButtonIsLeftAloneWhereTheJetwayIsInhibited();
    static void ownStairsAreLeftAloneWhileTheyAreAlreadyGone();
    static void nothingPendingMeansNothingToRemember();
    static void aSatisfiedDesireIsNotRemembered();
    static void restoredRequestsAreMeasuredAgainstTheLiveReading();
    static void restoredRequestsAreSentWhenTheLiveReadingDisagrees();
    static void aRestoredPassengerEntryKeepsPressingTheButton();
    static void aRestoredPlaceRequestWaitsForTheChocksReadingAndFindsThemSet();
    static void aRestoredRemoveRequestIsNotSatisfiedByTheDefaultReading();
    static void anOrdinaryChocksRequestDoesNotWaitForTheReading();
    static void aSavedValueThatIsNotAFlagIsNotRestored();
};

void PmdgGroundConnReconcilerTest::quietUntilSomethingIsAsked()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundConnRequests.empty());
}

void PmdgGroundConnReconcilerTest::chocksAreRequestedUntilTheyAppear()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);

    source.chocksSet = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::chocksStopAtTheAttemptCap()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 10);
}

void PmdgGroundConnReconcilerTest::groundPowerAlreadyPresentIsNeverRequested()
{
    FakeGroundSource source;
    source.groundPowerPresent = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundConnRequests.empty());
}

void PmdgGroundConnReconcilerTest::groundPowerIsRequestedUntilItAppears()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);

    source.groundPowerPresent = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);
}

void PmdgGroundConnReconcilerTest::reversingTheRequestRearmsTheRetry()
{
    FakeGroundSource source;
    source.chocksSet = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();
    QVERIFY(tablet.groundConnRequests.empty());

    reconciler.SetChocks(false);
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::passengerEntryIsNotPressedBeforeTheAircraftAnswers()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundConnRequests.empty());
}

void PmdgGroundConnReconcilerTest::passengerEntryIsPressedUntilTheAircraftSaysJetway()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = false;
    tablet.passengerEntryJetway = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 1);

    for (int tick = 0; tick < 4; ++tick)
    {
        reconciler.Reconcile();
    }
    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 1);

    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 2);

    tablet.passengerEntryJetway = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 2);
}

void PmdgGroundConnReconcilerTest::passengerEntryStopsAtTheAttemptCap()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = false;
    tablet.passengerEntryJetway = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 10);
}

void PmdgGroundConnReconcilerTest::groundPowerIsNotPressedAgainWhileConnecting()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);

    tablet.groundConnMoving.emplace(kGroundPowerRequest);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);
}

void PmdgGroundConnReconcilerTest::groundPowerIsPressedAgainOnceTheTransitEnds()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(true);
    reconciler.Reconcile();
    tablet.groundConnMoving.emplace(kGroundPowerRequest);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    tablet.groundConnMoving.clear();
    for (int tick = 0; tick < 6; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 2);
}

void PmdgGroundConnReconcilerTest::theChocksDesireDiesOnceItIsSatisfied()
{
    FakeGroundSource source;
    source.chocksSet = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(false);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);

    source.chocksSet = false;
    reconciler.Reconcile();

    source.chocksSet = true;
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::theGroundPowerDesireDiesOnceItIsSatisfied()
{
    FakeGroundSource source;
    source.groundPowerPresent = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetGroundPower(false);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);

    source.groundPowerPresent = false;
    reconciler.Reconcile();

    source.groundPowerPresent = true;
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);
}

void PmdgGroundConnReconcilerTest::theChocksWaitLongerThanTheTabletPublishPeriod()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);

    for (int tick = 0; tick < 9; ++tick)
    {
        reconciler.Reconcile();
    }
    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);

    reconciler.Reconcile();
    QCOMPARE(RequestCount(tablet, kChocksRequest), 2);
}

void PmdgGroundConnReconcilerTest::ownStairsAreReleasedWhereTheJetwayIsInhibited()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = true;
    tablet.ownStairsDeployed = true;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    reconciler.Reconcile();

    QCOMPARE(VehicleRequestCount(tablet, kOwnStairsRequest), 1);

    tablet.ownStairsDeployed = false;
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(VehicleRequestCount(tablet, kOwnStairsRequest), 1);
}

void PmdgGroundConnReconcilerTest::theJetwayButtonIsLeftAloneWhereTheJetwayIsInhibited()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = true;
    tablet.ownStairsDeployed = true;
    tablet.passengerEntryJetway = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    for (int tick = 0; tick < 200; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 0);
    QCOMPARE(VehicleRequestCount(tablet, kOwnStairsRequest), 10);
}

void PmdgGroundConnReconcilerTest::ownStairsAreLeftAloneWhileTheyAreAlreadyGone()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = true;
    tablet.ownStairsDeployed = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetPassengerEntryJetway();
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(tablet.groundVehicleRequests.empty());
}

namespace
{
    MemoryBag MemoryWithEverythingPending()
    {
        FakeGroundSource source;
        FakePmdgTabletGateway tablet;
        PmdgGroundConnReconciler reconciler(source, tablet);
        reconciler.SetChocks(true);
        reconciler.SetGroundPower(true);
        reconciler.SetPassengerEntryJetway();
        MemoryBag memory;
        reconciler.AppendMemory(memory);

        return memory;
    }
}

void PmdgGroundConnReconcilerTest::nothingPendingMeansNothingToRemember()
{
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);
    MemoryBag memory;

    reconciler.AppendMemory(memory);

    QVERIFY(memory.Entries().empty());
}

void PmdgGroundConnReconcilerTest::aSatisfiedDesireIsNotRemembered()
{
    FakeGroundSource source;
    source.chocksSet = true;
    source.groundPowerPresent = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);
    reconciler.SetChocks(true);
    reconciler.SetGroundPower(true);
    reconciler.Reconcile();
    MemoryBag memory;

    reconciler.AppendMemory(memory);

    QVERIFY(memory.Entries().empty());
}

void PmdgGroundConnReconcilerTest::restoredRequestsAreMeasuredAgainstTheLiveReading()
{
    const MemoryBag memory = MemoryWithEverythingPending();
    FakeGroundSource source;
    source.chocksSet = true;
    source.groundPowerPresent = true;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 0);
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 0);
}

void PmdgGroundConnReconcilerTest::restoredRequestsAreSentWhenTheLiveReadingDisagrees()
{
    const MemoryBag memory = MemoryWithEverythingPending();
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
    QCOMPARE(RequestCount(tablet, kGroundPowerRequest), 1);
}

void PmdgGroundConnReconcilerTest::aRestoredPassengerEntryKeepsPressingTheButton()
{
    const MemoryBag memory = MemoryWithEverythingPending();
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    tablet.jetwayInhibited = false;
    tablet.passengerEntryJetway = false;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kPassengerEntryRequest), 1);
}

namespace
{
    MemoryBag MemoryWithChocksPending(const bool placed)
    {
        FakeGroundSource source;
        FakePmdgTabletGateway tablet;
        PmdgGroundConnReconciler reconciler(source, tablet);
        reconciler.SetChocks(placed);
        MemoryBag memory;
        reconciler.AppendMemory(memory);

        return memory;
    }
}

void PmdgGroundConnReconcilerTest::aRestoredPlaceRequestWaitsForTheChocksReadingAndFindsThemSet()
{
    const MemoryBag memory = MemoryWithChocksPending(true);
    FakeGroundSource source;
    source.chocksSet = true;
    source.chocksArrived = false;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 0);

    source.chocksArrived = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(RequestCount(tablet, kChocksRequest), 0);

    MemoryBag after;
    reconciler.AppendMemory(after);

    QVERIFY(after.Entries().empty());
}

void PmdgGroundConnReconcilerTest::aRestoredRemoveRequestIsNotSatisfiedByTheDefaultReading()
{
    const MemoryBag memory = MemoryWithChocksPending(false);
    FakeGroundSource source;
    source.chocksSet = true;
    source.chocksArrived = false;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    MemoryBag whileWaiting;
    reconciler.AppendMemory(whileWaiting);

    QVERIFY(!whileWaiting.Entries().empty());
    QCOMPARE(RequestCount(tablet, kChocksRequest), 0);

    source.chocksArrived = true;
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::anOrdinaryChocksRequestDoesNotWaitForTheReading()
{
    FakeGroundSource source;
    source.chocksArrived = false;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.SetChocks(true);
    reconciler.Reconcile();

    QCOMPARE(RequestCount(tablet, kChocksRequest), 1);
}

void PmdgGroundConnReconcilerTest::aSavedValueThatIsNotAFlagIsNotRestored()
{
    MemoryBag memory;
    memory.PutNumber("pmdgGround.pendingChocks", 2.0);
    memory.PutNumber("pmdgGround.pendingGroundPower", 2.0);
    FakeGroundSource source;
    FakePmdgTabletGateway tablet;
    PmdgGroundConnReconciler reconciler(source, tablet);

    reconciler.RestoreMemory(memory);
    reconciler.Reconcile();
    MemoryBag after;
    reconciler.AppendMemory(after);

    QVERIFY(tablet.groundConnRequests.empty());
    QVERIFY(after.Entries().empty());
}

QTEST_APPLESS_MAIN(PmdgGroundConnReconcilerTest)

#include "tst_pmdg_ground_conn_reconciler.moc"
