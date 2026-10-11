#include <QtTest/QTest>

#include <algorithm>
#include "doubles/FakePmdgDoorSource.h"
#include "../src/infrastructure/pmdg/PmdgDoorReconciler.h"

namespace
{
    constexpr int kSlots = 16;

    int ToggleCount(const FakePmdgDoorSource& source, const int slot)
    {
        return static_cast<int>(std::ranges::count(source.toggled, slot));
    }
}

class PmdgDoorReconcilerTest final : public QObject
{
    Q_OBJECT

private slots:
    static void doorWithoutSlotIsIgnored();
    static void unavailableDoorIsLeftUntouched();
    static void movingDoorIsLeftUntouched();
    static void closeArrivingMidOpeningWaitsForTheDoorToSettle();
    static void commandIsSentOnceWhenTheDoorDisagrees();
    static void agreeingDoorIsNeverCommanded();
    static void retryStopsAtTheAttemptCap();
    static void unknownDoorIsCommandedOnTheEdgeAndNeverRetried();
    static void closedBaselineDoesNotCommandAnUnreadDoorShut();
    static void closingUsesTheSlotThatWasOpened();
    static void stuckOnlyWhenTheDoorIsWantedOpenAndRefuses();
    static void anUnreadDoorWaitsWhileTheReadingsArePending();
    static void aDoorReadOpenAfterTheWaitIsNeverToggled();
    static void aDoorReadClosedAfterTheWaitIsOpenedOnce();
    static void anUnreadDoorIsCommandedOnceTheWaitEnds();
    static void pendingReadingsDoNotHoldADoorThatIsRead();
    static void anIdleReconcilerHasNothingToRemember();
    static void theDesiredTargetsComeBackWithoutTogglingAnUnreadDoor();
    static void aRestoredTargetStillRetriesADoorReadAgainstIt();
    static void aRestoredOpenedSlotIsTheOneThatCloses();
    static void garbageInTheMemoryIsIgnored();
};

void PmdgDoorReconcilerTest::doorWithoutSlotIsIgnored()
{
    FakePmdgDoorSource source;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    reconciler.Reconcile();

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::unavailableDoorIsLeftUntouched()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unavailable;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::movingDoorIsLeftUntouched()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Moving;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::closeArrivingMidOpeningWaitsForTheDoorToSettle()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::AftPax] = 7;
    source.observations[7] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::AftPax, true);
    reconciler.Reconcile();
    QCOMPARE(ToggleCount(source, 7), 1);

    source.observations[7] = DoorObservation::Moving;
    reconciler.SetDesired(GsxDoor::AftPax, false);
    reconciler.Reconcile();
    reconciler.Reconcile();
    QCOMPARE(ToggleCount(source, 7), 1);

    source.observations[7] = DoorObservation::Open;
    reconciler.Reconcile();
    QCOMPARE(ToggleCount(source, 7), 2);

    source.observations[7] = DoorObservation::Moving;
    for (int tick = 0; tick < 10; ++tick)
    {
        reconciler.Reconcile();
    }
    source.observations[7] = DoorObservation::Closed;
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(source, 7), 2);
}

void PmdgDoorReconcilerTest::commandIsSentOnceWhenTheDoorDisagrees()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(source, 3), 1);
}

void PmdgDoorReconcilerTest::agreeingDoorIsNeverCommanded()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Open;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::retryStopsAtTheAttemptCap()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 60; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(ToggleCount(source, 3), 3);
}

void PmdgDoorReconcilerTest::unknownDoorIsCommandedOnTheEdgeAndNeverRetried()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdCargo] = 5;
    source.observations[5] = DoorObservation::Unknown;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdCargo, true);
    for (int tick = 0; tick < 60; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(ToggleCount(source, 5), 1);
}

void PmdgDoorReconcilerTest::closedBaselineDoesNotCommandAnUnreadDoorShut()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unknown;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, false);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::closingUsesTheSlotThatWasOpened()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 0;
    source.observations[0] = DoorObservation::Closed;
    source.observations[2] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    reconciler.Reconcile();
    source.observations[0] = DoorObservation::Open;
    source.doorSlots[GsxDoor::FwdPax] = 2;
    reconciler.SetDesired(GsxDoor::FwdPax, false);
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(source, 0), 2);
    QCOMPARE(ToggleCount(source, 2), 0);
}

void PmdgDoorReconcilerTest::stuckOnlyWhenTheDoorIsWantedOpenAndRefuses()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdCargo] = 5;
    source.observations[5] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);

    reconciler.SetDesired(GsxDoor::FwdCargo, true);
    QVERIFY(!reconciler.IsStuck(5));

    for (int tick = 0; tick < 60; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(reconciler.IsStuck(5));
    QVERIFY(!reconciler.IsStuck(4));
}

void PmdgDoorReconcilerTest::anUnreadDoorWaitsWhileTheReadingsArePending()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unknown;
    source.readingsPending = true;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::aDoorReadOpenAfterTheWaitIsNeverToggled()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unknown;
    source.readingsPending = true;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 2; ++tick)
    {
        reconciler.Reconcile();
    }

    source.observations[3] = DoorObservation::Open;
    source.readingsPending = false;
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

void PmdgDoorReconcilerTest::aDoorReadClosedAfterTheWaitIsOpenedOnce()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unknown;
    source.readingsPending = true;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    reconciler.Reconcile();

    source.observations[3] = DoorObservation::Closed;
    source.readingsPending = false;
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(source, 3), 1);
}

void PmdgDoorReconcilerTest::anUnreadDoorIsCommandedOnceTheWaitEnds()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Unknown;
    source.readingsPending = true;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    for (int tick = 0; tick < 5; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());

    source.readingsPending = false;
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(ToggleCount(source, 3), 1);
}

void PmdgDoorReconcilerTest::pendingReadingsDoNotHoldADoorThatIsRead()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Closed;
    source.readingsPending = true;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);

    reconciler.SetDesired(GsxDoor::FwdPax, true);
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(source, 3), 1);
}

void PmdgDoorReconcilerTest::anIdleReconcilerHasNothingToRemember()
{
    FakePmdgDoorSource source;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Closed);
    MemoryBag memory;

    reconciler.AppendMemory(memory);

    QVERIFY(memory.Entries().empty());
}

void PmdgDoorReconcilerTest::theDesiredTargetsComeBackWithoutTogglingAnUnreadDoor()
{
    FakePmdgDoorSource dead;
    dead.doorSlots[GsxDoor::FwdPax] = 3;
    dead.observations[3] = DoorObservation::Closed;
    PmdgDoorReconciler deadReconciler(dead, kSlots, DoorBaseline::Closed);
    deadReconciler.SetDesired(GsxDoor::FwdPax, true);
    deadReconciler.Reconcile();
    MemoryBag memory;
    deadReconciler.AppendMemory(memory);

    FakePmdgDoorSource born;
    born.doorSlots[GsxDoor::FwdPax] = 3;
    born.observations[3] = DoorObservation::Unknown;
    PmdgDoorReconciler reconciler(born, kSlots, DoorBaseline::Closed);
    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 40; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(born.toggled.empty());
}

void PmdgDoorReconcilerTest::aRestoredTargetStillRetriesADoorReadAgainstIt()
{
    FakePmdgDoorSource dead;
    dead.doorSlots[GsxDoor::FwdPax] = 3;
    dead.observations[3] = DoorObservation::Closed;
    PmdgDoorReconciler deadReconciler(dead, kSlots, DoorBaseline::Closed);
    deadReconciler.SetDesired(GsxDoor::FwdPax, true);
    deadReconciler.Reconcile();
    MemoryBag memory;
    deadReconciler.AppendMemory(memory);

    FakePmdgDoorSource born;
    born.doorSlots[GsxDoor::FwdPax] = 3;
    born.observations[3] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(born, kSlots, DoorBaseline::Closed);
    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 5; ++tick)
    {
        reconciler.Reconcile();
    }

    QCOMPARE(ToggleCount(born, 3), 1);
}

void PmdgDoorReconcilerTest::aRestoredOpenedSlotIsTheOneThatCloses()
{
    FakePmdgDoorSource dead;
    dead.doorSlots[GsxDoor::FwdPax] = 0;
    dead.observations[0] = DoorObservation::Closed;
    PmdgDoorReconciler deadReconciler(dead, kSlots, DoorBaseline::Unknown);
    deadReconciler.SetDesired(GsxDoor::FwdPax, true);
    deadReconciler.Reconcile();
    MemoryBag memory;
    deadReconciler.AppendMemory(memory);

    FakePmdgDoorSource born;
    born.doorSlots[GsxDoor::FwdPax] = 2;
    born.observations[0] = DoorObservation::Open;
    born.observations[2] = DoorObservation::Closed;
    PmdgDoorReconciler reconciler(born, kSlots, DoorBaseline::Unknown);
    reconciler.RestoreMemory(memory);
    reconciler.SetDesired(GsxDoor::FwdPax, false);
    reconciler.Reconcile();

    QCOMPARE(ToggleCount(born, 0), 1);
    QCOMPARE(ToggleCount(born, 2), 0);
}

void PmdgDoorReconcilerTest::garbageInTheMemoryIsIgnored()
{
    FakePmdgDoorSource source;
    source.doorSlots[GsxDoor::FwdPax] = 3;
    source.observations[3] = DoorObservation::Open;
    PmdgDoorReconciler reconciler(source, kSlots, DoorBaseline::Unknown);
    MemoryBag memory;
    memory.PutNumber("pmdgDoors.desired.3", 7.0);

    reconciler.RestoreMemory(memory);
    for (int tick = 0; tick < 20; ++tick)
    {
        reconciler.Reconcile();
    }

    QVERIFY(source.toggled.empty());
}

QTEST_APPLESS_MAIN(PmdgDoorReconcilerTest)

#include "tst_pmdg_door_reconciler.moc"
