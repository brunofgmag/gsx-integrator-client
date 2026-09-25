#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "AircraftTicks.h"
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/domain/support/Weight.h"
#include "../src/infrastructure/aircraft/fss/FssEJet.h"

namespace
{
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kKgUnit = "kg";
    constexpr auto kPoundsUnit = "pounds";
    constexpr auto kPercentOver100Unit = "percent over 100";
    constexpr auto kFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";
    constexpr auto kTankCapacity1 = "FUELSYSTEM TANK CAPACITY:1";
    constexpr auto kTankCapacity2 = "FUELSYSTEM TANK CAPACITY:2";
    constexpr auto kUnusableFuelTotal = "UNUSABLE FUEL TOTAL QUANTITY";
    constexpr std::array kTankLevels = {"FUELSYSTEM TANK LEVEL:1", "FUELSYSTEM TANK LEVEL:2"};

    constexpr double kFuelPoundsPerGallon = 6.7;
    constexpr double kTankGallons = 2149.0;
    constexpr double kReserveGallons = 30.0;
    constexpr double kUsableTankGallons = kTankGallons * 2.0 - kReserveGallons;
    constexpr double kFuelCapacityKg = weight::LbToKg(kFuelPoundsPerGallon * kUsableTankGallons);
    constexpr double kLevelTolerance = 1e-6;
    constexpr double kKgTolerance = 1e-6;

    constexpr auto kStation1 = "PAYLOAD STATION WEIGHT:1";
    constexpr auto kStation2 = "PAYLOAD STATION WEIGHT:2";
    constexpr auto kStation3 = "PAYLOAD STATION WEIGHT:3";
    constexpr auto kStation4 = "PAYLOAD STATION WEIGHT:4";
    constexpr auto kStation5 = "PAYLOAD STATION WEIGHT:5";
    constexpr auto kStation6 = "PAYLOAD STATION WEIGHT:6";

    constexpr auto kPlanWeightZoneA = "FSS_EXX_PLANE_SETUP_WEIGHT_ZONE_A";
    constexpr auto kPlanWeightZoneB = "FSS_EXX_PLANE_SETUP_WEIGHT_ZONE_B";
    constexpr auto kPlanWeightCargoFwd = "FSS_EXX_PLANE_SETUP_WEIGHT_CARGO_FWD";
    constexpr auto kPlanWeightCargoAft = "FSS_EXX_PLANE_SETUP_WEIGHT_CARGO_AFT";

    constexpr auto kNumPassengers = "FSDT_GSX_NUMPASSENGERS";
    constexpr auto kMaxNumPassengers = "FSDT_GSX_MAX_NUMPASSENGERS";
    constexpr double kMaxPassengersE190 = 114.0;
    constexpr double kMaxPassengersE195 = 124.0;

    void GiveTanks(FakeVariableGateway& gateway)
    {
        gateway.avars[kFuelWeightPerGallon] = kFuelPoundsPerGallon;
        gateway.avars[kTankCapacity1] = kTankGallons;
        gateway.avars[kTankCapacity2] = kTankGallons;
    }

    void GiveReserve(FakeVariableGateway& gateway)
    {
        gateway.avars[kUnusableFuelTotal] = kReserveGallons;
    }

    double ExpectedLevelForTargetKg(const double targetKg)
    {
        const double targetGallons = weight::KgToLb(targetKg) / kFuelPoundsPerGallon;

        return (targetGallons + kReserveGallons) / (kTankGallons * 2.0);
    }

    void PlanTheMeasuredFlight(AutomationStatus& status, const double payloadKg, const int passengers)
    {
        status.flightPlanStatus = FlightPlanStatus::Ready;
        status.plannedPayloadKg = payloadKg;
        status.plannedPassengers = passengers;
    }

    void GiveCargo(AutomationStatus& status, const double cargoKg)
    {
        status.plannedCargoKg = cargoKg;
    }

    void ParkWithTheMeasuredEmptyWeight(FakeVariableGateway& gateway, const double emptyWeightKg)
    {
        gateway.avars[kSimEmptyWeight] = emptyWeightKg;
    }

    constexpr auto kAcPowerAvailable = "FSS_EXX_ELEC_PWR_AC_AVAIL";
    constexpr auto kEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kParkBrakeLever = "FSS_EXX_PARKBRAKE_BV_LEVER";
    constexpr auto kBeaconSwitch = "FSS_EXX_OVHD_EXLT_RED_BCN_SWITCH";
    constexpr auto kCallRampLeft = "FSS_EXX_AUDIO_L_TEL_RAMP_BTN";
    constexpr auto kCallRampRight = "FSS_EXX_AUDIO_R_TEL_RAMP_BTN";
    constexpr auto kCallRampActive = "FSS_EXX_AUDIO_TEL_RAMP_ACTIVE";
    constexpr auto kCallRampLeftLight = "FSS_EXX_AUDIO_L_TEL_RAMP_LIGHT";
    constexpr auto kCallRampRightLight = "FSS_EXX_AUDIO_R_TEL_RAMP_LIGHT";
    constexpr auto kGpuState = "FSS_EXX_EXT_GPU_STATE";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kChocksF = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_F";
    constexpr auto kChocksL = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_L";
    constexpr auto kChocksR = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_R";
    constexpr std::array kChocks = {kChocksF, kChocksL, kChocksR};

    constexpr auto kAutoDeparture = "FSS_GNDSVC_AUTO_DEPARTURE";
    constexpr auto kAutoDeboarding = "FSS_GNDSVC_AUTO_DEBOARDING";
    constexpr auto kAutoGoodEngstart = "FSS_GNDSVC_AUTO_NOTIFY_GOOD_ENGSTART";
    constexpr std::array kAutomationLVars = {kAutoDeparture, kAutoDeboarding, kAutoGoodEngstart};
    constexpr auto kEnableGsxSupport = "FSS_ENABLE_GSX_SUPPORT";

    constexpr std::array kOwnGroundEquipment = {
        "FSS_EXX_GNDOBJ_CONE_ENG_L", "FSS_EXX_GNDOBJ_CONE_ENG_R", "FSS_EXX_GNDOBJ_CONE_ENTRY",
        "FSS_EXX_GNDOBJ_CONE_TAIL", "FSS_EXX_GNDOBJ_CONE_WING_L", "FSS_EXX_GNDOBJ_CONE_WING_R",
        "FSS_EXX_GNDOBJ_ENG_COVER_L", "FSS_EXX_GNDOBJ_ENG_COVER_R",
        "FSS_EXX_WHEEL_COVER_L", "FSS_EXX_WHEEL_COVER_R",
        "FSS_EXX_SAFETY_PIN_LDGGEAR_L", "FSS_EXX_SAFETY_PIN_LDGGEAR_R", "FSS_EXX_SAFETY_PIN_PITOT_BOTTOM",
        "FSS_EXX_SAFETY_PIN_PITOT_F_L", "FSS_EXX_SAFETY_PIN_PITOT_F_R",
        "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_L", "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_R",
        "FSS_EXX_STAIR_FWD_L_ACTIVE", "FSS_EXX_STAIR_AFT_L_ACTIVE"
    };

    constexpr double kGpuHidden = -1.0;
    constexpr double kGpuInactive = 0.0;
    constexpr double kGpuRequested = 1.0;
    constexpr double kGpuAvailable = 2.0;
    constexpr double kGpuStarting = 3.0;
    constexpr double kGpuStopping = 4.0;
    constexpr double kGpuFeeding = 5.0;

    constexpr int kFiftyTicks = 50;
    constexpr int kTwentyTicks = 20;
    constexpr int kTicksToWaitForTheEcho = 5;
    constexpr int kCallRampClearingTicks = 5;
    constexpr double kCallRampLit = 1.0;
    constexpr double kCallRampDark = 0.0;
    constexpr auto kAutomationRule = "fss-ejet-keep-vendor-automation-off";
    constexpr auto kGpuRule = "fss-ejet-gpu-follows-request";
    constexpr auto kDoorsRule = "fss-ejet-doors-follow-gsx";

    constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    constexpr auto kJetway = "FSDT_GSX_JETWAY";
    constexpr auto kFrontStairsState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSFRONT_STATE";
    constexpr auto kRearStairsState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSREAR_STATE";
    constexpr auto kFrontCateringState = "FSDT_GSX_VEHICLE_CATERINGVEHICLEFRONT_STATE";
    constexpr auto kRearCateringState = "FSDT_GSX_VEHICLE_CATERINGVEHICLEREAR_STATE";
    constexpr auto kFrontLoaderState = "FSDT_GSX_VEHICLE_BAGGAGELOADERFRONT_STATE";
    constexpr auto kRearLoaderState = "FSDT_GSX_VEHICLE_BAGGAGELOADERREAR_STATE";
    constexpr auto kMainLoaderState = "FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE";
    constexpr double kJetwayDocked = 5.0;
    constexpr double kStairsDocked = 3.0;
    constexpr double kVehicleApproaching = 5.0;
    constexpr double kLoaderWaitingForDoor = 6.0;
    constexpr double kVehicleGone = 0.0;

    constexpr auto kL1Req = "FSS_GNDSVC_MAINDOOR_FWD_L_REQ";
    constexpr auto kL1Ack = "FSS_FLTCREW_MAINDOOR_FWD_L_REQ";
    constexpr auto kL1Open = "FSS_EXX_DOOR_FWD_L_OPEN";
    constexpr auto kL1Moving = "FSS_EXX_DOOR_FWD_L_MOVING";
    constexpr auto kL2Req = "FSS_GNDSVC_MAINDOOR_AFT_L_REQ";
    constexpr auto kL2Open = "FSS_EXX_DOOR_AFT_L_OPEN";
    constexpr auto kR1Req = "FSS_GNDSVC_MAINDOOR_FWD_R_REQ";
    constexpr auto kR1Open = "FSS_EXX_DOOR_FWD_R_OPEN";
    constexpr auto kR2Req = "FSS_GNDSVC_MAINDOOR_AFT_R_REQ";
    constexpr auto kR2Open = "FSS_EXX_DOOR_AFT_R_OPEN";
    constexpr auto kCargoFwdReq = "FSS_GNDSVC_CARGO_FWD_REQ";
    constexpr auto kCargoFwdOpen = "FSS_EXX_DOOR_CARGO_FWD_OPEN";
    constexpr auto kCargoAftReq = "FSS_GNDSVC_CARGO_AFT_REQ";
    constexpr auto kCargoAftOpen = "FSS_EXX_DOOR_CARGO_AFT_OPEN";
    constexpr auto kMainDeckReq = "FSS_GNDSVC_CARGO_MAIN_REQ";
    constexpr auto kMainDeckOpen = "FSS_EXX_DOOR_CARGO_MAIN_OPEN";

    constexpr int kFourteenTicks = 14;

    class LogCapture
    {
    public:
        LogCapture() : previous_(qInstallMessageHandler(Collect)) { Lines().clear(); }
        ~LogCapture() { qInstallMessageHandler(previous_); }

        [[nodiscard]] static bool Contains(const char* fragment)
        {
            return std::ranges::any_of(Lines(), [fragment](const QString& line)
                                       { return line.contains(QLatin1String(fragment)); });
        }

    private:
        static QStringList& Lines()
        {
            static QStringList lines;

            return lines;
        }

        static void Collect(QtMsgType, const QMessageLogContext&, const QString& message)
        {
            Lines().append(message);
        }

        QtMessageHandler previous_;
    };

    void AllEnginesStopped(FakeVariableGateway& gateway)
    {
        gateway.avars[kEng1Combustion] = 0.0;
        gateway.avars[kEng2Combustion] = 0.0;
    }

    void TickTimes(Aircraft& aircraft, FakeVariableGateway& gateway, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            TickAircraft(aircraft, gateway);
        }
    }
}

class FssEJetTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsTheCargoVariantItWasBuiltWith();
    static void logsTheProfileNameWithTheFreighterSuffix();
    static void readsThePlanFromTheClientOfp();
    static void doesNotConsiderThePlanLoadedWithoutThePayloadLineOrTheEmptyWeight();
    static void targetsTheEmptyWeightPlusThePlannedPayload();
    static void writesKgAsTheNativeUnit();
    static void loadsAndRefuelsThroughTheClient();
    static void energizedFollowsAcAvailable();
    static void engineRunningFollowsCombustion();
    static void defaultsHoldTheStateUntilDataArrives();
    static void parkingBrakeFollowsTheLeverOnly();
    static void heldInPlaceAcceptsTheLeverOrTheChocks();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
    static void chocksWriteTheSameValueToTheThree();
    static void chocksReadIsAnyOfTheThree();
    static void clearsTheNineteenOwnGroundEquipmentLVarsOnce();
    static void smartSwitchFiresOnceAndClearsTheCallByWritingActive();
    static void theRightCallRampAlsoFiresOnceAndClearsTheCallByWritingActive();
    static void consumingTheSmartSwitchNeverWritesToTheLightsNorToEitherButton();
    static void aCallRampLeftOffNeverFiresNorWrites();
    static void aToggleLandingAfterTheClearIsClearedAgainOnTheNextTick();
    static void aToggleLandingAfterADarkTickIsStillClearedButACallAfterTheWindowIsNot();
    static void theClearingStopsAfterItsWindowWhenTheCallStaysLit();
    static void aLitCallWithoutAConsumedTouchIsNeverWritten();
    static void anUnreceivedCallAfterATouchIsNeverWritten();
    static void aNewTouchReopensTheClearing();
    static void groundPowerStatusReadsTheThreeSettledValues();
    static void pulsesOnceWhenDisconnectedAndRequestedOn();
    static void pulsesNothingWhenTheStatusAlreadyMatchesTheRequest();
    static void neverPulsesWhileTheStateIsInTransit();
    static void stopsRequestingOnceTheStateSettlesOnTheTarget();
    static void gpuRuleNeverHoldsThePhase();
    static void vendorAutomationTurnsOffTheThreeKeysOnce();
    static void vendorAutomationRewritesAKeyThatDriftsBackOn();
    static void vendorAutomationNeverTouchesEnableGsxSupport();
    static void vendorAutomationRuleNeverHoldsThePhase();
    static void doorsFollowTheGsxVehicleThatServesEach();
    static void theFreighterNeverWritesToL2NorR2();
    static void aPassengerDoorHeardWithinTheWaitIsNeverReaffirmed();
    static void aPassengerDoorWithNoAckIsReaffirmedTwiceAtMost();
    static void aCargoBayWithoutAckReaffirmsAfterTwoTicks();
    static void theMainDeckOpensOnlyWithTheAircraftEnergized();
    static void theMainDeckRuleHoldsWhileTheLoaderWaitsUnpowered();
    static void closeAllDoorsWritesZeroEverywhereItManages();
    static void doorStatusCombinesOpenMovingAndClosedWithAPrazo();
    static void theFreighterDoorStatusIgnoresL2AndR2AndIncludesTheMainDeck();
    static void doorsRuleNeverWritesToAnInteractivePointOrTheExitToggle();
    static void observingEvaluatingAndReadingWriteNoVariable();
    static void fuelCapacitySumsBothTanksMinusTheReserveInKg();
    static void fuelCapacityWaitsForTheWeightPerGallonTheTwoCapacitiesAndTheReserve();
    static void refuelWritesTheSameLevelFractionInBothTanks();
    static void refuelKeepsTheTankLevelBetweenEmptyAndFull();
    static void refuelWritesNothingUntilTheWeightPerGallonTheTwoCapacitiesAndTheReserveArrive();
    static void refuelWritesTheSameTargetOnlyOnce();
    static void loadsPassengerZonesThenHoldsByTheMeasuredSplit();
    static void writesNoStationsWithoutACargoLineInThePlan();
    static void loadsTheFreighterStationsByTheFixedRatios();
    static void loadingNeverTouchesTheCrewStations();
    static void loadingWritesNothingUntilTheEmptyWeightArrives();
    static void mirrorsThePlannedCargoIntoThePlaneSetupWeightsOnce();
    static void mirrorsNoPlaneSetupWeightsWithoutACargoLineInThePlan();
    static void reportsPlannedPassengersToGsxOnce();
    static void reportsTheMaxPassengersForTheType();
};

void FssEJetTest::reportsTheCargoVariantItWasBuiltWith()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    const FssEJet passenger(&gateway, &status, FssEJet::kNameE190, false);
    const FssEJet freighter(&gateway, &status, FssEJet::kNameE190, true);

    QVERIFY(!passenger.IsCargoVariant());
    QVERIFY(freighter.IsCargoVariant());
}

void FssEJetTest::logsTheProfileNameWithTheFreighterSuffix()
{
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        const LogCapture log;
        const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

        QVERIFY(LogCapture::Contains("Profile loaded: FSS Embraer E190"));
        QVERIFY(!LogCapture::Contains("Profile loaded: FSS Embraer E190 Freighter"));
    }
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        const LogCapture log;
        const FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, true);

        QVERIFY(LogCapture::Contains("Profile loaded: FSS Embraer E195 Freighter"));
    }
}

void FssEJetTest::readsThePlanFromTheClientOfp()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.RequiresEfbFlightPlan());
    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.plannedFuelKg = 6531.0;
    PlanTheMeasuredFlight(status, 6000.0, 88);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    QVERIFY(aircraft.IsFlightPlanLoaded());
    QCOMPARE(aircraft.GetPlannedFuelKg(), 6531.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 40000.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 88);
}

void FssEJetTest::doesNotConsiderThePlanLoadedWithoutThePayloadLineOrTheEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 6000.0, 50);

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    QVERIFY(aircraft.IsFlightPlanLoaded());

    status.plannedPayloadKg.reset();

    QVERIFY(!aircraft.IsFlightPlanLoaded());
}

void FssEJetTest::targetsTheEmptyWeightPlusThePlannedPayload()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 6000.0, 50);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    QVERIFY(aircraft.IsFlightPlanLoaded());
    QCOMPARE(aircraft.GetPlannedZfwKg(), 40000.0);

    status.plannedZfwKg = 39000.0;

    QCOMPARE(aircraft.GetPlannedZfwKg(), 40000.0);
}

void FssEJetTest::writesKgAsTheNativeUnit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetNativeWeightUnit() == WeightUnit::Kg);
}

void FssEJetTest::loadsAndRefuelsThroughTheClient()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Client);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Client);
    QVERIFY(aircraft.SupportsStairsOrJetways());
    QVERIFY(!aircraft.CompletesPushbackViaInterruptMenu());
    QVERIFY(!aircraft.TakesExternalPowerAtTheEngineerPanel());
}

void FssEJetTest::energizedFollowsAcAvailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kAcPowerAvailable] = 0.0;

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(aircraft.IsPowered());
}

void FssEJetTest::engineRunningFollowsCombustion()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void FssEJetTest::defaultsHoldTheStateUntilDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsPowered());
    QVERIFY(aircraft.IsEngineRunning());
    QVERIFY(!aircraft.IsParkingBrakeSet());
    QVERIFY(!aircraft.IsHeldInPlace());
    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);
}

void FssEJetTest::parkingBrakeFollowsTheLeverOnly()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.avars[kParkBrakeLever] = 1.0;

    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsParkingBrakeSet());
}

void FssEJetTest::heldInPlaceAcceptsTheLeverOrTheChocks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());

    gateway.lvars[kParkBrakeLever] = 0.0;
    gateway.lvars[kChocksR] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
}

void FssEJetTest::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(aircraft.IsReadyToPush());

    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());
}

void FssEJetTest::readyToDeboardFollowsSafetyState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsReadyToDeboard());

    gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());
}

void FssEJetTest::chocksWriteTheSameValueToTheThree()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.SetChocks(true));

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.Written(lVar), 1.0);
    }

    QVERIFY(aircraft.SetChocks(false));

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.Written(lVar), 0.0);
    }
}

void FssEJetTest::chocksReadIsAnyOfTheThree()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kChocksF] = 0.0;
    gateway.lvars[kChocksL] = 0.0;
    gateway.lvars[kChocksR] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
}

void FssEJetTest::clearsTheNineteenOwnGroundEquipmentLVarsOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    aircraft.ClearOwnGroundEquipment();

    for (const char* lVar : kOwnGroundEquipment)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
        QCOMPARE(gateway.Written(lVar), 0.0);
    }

    TickTimes(aircraft, gateway, kTwentyTicks);

    for (const char* lVar : kOwnGroundEquipment)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
    }

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.WriteCount(lVar), 0);
    }
}

void FssEJetTest::smartSwitchFiresOnceAndClearsTheCallByWritingActive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);
    QCOMPARE(gateway.Written(kCallRampActive), 0.0);
    QCOMPARE(gateway.WriteCount(kCallRampLeft), 0);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);
    QCOMPARE(gateway.WriteCount(kCallRampLeft), 0);
}

void FssEJetTest::theRightCallRampAlsoFiresOnceAndClearsTheCallByWritingActive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampRight] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);
    QCOMPARE(gateway.Written(kCallRampActive), 0.0);
    QCOMPARE(gateway.WriteCount(kCallRampRight), 0);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvarSpans[kCallRampRight] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);
    QCOMPARE(gateway.WriteCount(kCallRampRight), 0);
}

void FssEJetTest::consumingTheSmartSwitchNeverWritesToTheLightsNorToEitherButton()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    QCOMPARE(gateway.WriteCount(kCallRampLeftLight), 0);
    QCOMPARE(gateway.WriteCount(kCallRampRightLight), 0);
    QCOMPARE(gateway.WriteCount(kCallRampLeft), 0);
    QCOMPARE(gateway.WriteCount(kCallRampRight), 0);
}

void FssEJetTest::aCallRampLeftOffNeverFiresNorWrites()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
}

void FssEJetTest::aToggleLandingAfterTheClearIsClearedAgainOnTheNextTick()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    gateway.lvars[kCallRampActive] = kCallRampDark;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);

    gateway.lvars[kCallRampActive] = kCallRampLit;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 2);
    QCOMPARE(gateway.Written(kCallRampActive), kCallRampDark);
    QCOMPARE(gateway.WriteCount(kCallRampLeftLight), 0);
    QCOMPARE(gateway.WriteCount(kCallRampRightLight), 0);
    QCOMPARE(gateway.WriteCount(kCallRampLeft), 0);
    QCOMPARE(gateway.WriteCount(kCallRampRight), 0);
}

void FssEJetTest::aToggleLandingAfterADarkTickIsStillClearedButACallAfterTheWindowIsNot()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    gateway.lvars[kCallRampActive] = kCallRampDark;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvars[kCallRampActive] = kCallRampDark;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);

    gateway.lvars[kCallRampActive] = kCallRampLit;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 2);
    QCOMPARE(gateway.Written(kCallRampActive), kCallRampDark);

    for (int tick = 2; tick < kCallRampClearingTicks; ++tick)
    {
        gateway.lvars[kCallRampActive] = kCallRampDark;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 2);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvars[kCallRampActive] = kCallRampLit;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 2);
}

void FssEJetTest::theClearingStopsAfterItsWindowWhenTheCallStaysLit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvars[kCallRampActive] = kCallRampLit;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 1 + kCallRampClearingTicks);
}

void FssEJetTest::aLitCallWithoutAConsumedTouchIsNeverWritten()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvars[kCallRampActive] = kCallRampLit;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 0);
    QCOMPARE(gateway.setLVarCalls, 0);
}

void FssEJetTest::anUnreceivedCallAfterATouchIsNeverWritten()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvars.erase(kCallRampActive);
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 1);
}

void FssEJetTest::aNewTouchReopensTheClearing()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvars[kCallRampActive] = kCallRampLit;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampActive), 1 + kCallRampClearingTicks);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 2 + kCallRampClearingTicks);

    gateway.lvars[kCallRampActive] = kCallRampLit;
    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampActive), 3 + kCallRampClearingTicks);
}

void FssEJetTest::groundPowerStatusReadsTheThreeSettledValues()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = kGpuInactive;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = kGpuHidden;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = kGpuFeeding;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);

    for (const double transit : {kGpuRequested, kGpuAvailable, kGpuStarting, kGpuStopping})
    {
        gateway.lvars[kGpuState] = transit;

        QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);
    }
}

void FssEJetTest::pulsesOnceWhenDisconnectedAndRequestedOn()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuInactive;
    aircraft.SetGroundPower(true);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);
}

void FssEJetTest::pulsesNothingWhenTheStatusAlreadyMatchesTheRequest()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuFeeding;
    aircraft.SetGroundPower(true);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 0);
}

void FssEJetTest::neverPulsesWhileTheStateIsInTransit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuAvailable;
    aircraft.SetGroundPower(true);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 0);
}

void FssEJetTest::stopsRequestingOnceTheStateSettlesOnTheTarget()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuInactive;
    aircraft.SetGroundPower(true);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);

    gateway.lvars[kGpuState] = kGpuFeeding;

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);
}

void FssEJetTest::gpuRuleNeverHoldsThePhase()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AircraftRule* const rule = FindRule(aircraft, kGpuRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void FssEJetTest::vendorAutomationTurnsOffTheThreeKeysOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickAircraft(aircraft, gateway);

    for (const char* lVar : kAutomationLVars)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
        QCOMPARE(gateway.Written(lVar), 0.0);
    }

    TickTimes(aircraft, gateway, kFiftyTicks - 1);

    for (const char* lVar : kAutomationLVars)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
    }
}

void FssEJetTest::vendorAutomationRewritesAKeyThatDriftsBackOn()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickTimes(aircraft, gateway, kTicksToWaitForTheEcho + 2);

    QCOMPARE(gateway.WriteCount(kAutoDeparture), 1);

    gateway.lvars[kAutoDeparture] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAutoDeparture), 2);
    QCOMPARE(gateway.Written(kAutoDeparture), 0.0);
}

void FssEJetTest::vendorAutomationNeverTouchesEnableGsxSupport()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kEnableGsxSupport), 0);
}

void FssEJetTest::vendorAutomationRuleNeverHoldsThePhase()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AircraftRule* const rule = FindRule(aircraft, kAutomationRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void FssEJetTest::doorsFollowTheGsxVehicleThatServesEach()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kL1Req), 0);
    QCOMPARE(gateway.WriteCount(kL2Req), 0);
    QCOMPARE(gateway.WriteCount(kR1Req), 0);
    QCOMPARE(gateway.WriteCount(kR2Req), 0);
    QCOMPARE(gateway.WriteCount(kCargoFwdReq), 0);
    QCOMPARE(gateway.WriteCount(kCargoAftReq), 0);

    gateway.lvars[kFrontStairsState] = kStairsDocked;
    gateway.lvars[kRearStairsState] = kStairsDocked;
    gateway.lvars[kFrontCateringState] = kVehicleApproaching;
    gateway.lvars[kRearCateringState] = kVehicleApproaching;
    gateway.lvars[kFrontLoaderState] = kLoaderWaitingForDoor;
    gateway.lvars[kRearLoaderState] = kLoaderWaitingForDoor;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kL1Req), 1.0);
    QCOMPARE(gateway.Written(kL2Req), 1.0);
    QCOMPARE(gateway.Written(kR1Req), 1.0);
    QCOMPARE(gateway.Written(kR2Req), 1.0);
    QCOMPARE(gateway.Written(kCargoFwdReq), 1.0);
    QCOMPARE(gateway.Written(kCargoAftReq), 1.0);

    gateway.lvars[kFrontStairsState] = kVehicleGone;
    gateway.lvars[kRearStairsState] = kVehicleGone;
    gateway.lvars[kFrontCateringState] = kVehicleGone;
    gateway.lvars[kRearCateringState] = kVehicleGone;
    gateway.lvars[kFrontLoaderState] = kVehicleGone;
    gateway.lvars[kRearLoaderState] = kVehicleGone;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kL1Req), 0.0);
    QCOMPARE(gateway.Written(kL2Req), 0.0);
    QCOMPARE(gateway.Written(kR1Req), 0.0);
    QCOMPARE(gateway.Written(kR2Req), 0.0);
    QCOMPARE(gateway.Written(kCargoFwdReq), 0.0);
    QCOMPARE(gateway.Written(kCargoAftReq), 0.0);

    gateway.lvars[kFrontStairsState] = kVehicleGone;
    gateway.lvars[kJetway] = kJetwayDocked;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kL1Req), 1.0);
}

void FssEJetTest::theFreighterNeverWritesToL2NorR2()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, true);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kRearStairsState] = kStairsDocked;
    gateway.lvars[kRearCateringState] = kVehicleApproaching;

    TickTimes(aircraft, gateway, kFiftyTicks);

    aircraft.CloseAllDoors();
    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kL2Req), 0);
    QCOMPARE(gateway.WriteCount(kR2Req), 0);
}

void FssEJetTest::aPassengerDoorHeardWithinTheWaitIsNeverReaffirmed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFrontStairsState] = kStairsDocked;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kL1Req), 1);

    gateway.lvars[kL1Ack] = 11.0;
    TickTimes(aircraft, gateway, kFourteenTicks + 5);

    QCOMPARE(gateway.WriteCount(kL1Req), 1);
}

void FssEJetTest::aPassengerDoorWithNoAckIsReaffirmedTwiceAtMost()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFrontStairsState] = kStairsDocked;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kL1Req), 1);

    TickTimes(aircraft, gateway, kFourteenTicks - 1);
    QCOMPARE(gateway.WriteCount(kL1Req), 1);

    TickAircraft(aircraft, gateway);
    QCOMPARE(gateway.WriteCount(kL1Req), 2);

    TickTimes(aircraft, gateway, kFourteenTicks - 1);
    QCOMPARE(gateway.WriteCount(kL1Req), 2);

    TickAircraft(aircraft, gateway);
    QCOMPARE(gateway.WriteCount(kL1Req), 3);

    TickTimes(aircraft, gateway, kFourteenTicks * 3);
    QCOMPARE(gateway.WriteCount(kL1Req), 3);
}

void FssEJetTest::aCargoBayWithoutAckReaffirmsAfterTwoTicks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFrontLoaderState] = kLoaderWaitingForDoor;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kCargoFwdReq), 1);

    TickAircraft(aircraft, gateway);
    QCOMPARE(gateway.WriteCount(kCargoFwdReq), 1);

    TickAircraft(aircraft, gateway);
    QCOMPARE(gateway.WriteCount(kCargoFwdReq), 2);

    gateway.lvars[kCargoFwdOpen] = 1.0;
    TickTimes(aircraft, gateway, kTwentyTicks);
    QCOMPARE(gateway.WriteCount(kCargoFwdReq), 2);
}

void FssEJetTest::theMainDeckOpensOnlyWithTheAircraftEnergized()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, true);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kMainLoaderState] = kLoaderWaitingForDoor;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kMainDeckReq), 0);

    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kMainDeckReq), 0);

    gateway.lvars[kAcPowerAvailable] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kMainDeckReq), 1);
    QCOMPARE(gateway.Written(kMainDeckReq), 1.0);
}

void FssEJetTest::theMainDeckRuleHoldsWhileTheLoaderWaitsUnpowered()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, true);

    AircraftRule* const rule = FindRule(aircraft, kDoorsRule);

    QVERIFY(rule != nullptr);
    QVERIFY(!rule->Evaluate(kLoading).holds);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kMainLoaderState] = kLoaderWaitingForDoor;

    QVERIFY(rule->Evaluate(kLoading).holds);
    QVERIFY(!rule->Evaluate(kPassengerAccess).holds);
    QVERIFY(!rule->Evaluate(RuleContext{}).holds);

    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(!rule->Evaluate(kLoading).holds);
}

void FssEJetTest::closeAllDoorsWritesZeroEverywhereItManages()
{
    for (const bool cargo : {false, true})
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, cargo);

        gateway.lvars[kCouatlStarted] = 1.0;
        gateway.lvars[kFrontStairsState] = kStairsDocked;
        gateway.lvars[kFrontCateringState] = kVehicleApproaching;
        gateway.lvars[kFrontLoaderState] = kLoaderWaitingForDoor;
        gateway.lvars[kRearLoaderState] = kLoaderWaitingForDoor;

        if (!cargo)
        {
            gateway.lvars[kRearStairsState] = kStairsDocked;
            gateway.lvars[kRearCateringState] = kVehicleApproaching;
        }
        else
        {
            gateway.lvars[kMainLoaderState] = kLoaderWaitingForDoor;
            gateway.lvars[kAcPowerAvailable] = 1.0;
        }

        TickTimes(aircraft, gateway, 3);

        QCOMPARE(gateway.Written(kL1Req), 1.0);
        QCOMPARE(gateway.Written(kCargoFwdReq), 1.0);
        QCOMPARE(gateway.Written(kCargoAftReq), 1.0);

        if (!cargo)
        {
            QCOMPARE(gateway.Written(kL2Req), 1.0);
            QCOMPARE(gateway.Written(kR2Req), 1.0);
        }
        else
        {
            QCOMPARE(gateway.Written(kMainDeckReq), 1.0);
        }

        aircraft.CloseAllDoors();
        TickAircraft(aircraft, gateway);

        QCOMPARE(gateway.Written(kL1Req), 0.0);
        QCOMPARE(gateway.Written(kCargoFwdReq), 0.0);
        QCOMPARE(gateway.Written(kCargoAftReq), 0.0);

        if (!cargo)
        {
            QCOMPARE(gateway.Written(kL2Req), 0.0);
            QCOMPARE(gateway.Written(kR2Req), 0.0);
        }
        else
        {
            QCOMPARE(gateway.Written(kMainDeckReq), 0.0);
        }
    }
}

void FssEJetTest::doorStatusCombinesOpenMovingAndClosedWithAPrazo()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);

    for (const char* lVar : {kL1Open, kL2Open, kR1Open, kR2Open, kCargoFwdOpen, kCargoAftOpen})
    {
        gateway.lvars[lVar] = 0.0;
    }

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.lvars[kL1Open] = 1.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);

    gateway.lvars[kL1Open] = 0.0;
    gateway.lvars[kL1Moving] = 1.0;

    for (int tick = 0; tick < kFourteenTicks - 1; ++tick)
    {
        gateway.MarkTick();
        aircraft.Observe();

        QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
    }

    gateway.MarkTick();
    aircraft.Observe();

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void FssEJetTest::theFreighterDoorStatusIgnoresL2AndR2AndIncludesTheMainDeck()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, true);

    for (const char* lVar : {kL1Open, kR1Open, kCargoFwdOpen, kCargoAftOpen, kMainDeckOpen})
    {
        gateway.lvars[lVar] = 0.0;
    }

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.lvars[kL2Open] = 1.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.lvars[kMainDeckOpen] = 1.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void FssEJetTest::doorsRuleNeverWritesToAnInteractivePointOrTheExitToggle()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFrontStairsState] = kStairsDocked;
    gateway.lvars[kRearStairsState] = kStairsDocked;
    gateway.lvars[kFrontCateringState] = kVehicleApproaching;
    gateway.lvars[kRearCateringState] = kVehicleApproaching;
    gateway.lvars[kFrontLoaderState] = kLoaderWaitingForDoor;
    gateway.lvars[kRearLoaderState] = kLoaderWaitingForDoor;

    TickTimes(aircraft, gateway, kFiftyTicks);

    aircraft.CloseAllDoors();
    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.setAVarCalls, 0);
}

void FssEJetTest::observingEvaluatingAndReadingWriteNoVariable()
{
    for (const bool cargo : {false, true})
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, cargo);

        QVERIFY(!aircraft.Rules().empty());

        AllEnginesStopped(gateway);
        gateway.lvars[kAcPowerAvailable] = 1.0;
        gateway.lvars[kParkBrakeLever] = 1.0;
        gateway.lvars[kChocksF] = 1.0;
        gateway.lvars[kGpuState] = kGpuFeeding;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};
        status.flightPlanStatus = FlightPlanStatus::Ready;

        for (int tick = 0; tick < kFiftyTicks; ++tick)
        {
            gateway.MarkTick();
            aircraft.Observe();
            for (AircraftRule* const rule : aircraft.Rules())
            {
                static_cast<void>(rule->Evaluate(kLoading));
                static_cast<void>(rule->Evaluate(kPassengerAccess));
            }
        }

        static_cast<void>(aircraft.IsPowered());
        static_cast<void>(aircraft.IsEngineRunning());
        static_cast<void>(aircraft.IsParkingBrakeSet());
        static_cast<void>(aircraft.IsHeldInPlace());
        static_cast<void>(aircraft.IsReadyToPush());
        static_cast<void>(aircraft.IsReadyToDeboard());
        static_cast<void>(aircraft.IsFlightPlanLoaded());
        static_cast<void>(aircraft.GetPlannedZfwKg());
        static_cast<void>(aircraft.GetPlannedFuelKg());
        static_cast<void>(aircraft.GetPlannedPassengers());
        static_cast<void>(aircraft.GetEmptyZfwKg());
        static_cast<void>(aircraft.GetCurrentFuelKg());
        static_cast<void>(aircraft.GetCurrentZfwKg());
        static_cast<void>(aircraft.GetGroundPowerStatus());
        static_cast<void>(aircraft.GetDoorStatus());
        static_cast<void>(aircraft.ConsumeSmartSwitch());

        QCOMPARE(gateway.setLVarCalls, 0);
        QCOMPARE(gateway.setAVarCalls, 0);
    }
}

void FssEJetTest::fuelCapacitySumsBothTanksMinusTheReserveInKg()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    GiveTanks(gateway);
    GiveReserve(gateway);

    QVERIFY(std::abs(aircraft.GetFuelCapacityKg() - kFuelCapacityKg) < kKgTolerance);
}

void FssEJetTest::fuelCapacityWaitsForTheWeightPerGallonTheTwoCapacitiesAndTheReserve()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);

    gateway.avars[kTankCapacity1] = kTankGallons;
    gateway.avars[kTankCapacity2] = kTankGallons;

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);

    gateway.avars[kFuelWeightPerGallon] = kFuelPoundsPerGallon;

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);

    GiveReserve(gateway);

    QVERIFY(std::abs(aircraft.GetFuelCapacityKg() - kFuelCapacityKg) < kKgTolerance);
}

void FssEJetTest::refuelWritesTheSameLevelFractionInBothTanks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    GiveTanks(gateway);
    GiveReserve(gateway);

    const double targetKg = kFuelCapacityKg / 2.0;
    aircraft.SetCurrentFuelKg(targetKg);

    const double expectedLevel = ExpectedLevelForTargetKg(targetKg);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.AVarWriteCount(level), 1);
        QVERIFY(std::abs(gateway.WrittenAVar(level) - expectedLevel) < kLevelTolerance);
        QCOMPARE(gateway.AVarWriteUnit(level), std::string(kPercentOver100Unit));
    }
}

void FssEJetTest::refuelKeepsTheTankLevelBetweenEmptyAndFull()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    GiveTanks(gateway);
    GiveReserve(gateway);

    aircraft.SetCurrentFuelKg(kFuelCapacityKg * 2.0);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.WrittenAVar(level), 1.0);
    }

    aircraft.SetCurrentFuelKg(-5000.0);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.WrittenAVar(level), 0.0);
    }
}

void FssEJetTest::refuelWritesNothingUntilTheWeightPerGallonTheTwoCapacitiesAndTheReserveArrive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);

    QCOMPARE(gateway.setAVarCalls, 0);

    gateway.avars[kTankCapacity1] = kTankGallons;
    gateway.avars[kTankCapacity2] = kTankGallons;

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);

    QCOMPARE(gateway.setAVarCalls, 0);

    gateway.avars[kFuelWeightPerGallon] = kFuelPoundsPerGallon;

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);

    QCOMPARE(gateway.setAVarCalls, 0);

    GiveReserve(gateway);

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.AVarWriteCount(level), 1);
    }
}

void FssEJetTest::refuelWritesTheSameTargetOnlyOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    GiveTanks(gateway);
    GiveReserve(gateway);

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);
    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 2.0);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.AVarWriteCount(level), 1);
    }

    aircraft.SetCurrentFuelKg(kFuelCapacityKg / 4.0);

    for (const char* level : kTankLevels)
    {
        QCOMPARE(gateway.AVarWriteCount(level), 2);
    }
}

void FssEJetTest::loadsPassengerZonesThenHoldsByTheMeasuredSplit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 5940.0, 66);
    GiveCargo(status, 660.0);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.SetCurrentZfwKg(aircraft.GetPlannedZfwKg());

    QVERIFY(std::abs(gateway.WrittenAVar(kStation3) - 1140.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation5) - 4140.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation4) - 475.2) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation6) - 184.8) < kKgTolerance);

    for (const char* station : {kStation3, kStation4, kStation5, kStation6})
    {
        QCOMPARE(gateway.AVarWriteUnit(station), std::string(kKgUnit));
    }
}

void FssEJetTest::writesNoStationsWithoutACargoLineInThePlan()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 5940.0, 66);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.SetCurrentZfwKg(aircraft.GetPlannedZfwKg());

    QCOMPARE(gateway.setAVarCalls, 0);
}

void FssEJetTest::loadsTheFreighterStationsByTheFixedRatios()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, true);

    PlanTheMeasuredFlight(status, 10000.0, 0);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.SetCurrentZfwKg(aircraft.GetPlannedZfwKg());

    QVERIFY(std::abs(gateway.WrittenAVar(kStation3) - 2500.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation5) - 4500.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation4) - 1000.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.WrittenAVar(kStation6) - 2000.0) < kKgTolerance);
}

void FssEJetTest::loadingNeverTouchesTheCrewStations()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 5940.0, 66);
    GiveCargo(status, 660.0);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.SetCurrentZfwKg(aircraft.GetPlannedZfwKg());

    QCOMPARE(gateway.AVarWriteCount(kStation1), 0);
    QCOMPARE(gateway.AVarWriteCount(kStation2), 0);
}

void FssEJetTest::loadingWritesNothingUntilTheEmptyWeightArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 6000.0, 50);

    aircraft.SetCurrentZfwKg(40000.0);

    QCOMPARE(gateway.setAVarCalls, 0);
}

void FssEJetTest::mirrorsThePlannedCargoIntoThePlaneSetupWeightsOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 5940.0, 66);
    GiveCargo(status, 660.0);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.OnLoadingStarted();

    QVERIFY(std::abs(gateway.Written(kPlanWeightZoneA) - 1140.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.Written(kPlanWeightZoneB) - 4140.0) < kKgTolerance);
    QVERIFY(std::abs(gateway.Written(kPlanWeightCargoFwd) - 475.2) < kKgTolerance);
    QVERIFY(std::abs(gateway.Written(kPlanWeightCargoAft) - 184.8) < kKgTolerance);

    for (const char* lVar : {kPlanWeightZoneA, kPlanWeightZoneB, kPlanWeightCargoFwd, kPlanWeightCargoAft})
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
    }

    aircraft.OnLoadingStarted();

    for (const char* lVar : {kPlanWeightZoneA, kPlanWeightZoneB, kPlanWeightCargoFwd, kPlanWeightCargoAft})
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
    }
}

void FssEJetTest::mirrorsNoPlaneSetupWeightsWithoutACargoLineInThePlan()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 5940.0, 66);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.OnLoadingStarted();

    for (const char* lVar : {kPlanWeightZoneA, kPlanWeightZoneB, kPlanWeightCargoFwd, kPlanWeightCargoAft})
    {
        QCOMPARE(gateway.WriteCount(lVar), 0);
    }

    QCOMPARE(gateway.WriteCount(kNumPassengers), 1);
    QCOMPARE(gateway.WriteCount(kMaxNumPassengers), 1);
}

void FssEJetTest::reportsPlannedPassengersToGsxOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    PlanTheMeasuredFlight(status, 6000.0, 50);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.OnLoadingStarted();

    QCOMPARE(gateway.WriteCount(kNumPassengers), 1);
    QCOMPARE(gateway.Written(kNumPassengers), 50.0);
    QCOMPARE(gateway.WriteCount(kMaxNumPassengers), 1);
    QCOMPARE(gateway.Written(kMaxNumPassengers), kMaxPassengersE190);

    aircraft.OnLoadingStarted();

    QCOMPARE(gateway.WriteCount(kNumPassengers), 1);
    QCOMPARE(gateway.WriteCount(kMaxNumPassengers), 1);
}

void FssEJetTest::reportsTheMaxPassengersForTheType()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, false);

    PlanTheMeasuredFlight(status, 6000.0, 50);
    ParkWithTheMeasuredEmptyWeight(gateway, 34000.0);

    aircraft.OnLoadingStarted();

    QCOMPARE(gateway.Written(kMaxNumPassengers), kMaxPassengersE195);
}

QTEST_APPLESS_MAIN(FssEJetTest)

#include "tst_fss_e_jets.moc"
