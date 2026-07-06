#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/infrastructure/gsx/GsxStateService.h"

namespace
{
    constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    constexpr auto kRefuelingState = "FSDT_GSX_REFUELING_STATE";
    constexpr auto kBoardingState = "FSDT_GSX_BOARDING_STATE";
    constexpr auto kFuelHoseConnected = "FSDT_GSX_FUELHOSE_CONNECTED";
    constexpr auto kMaxPassengers = "FSDT_GSX_MAX_NUMPASSENGERS";
    constexpr auto kBoardingTotal = "FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL";
    constexpr auto kSimbriefSuccess = "FSDT_GSX_SIMBRIEF_SUCCESS";
    constexpr auto kPushbackStatus = "FSDT_GSX_PUSHBACK_STATUS";
    constexpr auto kPushbackVehicleState = "FSDT_GSX_VEHICLE_PUSHBACK_STATE";
    constexpr auto kRepositioning = "FSDT_GSX_REPOSITIONING";
    constexpr auto kJetway = "FSDT_GSX_JETWAY";
    constexpr auto kStairs = "FSDT_GSX_STAIRS";
    constexpr auto kBoardingCargoPercent = "FSDT_GSX_BOARDING_CARGO_PERCENT";
    constexpr auto kDeboardingCargoPercent = "FSDT_GSX_DEBOARDING_CARGO_PERCENT";
    constexpr auto kAutomationFuel = "FSDT_GSX_AUTOMATION_FUEL";
    constexpr auto kAutomationPayload = "FSDT_GSX_AUTOMATION_PAYLOAD";
    constexpr auto kNumPaxBoardingTotal = "FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL";
    constexpr auto kNumPaxDeboardingTotal = "FSDT_GSX_NUMPASSENGERS_DEBOARDING_TOTAL";
    constexpr auto kSimOnGround = "SIM ON GROUND";
    constexpr auto kGoodEngineStart = "FSDT_GSX_SETTINGS_GOOD_ENGINE_START";
}

class GsxInterfaceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void availabilityFollowsCouatlFlag();
    static void mapsServiceStateLVars();
    static void recordsExplicitCompletedState();
    static void recordsCompletionWhenActiveReturnsToIdle();
    static void doesNotRecordCompletionWithoutActiveState();
    static void readsFuelHoseAndPassengerCounts();
    static void detectsSimbriefLoaded();
    static void resetClearsCompletionFlags();
    static void detectsWaitingForEngines();
    static void detectsPushbackStarted();
    static void detectsPushbackFinished();
    static void detectsRepositioning();
    static void cargoPercentReadsLVars();
    static void jetwayAndStairsAvailability();
    static void jetwayAndStairsUnavailableUntilLVarsReceived();
    static void goodEngineStartAssumedEnabledUntilLVarReceived();
    static void aircraftOnGroundFollowsSimVar();
    static void boardedPassengersAccumulatesAcrossResets();
    static void deboardedPassengersAccumulatesAcrossResets();
    static void takeOverFuelAndPayloadClearsAutomationLVars();
};

void GsxInterfaceTest::availabilityFollowsCouatlFlag()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

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

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Active);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Completed);
    QCOMPARE(gsx.GetStateStatus(GsxState::Pushback), GsxStateStatus::Unavailable);
}

void GsxInterfaceTest::recordsExplicitCompletedState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Completed);

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Completed);
    QVERIFY(gsx.WasStateCompleted(GsxState::Refueling));
}

void GsxInterfaceTest::recordsCompletionWhenActiveReturnsToIdle()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Active);
    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);

    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);
    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::doesNotRecordCompletionWithoutActiveState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);

    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);
    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::readsFuelHoseAndPassengerCounts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kFuelHoseConnected] = 1.0;
    gateway.lvars[kMaxPassengers] = 215.0;
    gateway.lvars[kBoardingTotal] = 130.0;

    QVERIFY(gsx.IsFuelHoseConnected());
    QCOMPARE(gsx.GetPlannedPassengers(), 215);
    QCOMPARE(gsx.GetBoardedPassengers(), 130);
}

void GsxInterfaceTest::detectsSimbriefLoaded()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsSimbriefLoaded());

    gateway.lvars[kSimbriefSuccess] = 1.0;

    QVERIFY(gsx.IsSimbriefLoaded());
}

void GsxInterfaceTest::resetClearsCompletionFlags()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    (void)gsx.GetStateStatus(GsxState::Boarding);
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    (void)gsx.GetStateStatus(GsxState::Boarding);

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));

    gsx.Reset();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::detectsWaitingForEngines()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 8.0;

    QVERIFY(gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 0.0;

    QVERIFY(!gsx.IsWaitingForEngines());
}

void GsxInterfaceTest::detectsPushbackStarted()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

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
    GsxStateService gsx(&gateway);

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
    GsxStateService gsx(&gateway);

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

    gateway.lvars[kBoardingCargoPercent] = 42.5;
    gateway.lvars[kDeboardingCargoPercent] = 17.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 42.5);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 17.0);
}

void GsxInterfaceTest::jetwayAndStairsAvailability()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

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
    GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 1.0;
    gateway.lvars[kStairs] = 1.0;

    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(gsx.AreStairsAvailable());
}

void GsxInterfaceTest::goodEngineStartAssumedEnabledUntilLVarReceived()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 0.0;

    QVERIFY(!gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 1.0;

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());
}

void GsxInterfaceTest::aircraftOnGroundFollowsSimVar()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

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

    gateway.lvars[kNumPaxBoardingTotal] = 50.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 50);

    gateway.lvars[kNumPaxBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 60);

    gateway.lvars[kNumPaxBoardingTotal] = 30.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);

    gateway.lvars[kNumPaxBoardingTotal] = 0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);
}

void GsxInterfaceTest::deboardedPassengersAccumulatesAcrossResets()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kNumPaxDeboardingTotal] = 80.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 80);

    gateway.lvars[kNumPaxDeboardingTotal] = 5.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);

    gateway.lvars[kNumPaxDeboardingTotal] = 0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);
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

QTEST_APPLESS_MAIN(GsxInterfaceTest)

#include "tst_gsx_interface.moc"
