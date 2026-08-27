#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingReadyToPushState.h"

class WaitingReadyToPushStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilAircraftReady();
    static void advancesWhenReady();
    static void holdsWhileADoorReportsOpen();
    static void advancesOnceTheOpenDoorCloses();
    static void advancesWhenNoDoorAnswers();
    static void holdsWhileTheParkingBrakeIsReleased();
    static void advancingByReadingCarriesTheReadingOrigin();
    static void theSmartSwitchUnlocksTheDoorHold();
    static void theUnlockCarriesThePilotOrigin();
    static void theSmartSwitchDoesNotUnlockTheParkingBrakeHold();
    static void theUnlockConsumesTheTouchOnce();
    static void anOpenGateLeavesTheTouchForTheNextPhase();
};

void WaitingReadyToPushStateTest::holdsUntilAircraftReady()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.parkingBrakeSet = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingReadyToPushStateTest::advancesWhenReady()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitCatering);
}

void WaitingReadyToPushStateTest::holdsWhileADoorReportsOpen()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AnyOpen;

    for (int tick = 0; tick < 600; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }
}

void WaitingReadyToPushStateTest::advancesOnceTheOpenDoorCloses()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AnyOpen;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.aircraft.doorStatus = DoorStatus::AllClosed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitCatering);
}

void WaitingReadyToPushStateTest::advancesWhenNoDoorAnswers()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::Unknown;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitCatering);
}

void WaitingReadyToPushStateTest::holdsWhileTheParkingBrakeIsReleased()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.doorStatus = DoorStatus::AllClosed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.aircraft.parkingBrakeSet = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitCatering);
}

void WaitingReadyToPushStateTest::advancingByReadingCarriesTheReadingOrigin()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AllClosed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->origin, TransitionOrigin::Reading);
}

void WaitingReadyToPushStateTest::theSmartSwitchUnlocksTheDoorHold()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AnyOpen;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.ctx.pilotTouched = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitCatering);
}

void WaitingReadyToPushStateTest::theUnlockCarriesThePilotOrigin()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AnyOpen;
    f.ctx.pilotTouched = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->origin, TransitionOrigin::Pilot);
}

void WaitingReadyToPushStateTest::theSmartSwitchDoesNotUnlockTheParkingBrakeHold()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.doorStatus = DoorStatus::AllClosed;
    f.ctx.pilotTouched = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingReadyToPushStateTest::theUnlockConsumesTheTouchOnce()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AnyOpen;
    f.ctx.pilotTouched = true;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.pilotTouched);
}

void WaitingReadyToPushStateTest::anOpenGateLeavesTheTouchForTheNextPhase()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.doorStatus = DoorStatus::AllClosed;
    f.ctx.pilotTouched = true;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.pilotTouched);
}

QTEST_APPLESS_MAIN(WaitingReadyToPushStateTest)

#include "tst_waiting_ready_to_push_state.moc"
