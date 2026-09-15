#include <QtTest/QTest>

#include <array>

#include "TestDoubles.h"
#include "../src/infrastructure/gsx/GsxStateService.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    using namespace gsx::lvars;

    constexpr auto kSimOnGround = "SIM ON GROUND";

    struct ServiceStateLVar
    {
        const char* lvar;
        GsxState state;
    };

    constexpr std::array kServicesThatEndThroughCompleted = {
        ServiceStateLVar{kRefuelingState, GsxState::Refueling},
        ServiceStateLVar{kBoardingState, GsxState::Boarding},
        ServiceStateLVar{kDeboardingState, GsxState::Deboarding},
    };

    void ObserveFor(GsxStateService& gsx, FakeVariableGateway& gateway, const char* stateLVar,
                    const double couatlStarted, const double state, const int ticks)
    {
        gateway.lvars[kCouatlStarted] = couatlStarted;
        gateway.lvars[stateLVar] = state;

        for (int tick = 0; tick < ticks; ++tick)
        {
            gsx.Observe();
        }
    }
}

class GsxInterfaceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void availabilityFollowsCouatlFlag();
    static void mapsServiceStateLVars();
    static void recordsExplicitCompletedState();
    static void aServiceTheCouatlDropsIsNotRecordedAsCompleted();
    static void aServiceThatPassesThroughCompletedStaysRecordedBackAtIdle();
    static void doesNotRecordCompletionWithoutActiveState();
    static void latchAdvancesWithoutAnyoneAskingForTheStatus();
    static void aDeiceReturningToIdleIsRecordedAsCompleted();
    static void readingTheStatusDoesNotAdvanceTheLatch();
    static void readsFuelHoseAndPassengerCounts();
    static void detectsSimbriefLoaded();
    static void resetClearsCompletionFlags();
    static void detectsWaitingForEngines();
    static void detectsPushbackStarted();
    static void detectsPushbackFinished();
    static void detectsRepositioning();
    static void cargoPercentReadsLVars();
    static void boardingCargoPercentIgnoresTheStalePercentUntilItMoves();
    static void cargoLoadingReadsLVars();
    static void loaderWaitingForDoorNamesTheFrontOneWhenSeveralWait();
    static void jetwayAndStairsAvailability();
    static void jetwayAndStairsUnavailableUntilLVarsReceived();
    static void jetwayAndStairsUnavailableWhileGsxStillEvaluatesTheParking();
    static void serviceVehicleActiveFollowsTheStairsVehicles();
    static void goodEngineStartAssumedEnabledUntilLVarReceived();
    static void aircraftOnGroundFollowsSimVar();
    static void boardedPassengersAccumulatesAcrossResets();
    static void deboardedPassengersAccumulatesAcrossResets();
    static void boardedPassengersIgnoresStaleTotalBeforeBoardingStarts();
    static void deboardedPassengersIgnoresStaleTotalBeforeDeboardingStarts();
    static void boardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick();
    static void deboardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick();
    static void takeOverFuelAndPayloadClearsAutomationLVars();
    static void reassertsTakeoverAfterCouatlReset();
    static void refuelCounterComesFromFuelCounterLvar();
    static void gpuStatusPrefersTheRemoteApiOverALyingLVar();
    static void gpuStatusFallsBackToTheLVarsWithoutTheRemoteApi();
    static void gpuStatusFollowsStateLVar();
    static void gpuStatusConnectedWhenConnectedFlagSetDespiteState();
    static void gpuStatusIgnoresAConnectedFlagInheritedFromTheLastTurnaround();
    static void gpuStatusDoesNotArmTheConnectedFlagBeforeItArrives();
    static void serviceInProgressFollowsRemoteStateRaw();
    static void serviceInProgressFalseWhenAbsentOrNoRemote();
    static void pushbackIsOfferedUnlessTheApronVerdictSaysOtherwise();
};

void GsxInterfaceTest::availabilityFollowsCouatlFlag()
{
    FakeVariableGateway gateway;

    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsAvailable());

    gateway.lvars[kCouatlStarted] = 1.0;

    QVERIFY(gsx.IsAvailable());
}

void GsxInterfaceTest::mapsServiceStateLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Completed);
    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Requested);

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Active);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Completed);
    QCOMPARE(gsx.GetStateStatus(GsxState::Pushback), GsxStateStatus::Unavailable);
    QCOMPARE(gsx.GetStateStatus(GsxState::Deice), GsxStateStatus::Requested);
}

void GsxInterfaceTest::recordsExplicitCompletedState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Completed);
    gsx.Observe();

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Completed);
    QVERIFY(gsx.WasStateCompleted(GsxState::Refueling));
}

void GsxInterfaceTest::aServiceTheCouatlDropsIsNotRecordedAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);
        ObserveFor(gsx, gateway, stateLVar, 0.0, 5.0, 12);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 5);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 30);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aServiceThatPassesThroughCompletedStaysRecordedBackAtIdle()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 63);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 7.0, 2);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 6.0, 11);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 20);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::doesNotRecordCompletionWithoutActiveState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);
    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::readsFuelHoseAndPassengerCounts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kFuelHoseConnected] = 1.0;
    gateway.lvars[kMaxPassengers] = 215.0;
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QVERIFY(gsx.IsFuelHoseConnected());
    QCOMPARE(gsx.GetPlannedPassengers(), 215);
    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 130.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 130);
}

void GsxInterfaceTest::detectsSimbriefLoaded()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsSimbriefLoaded());

    gateway.lvars[kSimbriefSuccess] = 1.0;

    QVERIFY(gsx.IsSimbriefLoaded());
}

void GsxInterfaceTest::resetClearsCompletionFlags()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Completed);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));

    gsx.Reset();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::detectsWaitingForEngines()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 8.0;

    QVERIFY(gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 0.0;

    QVERIFY(!gsx.IsWaitingForEngines());
}

void GsxInterfaceTest::detectsPushbackStarted()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 4.0;

    QVERIFY(!gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 6.0;

    QVERIFY(gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 8.0;

    QVERIFY(gsx.HasPushbackStarted());
}

void GsxInterfaceTest::detectsPushbackFinished()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kPushbackStatus] = 0.0;
    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Active);

    QVERIFY(!gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 6.0;

    QVERIFY(!gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 11.0;

    QVERIFY(gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 0.0;
    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Completed);

    QVERIFY(!gsx.IsPushbackFinished());
}

void GsxInterfaceTest::detectsRepositioning()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsRepositioning());

    gateway.lvars[kRepositioning] = 1.0;

    QVERIFY(gsx.IsRepositioning());

    gateway.lvars[kRepositioning] = 2.0;

    QVERIFY(!gsx.IsRepositioning());
}

void GsxInterfaceTest::cargoPercentReadsLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kBoardingCargoPercent] = 0.0;
    gateway.lvars[kDeboardingCargoPercent] = 17.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 42.5;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 42.5);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 17.0);
}

void GsxInterfaceTest::boardingCargoPercentIgnoresTheStalePercentUntilItMoves()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kBoardingCargoPercent] = 92.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 0.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 35.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 35.0);
}

void GsxInterfaceTest::cargoLoadingReadsLVars()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsLoadingCargo());
    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::None);

    gateway.lvars[kBoardingCargo] = 1.0;
    gateway.lvars[kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    QVERIFY(gsx.IsLoadingCargo());
    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::MainDeck);

    gateway.lvars[kBaggageLoaderMainState] = gsx::states::kLoaderRetracting;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::None);

    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Front);
}

void GsxInterfaceTest::loaderWaitingForDoorNamesTheFrontOneWhenSeveralWait()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kBaggageLoaderRearState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Rear);

    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Front);
}

void GsxInterfaceTest::jetwayAndStairsAvailability()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kJetway] = 5.0;
    gateway.lvars[kStairs] = 5.0;

    QVERIFY(gsx.IsJetwayInPlace());
    QVERIFY(gsx.AreStairsInPlace());
    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 2.0;
    gateway.lvars[kStairs] = 2.0;

    QVERIFY(!gsx.IsJetwayInPlace());
    QVERIFY(!gsx.AreStairsInPlace());
    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 3.0;

    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(!gsx.IsJetwayInPlace());
}

void GsxInterfaceTest::jetwayAndStairsUnavailableUntilLVarsReceived()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 1.0;
    gateway.lvars[kStairs] = 1.0;

    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(gsx.AreStairsAvailable());
}

void GsxInterfaceTest::jetwayAndStairsUnavailableWhileGsxStillEvaluatesTheParking()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kJetway] = 0.0;
    gateway.lvars[kStairs] = 0.0;

    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());
}

void GsxInterfaceTest::serviceVehicleActiveFollowsTheStairsVehicles()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = 1.0;

    QVERIFY(!gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kVehicleApproaching;

    QVERIFY(gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = 0.0;
    gateway.lvars[kPassengerStairsMiddleState] = gsx::states::kVehicleDispatched;

    QVERIFY(gsx.IsServiceVehicleActive());
}

void GsxInterfaceTest::goodEngineStartAssumedEnabledUntilLVarReceived()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 0.0;

    QVERIFY(!gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 1.0;

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());
}

void GsxInterfaceTest::aircraftOnGroundFollowsSimVar()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(gsx.IsAircraftOnGround());

    gateway.avars[kSimOnGround] = 1.0;

    QVERIFY(gsx.IsAircraftOnGround());

    gateway.avars[kSimOnGround] = 0.0;

    QVERIFY(!gsx.IsAircraftOnGround());
}

void GsxInterfaceTest::boardedPassengersAccumulatesAcrossResets()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 50.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 50);

    gateway.lvars[kNumPassengersBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 60);

    gateway.lvars[kNumPassengersBoardingTotal] = 30.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);

    gateway.lvars[kNumPassengersBoardingTotal] = 0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);
}

void GsxInterfaceTest::deboardedPassengersAccumulatesAcrossResets()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 80.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 80);

    gateway.lvars[kNumPassengersDeboardingTotal] = 5.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);

    gateway.lvars[kNumPassengersDeboardingTotal] = 0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);
}

void GsxInterfaceTest::boardedPassengersIgnoresStaleTotalBeforeBoardingStarts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kNumPassengersBoardingTotal] = 192.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Requested);

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 12.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 12);
}

void GsxInterfaceTest::deboardedPassengersIgnoresStaleTotalBeforeDeboardingStarts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Requested);
    gateway.lvars[kNumPassengersDeboardingTotal] = 192.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 7.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 7);
}

void GsxInterfaceTest::boardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 92.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);
    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 10);
}

void GsxInterfaceTest::deboardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 92.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 7.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 7);
}

void GsxInterfaceTest::takeOverFuelAndPayloadClearsAutomationLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);
    
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.TakeOverFuelAndPayload();

    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);
}

void GsxInterfaceTest::reassertsTakeoverAfterCouatlReset()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 1.0);

    gsx.TakeOverFuelAndPayload();
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);

    gsx.Reset();
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 1.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 1.0);
}

void GsxInterfaceTest::refuelCounterComesFromFuelCounterLvar()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QCOMPARE(gsx.GetRefuelCounterGallons(), 0.0);

    gateway.lvars[kFuelCounter] = 5000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 5000.0);

    gateway.lvars[kFuelCounterMax] = 8000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 8000.0);

    gateway.lvars[kFuelCounter] = 9000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 9000.0);
}

void GsxInterfaceTest::gpuStatusPrefersTheRemoteApiOverALyingLVar()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;

    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    remote.services.push_back(GsxRemoteService{"GPU", 1, true});

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    remote.services.front().stateRaw = 4;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    remote.services.front().stateRaw = 5;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusFallsBackToTheLVarsWithoutTheRemoteApi()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;

    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusFollowsStateLVar()
{
    FakeVariableGateway gateway;

    const GsxStateService gsx(&gateway);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kGpuConnected] = 0.0;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);
}

void GsxInterfaceTest::gpuStatusConnectedWhenConnectedFlagSetDespiteState()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Requested);
    gateway.lvars[kGpuConnected] = 0.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusIgnoresAConnectedFlagInheritedFromTheLastTurnaround()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Requested);
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 0.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusDoesNotArmTheConnectedFlagBeforeItArrives()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);
}

void GsxInterfaceTest::serviceInProgressFollowsRemoteStateRaw()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.services.push_back(GsxRemoteService{"Catering", 5, false});
    remote.services.push_back(GsxRemoteService{"Water", 4, false});
    remote.services.push_back(GsxRemoteService{"Lavatory", 1, true});
    remote.services.push_back(GsxRemoteService{"Cleaning", 6, false});

    const GsxStateService gsx(&gateway, &remote);

    QVERIFY(gsx.IsServiceInProgress(GroundService::Catering));
    QVERIFY(gsx.IsServiceInProgress(GroundService::Water));
    QVERIFY(!gsx.IsServiceInProgress(GroundService::Lavatory));
    QVERIFY(!gsx.IsServiceInProgress(GroundService::Cleaning));
}

void GsxInterfaceTest::serviceInProgressFalseWhenAbsentOrNoRemote()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.services.push_back(GsxRemoteService{"Catering", 5, false});

    const GsxStateService withRemote(&gateway, &remote);

    QVERIFY(!withRemote.IsServiceInProgress(GroundService::Lavatory));

    const GsxStateService noRemote(&gateway);

    QVERIFY(!noRemote.IsServiceInProgress(GroundService::Catering));
}

void GsxInterfaceTest::latchAdvancesWithoutAnyoneAskingForTheStatus()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Pushback));

    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Pushback));
}

void GsxInterfaceTest::aDeiceReturningToIdleIsRecordedAsCompleted()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Deice));

    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Deice));
}

void GsxInterfaceTest::readingTheStatusDoesNotAdvanceTheLatch()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Active);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::pushbackIsOfferedUnlessTheApronVerdictSaysOtherwise()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.apronVerdict = {"Gate Medium", "no pushback", "no bus"};

    const GsxStateService withVerdict(&gateway, &remote);

    QVERIFY(!withVerdict.OffersPushback());

    remote.apronVerdict = {"Gate Medium", "no bus"};

    QVERIFY(withVerdict.OffersPushback());

    const GsxStateService noRemote(&gateway);

    QVERIFY(noRemote.OffersPushback());
}

QTEST_APPLESS_MAIN(GsxInterfaceTest)

#include "tst_gsx_interface.moc"
