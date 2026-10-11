#include <QtTest/QTest>

#include <QtCore/QTemporaryDir>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include "AircraftTicks.h"
#include "doubles/FakeVariableWriter.h"
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/domain/ports/GsxGateway.h"
#include "../src/infrastructure/aircraft/ifly/IFly737Max.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimAvionicsBusVoltage = "ELECTRICAL AVIONICS BUS VOLTAGE";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";

    constexpr auto kSmartSwitch = "VC_ACP_1_Push_to_Talk_SW_VAL";
    constexpr auto kParkingBrake = "VC_Parking_Brake_SW_VAL";
    constexpr auto kChocks = "iFly_NLG_Chock_Display_VAL";

    constexpr long long kOfpEpoch = 1786922025;

    void WriteIFlyPlan(const std::filesystem::path& appData, const long long epoch)
    {
        const std::filesystem::path directory = appData
            / "Microsoft Flight Simulator 2024" / "WASM" / "MSFS2020"
            / "ifly-aircraft-737max8" / "work" / "navdata" / "FLTPLAN";

        std::filesystem::create_directories(directory);

        std::ofstream stream(directory / "activeflightplan.xml", std::ios::binary);
        stream << "<?xml version=\"1.0\"?><OFP><params><time_generated>"
            << epoch
            << "</time_generated><units>kgs</units></params></OFP>";
    }

    constexpr auto kFwdCargoAnim = "Animation_FWD_Cargo_VAL";
    constexpr auto kFwdEntryAnim = "ANIMATION_FWD_ENTRY_VAL";
    constexpr auto kAftServiceAnim = "ANIMATION_AFT_SERVICE_VAL";
    constexpr auto kAftCargoAnim = "Animation_AFT_Cargo_VAL";

    constexpr std::array kAllDoorAnims = {
        kFwdCargoAnim, kAftCargoAnim,
        kFwdEntryAnim, "ANIMATION_FWD_SERVICE_VAL",
        "ANIMATION_AFT_ENTRY_VAL", kAftServiceAnim,
        "ANIMATION_L_FWD_OVERWING_VAL", "ANIMATION_R_FWD_OVERWING_VAL",
        "ANIMATION_L_AFT_OVERWING_VAL", "ANIMATION_R_AFT_OVERWING_VAL"
    };

    constexpr double kEmptyOperatingZfwKg = 45070.0;

    constexpr int kQuietTicks = 20;
    constexpr int kBudgetTicks = 200;
    constexpr int kWritesPerBudget = 10;
    constexpr int kLoaderReactionTicks = 12;

    double State(const GsxStateStatus status)
    {
        return static_cast<double>(status);
    }

    TurnaroundFacts ResumedAt(const TurnaroundPhase phase)
    {
        TurnaroundFacts facts;
        facts.phase = phase;

        return facts;
    }

    void CloseEveryDoorAnimation(FakeVariableGateway& gateway)
    {
        for (const char* animLVar : kAllDoorAnims)
        {
            gateway.lvars[animLVar] = 0.0;
        }
    }
}

class IFly737MaxTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsCargoVariant();
    static void evaluatingTheDoorRuleWritesNoVariable();
    static void evaluatingThePlanImportRuleWritesNoVariable();
    static void planRuleReadsTheFileAndWaitsForTheMatchingEpoch();
    static void readsCurrentFuelFromSim();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwDoesNotDropBelowEmptyWeight();
    static void plannedValuesComeFromSession();
    static void flightPlanLoadedWhenSessionReady();
    static void smartSwitchInactiveUntilSimDataArrives();
    static void smartSwitchConsumesWithoutWritingLVar();
    static void smartSwitchRegistersFastRefresh();
    static void smartSwitchReportsSinglePressPerActivation();
    static void smartSwitchDetectsBothDirections();
    static void emptyZfwReadsSimEmptyWeight();
    static void fuelSetterDoesNotWriteToSim();
    static void zfwSetterDistributesPayloadAcrossStations();
    static void zfwSetterHoldsUntilEmptyWeightArrives();
    static void zfwSetterClampsPayloadAtZero();
    static void zfwSetterSkipsRepeatedValue();
    static void parkingBrakeReadsTheSwitchAndIgnoresTheSimVar();
    static void heldInPlaceAcceptsChocksWithoutTheSwitch();
    static void readyToDeboardFollowsSafetyState();
    static void aircraftPowerFollowsAvionicsBus();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void engineRunningDetectsAnyCombustion();
    static void engineAssumedRunningUntilCombustionDataArrives();
    static void doorStatusOpenWhenACargoDoorAnimationReadsOpen();
    static void doorStatusUnknownUntilCargoDoorDataArrives();
    static void doorStatusAllClosedOnceEveryDoorReadsShut();
    static void doorStatusOpenWhenAPassengerDoorReadsOpen();
    static void doorStatusUnknownWhileAPassengerDoorHasNotAnswered();
    static void reportsLoadMethods();
    static void closesEachCargoDoorAsItsLoaderFinishes();
    static void waitsWhileLoadersUnloadCargo();
    static void closesDoorWhenLoaderVanishesWithoutRemovingState();
    static void ignoresStaleLoaderStateFromPreviousTurnaround();
    static void doesNotCloseDoorsWhenNoLoaderEverCame();
    static void ignoresCargoDoorsOutsideALoadingCycle();
    static void leavesClosedCargoDoorsAlone();
    static void givesUpPulsingAfterMaxAttempts();
    static void restartsClosingWhenBoardingStarts();
    static void closesBothCargoDoorsOnceBoardingCompletes();
    static void closesBothCargoDoorsWhenBoardingEndsWithoutSayingCompleted();
    static void doesNotPulseWhileBoardingIsStillRunning();
    static void waitsWhileALoaderIsStillFinishingAtTheDoor();
    static void reopensACargoDoorThatShutWithItsLoaderStillAtTheDoor();
    static void doesNotReopenOnceTheLoaderHasLeft();
    static void leavesTheDoorAloneWhileItIsAlreadyClosing();
    static void closesThePassengerDoorsWhenBoardingEndsHoldsThemClosed();
    static void leavesPassengerDoorsAloneUntilTheHoldIsAsked();
    static void opensThePassengerDoorWhenTheJetwayDocks();
    static void leavesThePassengerDoorOpenWhileTheJetwayIsOnItsWay();
    static void keepsThePassengerDoorClosedForDepartureDespiteTheJetway();
    static void leavesThePassengerDoorAloneWhileTheJetwayIsStillOperating();
    static void theTurnaroundStartDoesNotRefillTheReopenBudget();
    static void closesACargoDoorTheLoaderCycleOpenedWhenTheAircraftIsHeldForDeparture();
    static void closesACargoDoorWhenCloseAllDoorsIsRequestedAndTheLoaderIsGone();
    static void keepsACargoDoorOpenWhileItsLoaderIsAtTheDoorAfterCloseAllDoors();
    static void closesACargoDoorWhoseLoaderStateOutlivedACouatlRestart();
    static void actsOnALoaderStateOnlyOnceItChangesAfterACouatlRestart();
    static void isReachableOnceTheFuelAndTheEmptyWeightHaveArrived();
    static void aResumedAircraftKeepsTheDoorShutForDepartureUnderADockedJetway();
    static void holdingTheDoorsClosedOnANewbornAircraftClosesOnlyWhatIsOpen();
    static void resumingNeverAsksForTheDoorsToBeClosed();
    static void theCloseRequestSurvivesTheRelaunchThroughTheMemory();
    static void aResumeWithoutMemoryDoesNotCloseAnOpenCargoDoor();
    static void aGsxRestartedSinceTheSaveDistrustsTheVehicleStatesAtTheResume();
    static void aToggleLeftHighByTheDeadProcessIsReleased();
    static void theCloseRequestIsForgottenWhenTheNextTurnaroundStarts();
    static void theRuleForgetsItsLatchedCloseRequestWhenTheNextTurnaroundStarts();
    static void thePassengerHoldClosesTheEntryDoorsUnderADockedJetway();
    static void thePassengerHoldLeavesTheCateringAndCargoDoorsAlone();
    static void thePassengerHoldKeepsTheEntryDoorShut();
    static void releasingTheDepartureHoldAlsoReleasesThePassengerHold();
    static void theDepartureHoldGivesTheEntryDoorsANewBudgetAfterThePassengerHoldSpentTheirs();
};

void IFly737MaxTest::reportsCargoVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QVERIFY(!aircraft.IsCargoVariant());
}

void IFly737MaxTest::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimFuelTotalKg] = 8200.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 8200.0);
}

void IFly737MaxTest::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 70000.0;
    gateway.avars[kSimFuelTotalKg] = 9000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 61000.0);
}

void IFly737MaxTest::currentZfwDoesNotDropBelowEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 40000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), kEmptyOperatingZfwKg);
}

void IFly737MaxTest::plannedValuesComeFromSession()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    status.plannedFuelKg = 7600.0;
    status.plannedZfwKg = 62500.0;
    status.plannedPassengers = 178;;

    QCOMPARE(aircraft.GetPlannedFuelKg(), 7600.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 62500.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 178);
}

void IFly737MaxTest::flightPlanLoadedWhenSessionReady()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.flightPlanStatus = FlightPlanStatus::Ready;

    QVERIFY(aircraft.IsFlightPlanLoaded());
}

void IFly737MaxTest::smartSwitchInactiveUntilSimDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    QVERIFY(!aircraft.ConsumeSmartSwitch());
}

void IFly737MaxTest::smartSwitchConsumesWithoutWritingLVar()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kSmartSwitch] = 20.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
    QVERIFY(!aircraft.ConsumeSmartSwitch());
}

void IFly737MaxTest::smartSwitchRegistersFastRefresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    QCOMPARE(gateway.fastRefreshNames.size(), static_cast<std::size_t>(1));
    QVERIFY(gateway.fastRefreshNames.front() == kSmartSwitch);
}

void IFly737MaxTest::smartSwitchReportsSinglePressPerActivation()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kSmartSwitch] = 20.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 10.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 20.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
}

void IFly737MaxTest::smartSwitchDetectsBothDirections()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 10.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 20.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
}

void IFly737MaxTest::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = 45500.0;

    QCOMPARE(aircraft.GetEmptyZfwKg(), 45500.0);
}

void IFly737MaxTest::fuelSetterDoesNotWriteToSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    aircraft.SetCurrentFuelKg(7600.0);
    TickAircraft(aircraft, gateway);
    SlowTickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
    QCOMPARE(gateway.setAVarCalls, 0);
}

void IFly737MaxTest::zfwSetterDistributesPayloadAcrossStations()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentZfwKg(kEmptyOperatingZfwKg + 20000.0);

    QCOMPARE(gateway.setAVarCalls, 9);
    QCOMPARE(gateway.setLVarCalls, 0);

    double totalKg = 0.0;
    for (int i = 1; i <= 9; ++i)
    {
        totalKg += gateway.avars["PAYLOAD STATION WEIGHT:" + std::to_string(i)];
    }

    QVERIFY(std::abs(totalKg - 20000.0) < 0.001);

    const double paxB = gateway.avars["PAYLOAD STATION WEIGHT:2"];
    const double cargoAft = gateway.avars["PAYLOAD STATION WEIGHT:9"];

    QVERIFY(std::abs(paxB - (20000.0 * 5250.0 / 34070.0)) < 0.001);
    QVERIFY(std::abs(cargoAft - (20000.0 * 8018.0 / 34070.0)) < 0.001);
}

void IFly737MaxTest::zfwSetterHoldsUntilEmptyWeightArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    aircraft.SetCurrentZfwKg(62500.0);

    QCOMPARE(gateway.setAVarCalls, 0);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentZfwKg(62500.0);

    QCOMPARE(gateway.setAVarCalls, 9);
}

void IFly737MaxTest::zfwSetterClampsPayloadAtZero()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentZfwKg(kEmptyOperatingZfwKg - 5000.0);

    for (int i = 1; i <= 9; ++i)
    {
        QCOMPARE(gateway.avars["PAYLOAD STATION WEIGHT:" + std::to_string(i)], 0.0);
    }
}

void IFly737MaxTest::zfwSetterSkipsRepeatedValue()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentZfwKg(60000.0);
    aircraft.SetCurrentZfwKg(60000.0);

    QCOMPARE(gateway.setAVarCalls, 9);

    aircraft.SetCurrentZfwKg(61000.0);

    QCOMPARE(gateway.setAVarCalls, 18);
}

void IFly737MaxTest::parkingBrakeReadsTheSwitchAndIgnoresTheSimVar()
{
    struct TestCase
    {
        const char* name;
        double lever;
        double simBrake;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "released", .lever = 0.0, .simBrake = 0.0, .expected = false},
        TestCase{.name = "switch only", .lever = 1.0, .simBrake = 0.0, .expected = true},
        TestCase{.name = "cold sim brake lies", .lever = 0.0, .simBrake = 1.0, .expected = false},
        TestCase{.name = "both", .lever = 1.0, .simBrake = 1.0, .expected = true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.lvars[kParkingBrake] = testCase.lever;
        gateway.avars[kSimParkingBrake] = testCase.simBrake;

        QVERIFY2(aircraft.IsParkingBrakeSet() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::heldInPlaceAcceptsChocksWithoutTheSwitch()
{
    struct TestCase
    {
        const char* name;
        double lever;
        double chocks;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "rolling", .lever = 0.0, .chocks = 0.0, .expected = false},
        TestCase{.name = "switch only", .lever = 1.0, .chocks = 0.0, .expected = true},
        TestCase{.name = "chocks only", .lever = 0.0, .chocks = 1.0, .expected = true},
        TestCase{.name = "both", .lever = 1.0, .chocks = 1.0, .expected = true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.lvars[kParkingBrake] = testCase.lever;
        gateway.lvars[kChocks] = testCase.chocks;

        QVERIFY2(aircraft.IsHeldInPlace() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::readyToDeboardFollowsSafetyState()
{
    struct TestCase
    {
        const char* name;
        double parkingBrake;
        double chocks;
        double beacon;
        double combustion;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "brake set", .parkingBrake = 1.0, .chocks = 0.0, .beacon = 0.0, .combustion = 0.0,
                 .expected = true},
        TestCase{.name = "chocks set", .parkingBrake = 0.0, .chocks = 1.0, .beacon = 0.0, .combustion = 0.0,
                 .expected = true},
        TestCase{.name = "engine running", .parkingBrake = 1.0, .chocks = 0.0, .beacon = 0.0, .combustion = 1.0,
                 .expected = false},
        TestCase{.name = "beacon on", .parkingBrake = 1.0, .chocks = 0.0, .beacon = 1.0, .combustion = 0.0,
                 .expected = false},
        TestCase{.name = "brake released", .parkingBrake = 0.0, .chocks = 0.0, .beacon = 0.0, .combustion = 0.0,
                 .expected = false},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.lvars[kParkingBrake] = testCase.parkingBrake;
        gateway.avars[kSimParkingBrake] = testCase.parkingBrake;
        gateway.lvars[kChocks] = testCase.chocks;
        gateway.avars[kSimBeaconLight] = testCase.beacon;
        gateway.avars[kSimEng1Combustion] = 0.0;
        gateway.avars[kSimEng2Combustion] = testCase.combustion;

        QVERIFY2(aircraft.IsReadyToDeboard() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::aircraftPowerFollowsAvionicsBus()
{
    struct TestCase
    {
        const char* name;
        double busVoltage;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "dark", .busVoltage = 0.0, .expected = false},
        TestCase{.name = "powered", .busVoltage = 28.0, .expected = true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.avars[kSimAvionicsBusVoltage] = testCase.busVoltage;

        QVERIFY2(aircraft.IsPowered() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::readyToPushFollowsPowerBeaconAndEngines()
{
    struct TestCase
    {
        const char* name;
        double busVoltage;
        double beacon;
        double combustion;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "ready", .busVoltage = 28.0, .beacon = 1.0, .combustion = 0.0, .expected = true},
        TestCase{.name = "dark", .busVoltage = 0.0, .beacon = 1.0, .combustion = 0.0, .expected = false},
        TestCase{.name = "beacon off", .busVoltage = 28.0, .beacon = 0.0, .combustion = 0.0, .expected = false},
        TestCase{.name = "engine running", .busVoltage = 28.0, .beacon = 1.0, .combustion = 1.0, .expected = false},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.avars[kSimAvionicsBusVoltage] = testCase.busVoltage;
        gateway.avars[kSimBeaconLight] = testCase.beacon;
        gateway.avars[kSimEng1Combustion] = 0.0;
        gateway.avars[kSimEng2Combustion] = testCase.combustion;

        QVERIFY2(aircraft.IsReadyToPush() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::engineRunningDetectsAnyCombustion()
{
    struct TestCase
    {
        const char* name;
        double eng1Combustion;
        double eng2Combustion;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{.name = "stopped", .eng1Combustion = 0.0, .eng2Combustion = 0.0, .expected = false},
        TestCase{.name = "engine 1", .eng1Combustion = 1.0, .eng2Combustion = 0.0, .expected = true},
        TestCase{.name = "engine 2", .eng1Combustion = 0.0, .eng2Combustion = 1.0, .expected = true},
        TestCase{.name = "both", .eng1Combustion = 1.0, .eng2Combustion = 1.0, .expected = true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        IFly737Max aircraft(&gateway, &status);

        gateway.avars[kSimEng1Combustion] = testCase.eng1Combustion;
        gateway.avars[kSimEng2Combustion] = testCase.eng2Combustion;

        QVERIFY2(aircraft.IsEngineRunning() == testCase.expected, testCase.name);
    }
}

void IFly737MaxTest::engineAssumedRunningUntilCombustionDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QVERIFY(aircraft.IsEngineRunning());

    gateway.avars[kSimEng1Combustion] = 0.0;

    QVERIFY(aircraft.IsEngineRunning());

    gateway.avars[kSimEng2Combustion] = 0.0;

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kSimEng2Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void IFly737MaxTest::reportsLoadMethods()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Gsx);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Client);
    QVERIFY(aircraft.SupportsStairsOrJetways());
    QVERIFY(aircraft.RequiresEfbFlightPlan());
}

void IFly737MaxTest::closesEachCargoDoorAsItsLoaderFinishes()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRetracting;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), -1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdCargoAnim] = 0.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 0.0);

    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kAftCargoAnim] = 0.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 0.0);

    const int callsAfterPulse = gateway.setLVarCalls;

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, callsAfterPulse);
}

void IFly737MaxTest::waitsWhileLoadersUnloadCargo()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 15; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), -1.0);
}

void IFly737MaxTest::closesDoorWhenLoaderVanishesWithoutRemovingState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), -1.0);
}

void IFly737MaxTest::doesNotCloseDoorsWhenNoLoaderEverCame()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 1.0;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Completed);

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::ignoresCargoDoorsOutsideALoadingCycle()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Callable);
    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Callable);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::leavesClosedCargoDoorsAlone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    gateway.lvars[kFwdCargoAnim] = 50.0;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::givesUpPulsingAfterMaxAttempts()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;

    for (int tick = 0; tick < 100; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 10);
}

void IFly737MaxTest::restartsClosingWhenBoardingStarts()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::closesBothCargoDoorsWhenBoardingEndsWithoutSayingCompleted()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderLoading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 15; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Callable);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRetracting;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderRetracting;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 1.0);
}

void IFly737MaxTest::closesBothCargoDoorsOnceBoardingCompletes()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderLoading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 15; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Completed);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdCargoAnim] = 0.0;
    gateway.lvars[kAftCargoAnim] = 0.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 0.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 0.0);

    const int callsAfterPulse = gateway.setLVarCalls;

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, callsAfterPulse);
}

void IFly737MaxTest::doesNotPulseWhileBoardingIsStillRunning()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::reopensACargoDoorThatShutWithItsLoaderStillAtTheDoor()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[kFwdCargoAnim] = 0.0;
    gateway.lvars[kAftCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 0.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), 0.0);
}

void IFly737MaxTest::doesNotReopenOnceTheLoaderHasLeft()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    gateway.lvars[kFwdCargoAnim] = 0.0;

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::waitsWhileALoaderIsStillFinishingAtTheDoor()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderFinishing;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Completed);

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::closesThePassengerDoorsWhenBoardingEndsHoldsThemClosed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars[kAftServiceAnim] = 100.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdEntryAnim] = 0.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 0.0);

    for (int tick = 0; tick < 16; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftService2Toggle), 1.0);
}

void IFly737MaxTest::leavesThePassengerDoorOpenWhileTheJetwayIsOnItsWay()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars[gsx::lvars::kJetway] = 1.0;
    gateway.lvars[gsx::lvars::kOperateJetwaysState] = State(GsxStateStatus::Requested);
    aircraft.CloseAllDoors();

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kOperateJetwaysState] = State(GsxStateStatus::Callable);

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(gateway.setLVarCalls > 0);
}

void IFly737MaxTest::opensThePassengerDoorWhenTheJetwayDocks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdEntryAnim] = 0.0;
    aircraft.CloseAllDoors();

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;

    for (int tick = 0; tick < 9; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdEntryAnim] = 100.0;

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 0.0);

    const int callsAfterPulse = gateway.setLVarCalls;

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, callsAfterPulse);
}

void IFly737MaxTest::theTurnaroundStartDoesNotRefillTheReopenBudget()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 0.0;

    for (int tick = 0; tick < 11; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 2);

    aircraft.CloseAllDoors();

    for (int tick = 0; tick < 80; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 10);
}

void IFly737MaxTest::leavesThePassengerDoorAloneWhileTheJetwayIsStillOperating()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = static_cast<double>(GsxStateStatus::Requested);
    gateway.lvars[kFwdEntryAnim] = 100.0;

    aircraft.CloseAllDoors();

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kJetway] = static_cast<double>(GsxStateStatus::Callable);
    aircraft.CloseAllDoors();
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);
}

void IFly737MaxTest::keepsThePassengerDoorClosedForDepartureDespiteTheJetway()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 100.0;

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdEntryAnim] = 0.0;

    const int callsAfterPulse = gateway.setLVarCalls;

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, callsAfterPulse);
}

void IFly737MaxTest::leavesPassengerDoorsAloneUntilTheHoldIsAsked()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars[kAftServiceAnim] = 100.0;

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.HoldDoorsClosed(false);

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::leavesTheDoorAloneWhileItIsAlreadyClosing()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Active);
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBoardingState] = State(GsxStateStatus::Completed);
    gateway.lvars[kFwdCargoAnim] = 98.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[kFwdCargoAnim] = 95.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[kFwdCargoAnim] = 20.0;
    TickAircraft(aircraft, gateway);
    gateway.lvars[kFwdCargoAnim] = 0.0;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::ignoresStaleLoaderStateFromPreviousTurnaround()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = gsx::states::kLoaderInPosition;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 100.0;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = 1.0;
    gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = 1.0;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderRemoving;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo2Toggle), -1.0);
}

void IFly737MaxTest::closesACargoDoorTheLoaderCycleOpenedWhenTheAircraftIsHeldForDeparture()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;

    for (int tick = 0; tick < 3; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::closesACargoDoorWhenCloseAllDoorsIsRequestedAndTheLoaderIsGone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdCargoAnim] = 100.0;

    for (int tick = 0; tick < 3; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.CloseAllDoors();
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::keepsACargoDoorOpenWhileItsLoaderIsAtTheDoorAfterCloseAllDoors()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderInPosition;
    gateway.lvars[kFwdCargoAnim] = 100.0;

    aircraft.CloseAllDoors();

    for (int tick = 0; tick < kQuietTicks; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::closesACargoDoorWhoseLoaderStateOutlivedACouatlRestart()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderUnloading;
    gateway.lvars[kFwdCargoAnim] = 100.0;

    for (int tick = 0; tick < 3; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kCouatlStarted] = 0.0;
    TickAircraft(aircraft, gateway);
    gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::actsOnALoaderStateOnlyOnceItChangesAfterACouatlRestart()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    gateway.lvars[gsx::lvars::kDeboardingState] = State(GsxStateStatus::Active);
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderInPosition;
    gateway.lvars[kFwdCargoAnim] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars[gsx::lvars::kCouatlStarted] = 0.0;
    TickAircraft(aircraft, gateway);
    gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;

    for (int tick = 0; tick < kQuietTicks; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;

    for (int tick = 0; tick < kLoaderReactionTicks && gateway.setLVarCalls == 0; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::doorStatusOpenWhenACargoDoorAnimationReadsOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[kAftCargoAnim] = 0.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void IFly737MaxTest::doorStatusUnknownUntilCargoDoorDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void IFly737MaxTest::doorStatusAllClosedOnceEveryDoorReadsShut()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    for (const char* animLVar : kAllDoorAnims)
    {
        gateway.lvars[animLVar] = 0.0;
    }

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);
}

void IFly737MaxTest::doorStatusOpenWhenAPassengerDoorReadsOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    for (const char* animLVar : kAllDoorAnims)
    {
        gateway.lvars[animLVar] = 0.0;
    }
    gateway.lvars[kFwdEntryAnim] = 100.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void IFly737MaxTest::doorStatusUnknownWhileAPassengerDoorHasNotAnswered()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    for (const char* animLVar : kAllDoorAnims)
    {
        gateway.lvars[animLVar] = 0.0;
    }
    gateway.lvars.erase(kAftServiceAnim);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}


void IFly737MaxTest::evaluatingTheDoorRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);
    FakeVariableWriter writer;

    gateway.lvars[kFwdEntryAnim] = 0.0;
    aircraft.CloseAllDoors();
    gateway.lvars[gsx::lvars::kJetway] = 5.0;

    AircraftRule* const rule = FindRule(aircraft, "ifly-737max-doors-follow-loader-cycle");

    QVERIFY(rule != nullptr);

    const RuleContext context{};

    for (int tick = 0; tick < 20; ++tick)
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, 0);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);

    for (int tick = 0; tick < 10; ++tick)
    {
        rule->Act(context, writer);
    }

    QCOMPARE(writer.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);
}

void IFly737MaxTest::evaluatingThePlanImportRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);
    FakeVariableWriter writer;

    AircraftRule* const rule = FindRule(aircraft, "ifly-737max-watch-plan-file");

    QVERIFY(rule != nullptr);
    QVERIFY(rule->Cadence() == RuleCadence::Slow);

    const RuleContext context{};

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, 0);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);
}

void IFly737MaxTest::planRuleReadsTheFileAndWaitsForTheMatchingEpoch()
{
    const QTemporaryDir appData;
    const std::filesystem::path root(appData.path().toStdWString());

    FakeVariableGateway gateway;
    AutomationStatus status;
    status.flightPlanStatus = FlightPlanStatus::Ready;
    status.planGeneratedEpoch = kOfpEpoch;

    IFly737Max aircraft(&gateway, &status, root);

    AircraftRule* const rule = FindRule(aircraft, "ifly-737max-watch-plan-file");

    QVERIFY(rule != nullptr);

    const RuleContext context{};

    QVERIFY(aircraft.IsFlightPlanLoaded());

    WriteIFlyPlan(root, kOfpEpoch - 1);
    static_cast<void>(rule->Evaluate(context));

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    WriteIFlyPlan(root, kOfpEpoch);
    static_cast<void>(rule->Evaluate(context));

    QVERIFY(aircraft.IsFlightPlanLoaded());
}

void IFly737MaxTest::isReachableOnceTheFuelAndTheEmptyWeightHaveArrived()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    QVERIFY(!aircraft.IsReachable());

    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QVERIFY(!aircraft.IsReachable());

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;

    QVERIFY(aircraft.IsReachable());
}

void IFly737MaxTest::aResumedAircraftKeepsTheDoorShutForDepartureUnderADockedJetway()
{
    TurnaroundFacts facts = ResumedAt(TurnaroundPhase::WaitingReadyToPush);
    facts.loadingStarted = true;
    facts.refuelFinished = true;
    facts.boardingFinished = true;
    facts.departureDoorsHeld = true;

    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 0.0;

    aircraft.OnTurnaroundResumed(facts, MemoryBag{});
    aircraft.HoldDoorsClosed(true);

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    FakeVariableGateway unheldGateway;
    IFly737Max unheld(&unheldGateway, &status);

    unheldGateway.lvars[gsx::lvars::kJetway] = 5.0;
    unheldGateway.lvars[kFwdEntryAnim] = 0.0;

    unheld.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(unheld, unheldGateway);
    }

    QVERIFY(unheldGateway.setLVarCalls > 0);
}

void IFly737MaxTest::holdingTheDoorsClosedOnANewbornAircraftClosesOnlyWhatIsOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    CloseEveryDoorAnimation(gateway);
    gateway.lvars[kFwdEntryAnim] = 100.0;

    aircraft.HoldDoorsClosed(true);

    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(gateway.WriteCount(gsx::lvars::kAircraftExit1Toggle) > 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftExit4Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftService1Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftService2Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftCargo1Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftCargo2Toggle), 0);
    QVERIFY(!aircraft.WasCloseRequested());
}

void IFly737MaxTest::resumingNeverAsksForTheDoorsToBeClosed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    CloseEveryDoorAnimation(gateway);
    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;

    TurnaroundFacts facts = ResumedAt(TurnaroundPhase::Loading);
    facts.loadingStarted = true;
    facts.gsxRestartedSinceSave = true;

    aircraft.OnTurnaroundResumed(facts, MemoryBag{});

    QVERIFY(!aircraft.WasCloseRequested());

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::theCloseRequestSurvivesTheRelaunchThroughTheMemory()
{
    AutomationStatus status;
    FakeVariableGateway deadGateway;
    IFly737Max dead(&deadGateway, &status);

    dead.CloseAllDoors();

    const MemoryBag memory = dead.TurnaroundMemory();

    FakeVariableGateway gateway;
    IFly737Max relaunched(&gateway, &status);

    gateway.lvars[kFwdCargoAnim] = 100.0;

    relaunched.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), memory);

    QVERIFY(relaunched.WasCloseRequested());

    TickAircraft(relaunched, gateway);
    TickAircraft(relaunched, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftCargo1Toggle), 1.0);
}

void IFly737MaxTest::aResumeWithoutMemoryDoesNotCloseAnOpenCargoDoor()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[kFwdCargoAnim] = 100.0;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::aGsxRestartedSinceTheSaveDistrustsTheVehicleStatesAtTheResume()
{
    AutomationStatus status;

    const auto writesAfterResuming = [&status](const bool gsxRestartedSinceSave)
    {
        FakeVariableGateway gateway;
        IFly737Max aircraft(&gateway, &status);

        gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 3.0;
        gateway.lvars[kFwdEntryAnim] = 0.0;

        TurnaroundFacts facts = ResumedAt(TurnaroundPhase::Loading);
        facts.gsxRestartedSinceSave = gsxRestartedSinceSave;
        aircraft.OnTurnaroundResumed(facts, MemoryBag{});

        for (int tick = 0; tick < 20; ++tick)
        {
            TickAircraft(aircraft, gateway);
        }

        return gateway.setLVarCalls;
    };

    QCOMPARE(writesAfterResuming(true), 0);
    QVERIFY(writesAfterResuming(false) > 0);
}

void IFly737MaxTest::aToggleLeftHighByTheDeadProcessIsReleased()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    CloseEveryDoorAnimation(gateway);

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[gsx::lvars::kAircraftExit1Toggle] = 1.0;
    gateway.lvars[gsx::lvars::kAircraftCargo2Toggle] = 0.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 0.0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftExit1Toggle), 1);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftCargo2Toggle), 0);
    QCOMPARE(gateway.setLVarCalls, 1);

    FakeVariableGateway untouchedGateway;
    IFly737Max untouched(&untouchedGateway, &status);

    CloseEveryDoorAnimation(untouchedGateway);
    untouchedGateway.lvars[gsx::lvars::kAircraftExit1Toggle] = 1.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        TickAircraft(untouched, untouchedGateway);
    }

    QCOMPARE(untouchedGateway.setLVarCalls, 0);
}

void IFly737MaxTest::theCloseRequestIsForgottenWhenTheNextTurnaroundStarts()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    aircraft.CloseAllDoors();

    QVERIFY(aircraft.WasCloseRequested());

    aircraft.OnTurnaroundStarted();

    QVERIFY(!aircraft.WasCloseRequested());

    gateway.lvars[kFwdCargoAnim] = 100.0;

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::theRuleForgetsItsLatchedCloseRequestWhenTheNextTurnaroundStarts()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 100.0;

    aircraft.CloseAllDoors();
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.OnTurnaroundStarted();
    gateway.lvars[gsx::lvars::kJetway] = 3.0;

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::thePassengerHoldClosesTheEntryDoorsUnderADockedJetway()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars["ANIMATION_AFT_ENTRY_VAL"] = 100.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.HoldPassengerDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);
    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit4Toggle), 1.0);
}

void IFly737MaxTest::thePassengerHoldLeavesTheCateringAndCargoDoorsAlone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 100.0;
    gateway.lvars["ANIMATION_FWD_SERVICE_VAL"] = 100.0;
    gateway.lvars[kAftServiceAnim] = 100.0;
    gateway.lvars[kFwdCargoAnim] = 100.0;
    gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderInPosition;

    aircraft.HoldPassengerDoorsClosed(true);

    for (int tick = 0; tick < 30; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(gateway.WriteCount(gsx::lvars::kAircraftExit1Toggle) > 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftService1Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftService2Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftCargo1Toggle), 0);
    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftCargo2Toggle), 0);
}

void IFly737MaxTest::thePassengerHoldKeepsTheEntryDoorShut()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 0.0;

    aircraft.HoldPassengerDoorsClosed(true);

    for (int tick = 0; tick < 40; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void IFly737MaxTest::releasingTheDepartureHoldAlsoReleasesThePassengerHold()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 0.0;

    aircraft.HoldPassengerDoorsClosed(true);
    aircraft.HoldDoorsClosed(false);

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.Written(gsx::lvars::kAircraftExit1Toggle), 1.0);
}

void IFly737MaxTest::theDepartureHoldGivesTheEntryDoorsANewBudgetAfterThePassengerHoldSpentTheirs()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    gateway.lvars[gsx::lvars::kJetway] = 5.0;
    gateway.lvars[kFwdEntryAnim] = 100.0;

    aircraft.HoldPassengerDoorsClosed(true);

    for (int tick = 0; tick < kBudgetTicks; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftExit1Toggle), kWritesPerBudget);

    aircraft.HoldDoorsClosed(true);

    for (int tick = 0; tick < kBudgetTicks; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(gsx::lvars::kAircraftExit1Toggle), 2 * kWritesPerBudget);
}

QTEST_APPLESS_MAIN(IFly737MaxTest)

#include "tst_ifly_737max.moc"
