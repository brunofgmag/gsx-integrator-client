#include <QtTest/QTest>

#include <string>
#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/PlaceArrivalGroundEquipmentState.h"

namespace
{
    constexpr int kConfirmationGiveUpTicks = 30;
    constexpr int kEquipmentGiveUpTicks = 240;
    constexpr auto kGsxNeverConfirmed = "GSX never confirmed the ground power; moving on to the deboarding";
    constexpr auto kGaveUpOnTheEquipment =
        "The ground power or the chocks were not in place after 4 minutes; moving on to the deboarding";

    bool HoldsFor(PlaceArrivalGroundEquipmentState& state, TurnaroundStateFixture& f, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            ++f.ctx.data.stateTickCount;
            if (state.Evaluate(f.ctx).has_value())
            {
                return false;
            }
        }

        return true;
    }
}

class PlaceArrivalGroundEquipmentStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenOptionDisabled();
    static void skipsWhenSettingsAreNull();
    static void waitsWhileEnginesRunning();
    static void waitsWhileAircraftNotHeldInPlace();
    static void chocksAloneHoldTheAircraftEnough();
    static void placesChocksAndGpuWhenParked();
    static void commandsAircraftGpuWhenControlSupported();
    static void advancesWithoutToggleWhenGpuAlreadyConnected();
    static void waitsWhileGpuStatusUnknown();
    static void skipsChocksWhenUnsupported();
    static void placesChocksWithoutGpuAndAdvances();
    static void chocksWithoutGpuWaitForEnginesOff();
    static void chocksWithoutGpuWaitForTheAircraftToBeHeld();
    static void callsGpuWithoutPlacingChocks();
    static void closesAllDoorsEvenWhenOptionDisabled();
    static void closesAllDoorsOnlyOnce();
    static void releasesTheDepartureDoorHoldOnArrival();
    static void doesNotToggleAGpuServiceThatIsAlreadyInProgress();
    static void commandsTheAircraftGpuEvenWhileAGsxGpuServiceIsInProgress();
    static void holdsThePhaseAfterTheRequestUntilGsxConfirms();
    static void movesOnWhenGsxStartsTheGroundPowerService();
    static void movesOnWhenTheGroundPowerReadsConnectedAfterTheRequest();
    static void givesUpWaitingForGsxExactlyAtTheBound();
    static void aResumedRequestWaitsForGsxAndGivesUpAtTheBound();
    static void theBoundCountsFromTheRequestAndNotFromTheStateEntry();
    static void aResumedRequestMovesOnWhenGsxConfirms();
    static void aResumedAircraftControlledRequestMovesOnAtOnce();
    static void goesStraightToDeboardingWithBothOptionsOffAndNoChocksSupport();
    static void goesStraightToDeboardingWithBothOptionsOffEvenIfTheAircraftReadsTheChocks();
    static void leavesOnTheFirstTickWhenTheGpuIsConnectedAndNoChocksAreWanted();
    static void waitsForTheChocksReadingBeforeTheDeboarding();
    static void waitsUntilTheChocksCommandIsAcceptedWhenTheAircraftHasNoReading();
    static void doesNotWaitForTheChocksWhenTheSettingIsOffEvenIfTheAircraftReadsThemRemoved();
    static void holdsTheAircraftControlledGpuExitUntilTheChocksAreInPlace();
    static void holdsTheGsxConfirmedGpuExitUntilTheChocksAreInPlace();
    static void doesNotLogTheMissingGsxConfirmationWhileTheChocksHoldThePhase();
    static void theEquipmentCeilingDoesNotCountWhileTheEnginesRun();
    static void theEquipmentCeilingDoesNotCountWhileTheAircraftIsNotHeld();
    static void giveUpAtTheCeilingWhileTheGroundPowerNeverReads();
    static void giveUpAtTheCeilingWhileTheChocksNeverComeIn();
    static void doesNotFlagTheCeilingWhenTheEquipmentIsInPlace();
    static void doesNotWaitForTheChocksWhenTheSettingIsOffOnAnAircraftThatOnlyReadsThem();
    static void waitsForTheChocksReadingWhenTheSettingIsOnAndTheAircraftOnlyReadsThem();
    static void stillRequestsTheGsxGroundPowerWhileTheChocksAreOut();
    static void theGsxConfirmationCountsFromTheChocksAndNotFromTheHold();
    static void theEquipmentCeilingFiresBeforeTheConfirmationWhenTheChocksArriveLate();
};

void PlaceArrivalGroundEquipmentStateTest::skipsWhenOptionDisabled()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::skipsWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.ctx.settings = nullptr;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileEnginesRunning()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileAircraftNotHeldInPlace()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.heldInPlace = false;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::chocksAloneHoldTheAircraftEnough()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.heldInPlace = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void PlaceArrivalGroundEquipmentStateTest::placesChocksAndGpuWhenParked()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.aircraft.chocksPlaced);
    QVERIFY(f.ctx.data.arrivalChocksPlaced);
}

void PlaceArrivalGroundEquipmentStateTest::commandsAircraftGpuWhenControlSupported()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
    QVERIFY(f.aircraft.groundPowerOn);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::advancesWithoutToggleWhenGpuAlreadyConnected()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileGpuStatusUnknown()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::skipsChocksWhenUnsupported()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::placesChocksWithoutGpuAndAdvances()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.ctx.data.arrivalChocksPlaced);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 0);
    QVERIFY(!f.ctx.data.arrivalGpuRequested);
}

void PlaceArrivalGroundEquipmentStateTest::chocksWithoutGpuWaitForEnginesOff()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::chocksWithoutGpuWaitForTheAircraftToBeHeld()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.heldInPlace = false;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::callsGpuWithoutPlacingChocks()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = false;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
    QVERIFY(!f.ctx.data.arrivalChocksPlaced);
}

void PlaceArrivalGroundEquipmentStateTest::closesAllDoorsEvenWhenOptionDisabled()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
    QVERIFY(f.ctx.data.arrivalDoorsClosed);
}

void PlaceArrivalGroundEquipmentStateTest::releasesTheDepartureDoorHoldOnArrival()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.aircraft.doorsHeldClosed = true;
    f.settings.callGpuOnArrival = false;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.aircraft.doorsHeldClosed);
}

void PlaceArrivalGroundEquipmentStateTest::closesAllDoorsOnlyOnce()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
}

void PlaceArrivalGroundEquipmentStateTest::doesNotToggleAGpuServiceThatIsAlreadyInProgress()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.gsxService.gpuInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.logger.messages.size(), 1U);
}

void PlaceArrivalGroundEquipmentStateTest::commandsTheAircraftGpuEvenWhileAGsxGpuServiceIsInProgress()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;
    f.gsxService.gpuInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::holdsThePhaseAfterTheRequestUntilGsxConfirms()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);

    f.ctx.data.stateTickCount = 1;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::movesOnWhenGsxStartsTheGroundPowerService()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.gpuInProgress = true;
    f.ctx.data.stateTickCount = 1;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::movesOnWhenTheGroundPowerReadsConnectedAfterTheRequest()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.ctx.data.stateTickCount = 1;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::givesUpWaitingForGsxExactlyAtTheBound()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks - 1));
    QCOMPARE(f.logger.messages.size(), 0U);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGsxNeverConfirmed));
}

void PlaceArrivalGroundEquipmentStateTest::theBoundCountsFromTheRequestAndNotFromTheStateEntry()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.ctx.data.stateTickCount = kConfirmationGiveUpTicks * 4;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);

    f.aircraft.parkingBrakeSet = true;

    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks));
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::aResumedRequestWaitsForGsxAndGivesUpAtTheBound()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.ctx.data.arrivalGpuRequested = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks - 1));
    QCOMPARE(f.logger.messages.size(), 0U);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGsxNeverConfirmed));
}

void PlaceArrivalGroundEquipmentStateTest::aResumedRequestMovesOnWhenGsxConfirms()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;
    f.ctx.data.arrivalGpuRequested = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.gpuInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::aResumedAircraftControlledRequestMovesOnAtOnce()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;
    f.ctx.data.arrivalGpuRequested = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 0);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::goesStraightToDeboardingWithBothOptionsOffAndNoChocksSupport()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::goesStraightToDeboardingWithBothOptionsOffEvenIfTheAircraftReadsTheChocks()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = false;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

void PlaceArrivalGroundEquipmentStateTest::leavesOnTheFirstTickWhenTheGpuIsConnectedAndNoChocksAreWanted()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = false;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsForTheChocksReadingBeforeTheDeboarding()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.chocksStatus = ChocksStatus::Unknown;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

void PlaceArrivalGroundEquipmentStateTest::waitsUntilTheChocksCommandIsAcceptedWhenTheAircraftHasNoReading()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.supportsChocksControl = true;
    f.aircraft.refusesChocks = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.arrivalChocksPlaced);

    f.aircraft.refusesChocks = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(f.ctx.data.arrivalChocksPlaced);
}

void PlaceArrivalGroundEquipmentStateTest::doesNotWaitForTheChocksWhenTheSettingIsOffEvenIfTheAircraftReadsThemRemoved()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = false;
    f.aircraft.heldInPlace = true;
    f.aircraft.supportsChocksControl = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::holdsTheAircraftControlledGpuExitUntilTheChocksAreInPlace()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
}

void PlaceArrivalGroundEquipmentStateTest::holdsTheGsxConfirmedGpuExitUntilTheChocksAreInPlace()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.gsxService.gpuInProgress = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

void PlaceArrivalGroundEquipmentStateTest::doesNotLogTheMissingGsxConfirmationWhileTheChocksHoldThePhase()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks * 2));
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::theEquipmentCeilingDoesNotCountWhileTheEnginesRun()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.engineRunning = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    QVERIFY(HoldsFor(state, f, kEquipmentGiveUpTicks * 2));
    QCOMPARE(f.ctx.data.arrivalEquipmentTicks, 0);
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::theEquipmentCeilingDoesNotCountWhileTheAircraftIsNotHeld()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.heldInPlace = false;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    QVERIFY(HoldsFor(state, f, kEquipmentGiveUpTicks * 2));
    QCOMPARE(f.ctx.data.arrivalEquipmentTicks, 0);

    f.aircraft.heldInPlace = true;

    QVERIFY(HoldsFor(state, f, 10));
    QCOMPARE(f.ctx.data.arrivalEquipmentTicks, 10);
}

void PlaceArrivalGroundEquipmentStateTest::giveUpAtTheCeilingWhileTheGroundPowerNeverReads()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    QVERIFY(HoldsFor(state, f, kEquipmentGiveUpTicks - 1));
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 0U);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGaveUpOnTheEquipment));
}

void PlaceArrivalGroundEquipmentStateTest::giveUpAtTheCeilingWhileTheChocksNeverComeIn()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(HoldsFor(state, f, kEquipmentGiveUpTicks - 1));
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGaveUpOnTheEquipment));
}

void PlaceArrivalGroundEquipmentStateTest::doesNotFlagTheCeilingWhenTheEquipmentIsInPlace()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.chocksStatus = ChocksStatus::Placed;
    f.ctx.data.arrivalEquipmentTicks = kEquipmentGiveUpTicks - 1;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 0U);
}

void PlaceArrivalGroundEquipmentStateTest::doesNotWaitForTheChocksWhenTheSettingIsOffOnAnAircraftThatOnlyReadsThem()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = false;
    f.aircraft.heldInPlace = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsForTheChocksReadingWhenTheSettingIsOnAndTheAircraftOnlyReadsThem()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
    QVERIFY(!f.ctx.data.arrivalChocksPlaced);

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::stillRequestsTheGsxGroundPowerWhileTheChocksAreOut()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);

    QVERIFY(HoldsFor(state, f, 10));
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);

    f.gsxService.gpuInProgress = true;
    f.aircraft.chocksStatus = ChocksStatus::Placed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void PlaceArrivalGroundEquipmentStateTest::theGsxConfirmationCountsFromTheChocksAndNotFromTheHold()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks * 3));
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 0U);

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks));
    QCOMPARE(f.logger.messages.size(), 0U);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGsxNeverConfirmed));
}

void PlaceArrivalGroundEquipmentStateTest::theEquipmentCeilingFiresBeforeTheConfirmationWhenTheChocksArriveLate()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.settings.placeChocksOnArrival = true;
    f.aircraft.heldInPlace = true;
    f.aircraft.chocksStatus = ChocksStatus::Removed;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(HoldsFor(state, f, kEquipmentGiveUpTicks - kConfirmationGiveUpTicks));

    f.aircraft.chocksStatus = ChocksStatus::Placed;

    QVERIFY(HoldsFor(state, f, kConfirmationGiveUpTicks - 1));
    QVERIFY(!f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 0U);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QVERIFY(f.ctx.data.groundEquipmentTimedOut);
    QCOMPARE(f.logger.messages.size(), 1U);
    QCOMPARE(f.logger.messages.front(), std::string(kGaveUpOnTheEquipment));
}

QTEST_APPLESS_MAIN(PlaceArrivalGroundEquipmentStateTest)

#include "tst_place_arrival_ground_equipment_state.moc"
