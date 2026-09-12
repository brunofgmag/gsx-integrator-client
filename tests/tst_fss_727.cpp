#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "AircraftTicks.h"
#include "TestDoubles.h"
#include "doubles/FakeGsxService.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/infrastructure/aircraft/fss/Fss727.h"

namespace
{
    constexpr auto kAcPowerAvailable = "FSS_B727_FE_ELEC_AC_PWR_AVAIL";

    constexpr auto kEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kEng3Combustion = "ENG COMBUSTION:3";

    constexpr auto kParkBrakeLever = "FSS_B727_PDSTL_PARK_BRAKE_LEVER";
    constexpr auto kChocks = "FSS_B727_EFB_CHOCKS_VISIBLE";
    constexpr auto kCones = "FSS_B727_EFB_CONES_VISIBLE";
    constexpr auto kEngineCovers = "FSS_B727_EFB_COVER_ENGINE_VISIBLE";
    constexpr auto kBoardingStair = "FSS_B727_EFB_BOARDING_STAIR";
    constexpr std::array kOwnGroundEquipment = {kChocks, kCones, kEngineCovers, kBoardingStair};
    constexpr auto kGpuAvailable = "FSS_B727_GPU_AVAIL";
    constexpr auto kExtPowerSwitch = "FSS_B727_FE_ELEC_EXT_POWER_SWITCH";
    constexpr auto kAftStairLever = "FSS_B727_CD_AFT_STAIR_LEVER";

    constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    constexpr auto kFrontStairsState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSFRONT_STATE";
    constexpr auto kJetway = "FSDT_GSX_JETWAY";
    constexpr double kStairsDocked = 3.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kVehicleGone = 0.0;

    constexpr auto kEntryDoorGoal = "INTERACTIVE POINT GOAL:0";
    constexpr auto kAftStairGoal = "INTERACTIVE POINT GOAL:4";
    constexpr auto kForwardHoldGoal = "INTERACTIVE POINT GOAL:2";
    constexpr auto kAftHoldGoal = "INTERACTIVE POINT GOAL:3";
    constexpr auto kMainDeckGoal = "INTERACTIVE POINT GOAL:1";

    constexpr auto kPanelCover = "FSS_B727_CDP_MASTER_POWER_COVER_SWITCH";
    constexpr auto kPanelMaster = "FSS_B727_CDP_MASTER_POWER_SWITCH";
    constexpr auto kPanelDoorSwitch = "FSS_B727_CDP_CARGO_DOOR_SWITCH";

    constexpr int kThreeTicks = 3;
    constexpr int kTwentyTicks = 20;
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";

    constexpr auto kEntryDoorPoint = "INTERACTIVE POINT OPEN:0";
    constexpr auto kMainDeckDoorPoint = "INTERACTIVE POINT OPEN:1";
    constexpr auto kForwardHoldPoint = "INTERACTIVE POINT OPEN:2";
    constexpr std::array kDoorPoints = {
        kEntryDoorPoint, kMainDeckDoorPoint, kForwardHoldPoint,
        "INTERACTIVE POINT OPEN:3", "INTERACTIVE POINT OPEN:4"
    };

    constexpr double kPointHalfway = 0.5;
    constexpr double kMeasuredClosedSettle = 0.0099;
    constexpr double kMeasuredOpenSettle = 0.9902;

    constexpr int kPaxDoorDeadlineTicks = 15;
    constexpr int kMainDeckDoorDeadlineTicks = 120;

    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";
    constexpr auto kLeftTankCapacity = "FUELSYSTEM TANK CAPACITY:1";
    constexpr auto kCentreTankCapacity = "FUELSYSTEM TANK CAPACITY:2";
    constexpr auto kRightTankCapacity = "FUELSYSTEM TANK CAPACITY:3";

    constexpr double kFuelPoundsPerGallon = 6.7;
    constexpr double kWingTankGallons = 1780.0;
    constexpr double kCentreTankGallons = 4530.0;
    constexpr double kFuelCapacityKg = 24586.067;
    constexpr double kCapacityToleranceKg = 0.01;

    constexpr auto kAutomodeRule = "fss-727-keep-vendor-gsx-automode-off";
    constexpr auto kAutomodeDisabled = "FSS_B727_GSX_AUTOMODE_DISABLED";
    constexpr std::array kWasmKeysBornAtZero = {
        "FSS_ENABLE_GSX_SUPPORT", "FSS_GNDSVC_AUTO_DEPARTURE", "FSS_GNDSVC_AUTO_DEBOARDING",
        "FSS_GNDSVC_AUTO_NOTIFY_GOOD_ENGSTART"
    };
    constexpr int kTicksWithoutEcho = 5;
    constexpr int kFiftyTicks = 50;

    constexpr std::array kVariants = {Fss727::kName200F, Fss727::kName200ReFreighter};

    constexpr auto kPhone = "FSS_B727_PDSTL_PHONE_PICK_UP";
    constexpr double kPhoneFirstThird = 0.333;
    constexpr auto kSimBeaconLight = "LIGHT BEACON";

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

    void GiveTanks(FakeVariableGateway& gateway)
    {
        gateway.avars[kSimFuelWeightPerGallon] = kFuelPoundsPerGallon;
        gateway.avars[kLeftTankCapacity] = kWingTankGallons;
        gateway.avars[kCentreTankCapacity] = kCentreTankGallons;
        gateway.avars[kRightTankCapacity] = kWingTankGallons;
    }

    void AllEnginesStopped(FakeVariableGateway& gateway)
    {
        gateway.avars[kEng1Combustion] = 0.0;
        gateway.avars[kEng2Combustion] = 0.0;
        gateway.avars[kEng3Combustion] = 0.0;
    }

    void AllDoorPointsAt(FakeVariableGateway& gateway, const double position)
    {
        for (const char* point : kDoorPoints)
        {
            gateway.avars[point] = position;
        }
    }

    void MainDeckOpen(FakeVariableGateway& gateway)
    {
        AllDoorPointsAt(gateway, 0.0);
        gateway.avars[kMainDeckDoorPoint] = kMeasuredOpenSettle;
    }

    void TickTimes(Aircraft& aircraft, FakeVariableGateway& gateway, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            TickAircraft(aircraft, gateway);
        }
    }
}

class Fss727Test final : public QObject
{
    Q_OBJECT

private slots:
    static void poweredOnlyOnceTheEngineerPanelReportsAcPower();
    static void enginesAssumedRunningUntilTheThreeCombustionsArrive();
    static void theTailEngineAloneCountsAsRunning();
    static void parkingBrakeFollowsTheVendorLeverEvenWithTheAircraftCold();
    static void heldInPlaceAcceptsTheLeverOrTheChocks();
    static void doorStatusUnknownUntilTheFivePointsArrive();
    static void doorStatusAllClosedWithEveryPointAtZero();
    static void doorStatusOpenWithAPointFullyOpen();
    static void doorStatusUnknownWhileAPointTravels();
    static void travelThatOutlastsTheDeadlineReadsOpen();
    static void travelAfterAClosedSpellStartsItsDeadlineFresh();
    static void aDoorThatSettlesRestartsItsDeadline();
    static void mainDeckTravelGetsTheMainDeckDeadline();
    static void pointsSettledJustShortOfTheirEndsReadAsTheirEnds();
    static void fuelCapacityWaitsForTheWeightPerGallon();
    static void fuelCapacityWaitsForTheThreeTankCapacities();
    static void fuelCapacitySumsTheThreeTanksInKg();
    static void readsCurrentFuelFromSim();
    static void emptyZfwReadsSimEmptyWeight();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwHoldsAtZeroUntilEmptyWeightArrives();
    static void registersThePedestalPhoneForFastRefresh();
    static void aPhoneTouchFiresOnceFromItsFirstThird();
    static void aHeldPhoneFiresOnceAcrossThreeTicks();
    static void readsThePlanFromTheClientOfp();
    static void loadsThroughTheClientAndBoardsByGsxStairs();
    static void showsWeightsInPounds();
    static void everyVariantIsAFreighter();
    static void logsTheProfileNameItWasBuiltWith();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
    static void clearsTheFourOwnGroundEquipmentObjectsOnce();
    static void placesAndRemovesTheChocksByTheEfbLVar();
    static void groundPowerStatusFollowsTheAircraftOwnGpu();
    static void raisesItsOwnGpuOnceWhenTheGsxUnitConnectsAndStowsItWhenItLeaves();
    static void neverStowsAGroundPowerUnitItDidNotRaise();
    static void waitsForTheGsxUnitToBeKnownBeforeTouchingItsOwnGpu();
    static void leavesTheGroundPowerUnitCallToTheGsxMenu();
    static void theFrontEntryFollowsTheGsxStairs();
    static void theFrontEntryFollowsTheJetway();
    static void theFrontEntryStaysClosedOnceHeldForDeparture();
    static void neverCommandsTheAftAirstair();
    static void closingEveryDoorSendsTheThreeGoalsAndTheCargoPanel();
    static void keepsThePanelMasterOnUntilTheMainDeckReadsClosed();
    static void neverTurnsTheCargoDoorSwitchOffWhileTheGsxIsLoading();
    static void closesTheMainDeckOnceTheGsxIsDoneWithTheCargoDoors();
    static void leavesTheCargoPanelAloneWhenTheMainDeckIsAlreadyClosed();
    static void automodeRuleNeverHoldsThePhase();
    static void observingEvaluatingAndReadingWriteNoVariable();
    static void writesOnlyTheAutomodeKeyOnceAcrossFiftyTicks();
    static void rewritesTheAutomodeKeyWhenTheEfbTurnsItBackOn();
    static void waitsForTheAutomodeWriteToComeBackBeforeRetrying();
};

void Fss727Test::poweredOnlyOnceTheEngineerPanelReportsAcPower()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(aircraft.IsPowered());
}

void Fss727Test::enginesAssumedRunningUntilTheThreeCombustionsArrive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(aircraft.IsEngineRunning());

    AllEnginesStopped(gateway);

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kEng2Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void Fss727Test::theTailEngineAloneCountsAsRunning()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kEng1Combustion] = 0.0;
    gateway.avars[kEng2Combustion] = 0.0;

    QVERIFY(aircraft.IsEngineRunning());

    gateway.avars[kEng3Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void Fss727Test::parkingBrakeFollowsTheVendorLeverEvenWithTheAircraftCold()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kSimParkingBrake] = 1.0;
    gateway.lvars[kParkBrakeLever] = 0.0;

    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kAcPowerAvailable] = 0.0;
    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsParkingBrakeSet());
}

void Fss727Test::heldInPlaceAcceptsTheLeverOrTheChocks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kParkBrakeLever] = 0.0;
    gateway.lvars[kChocks] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kChocks] = 0.0;
    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
}

void Fss727Test::doorStatusUnknownUntilTheFivePointsArrive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::doorStatusAllClosedWithEveryPointAtZero()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);
}

void Fss727Test::doorStatusOpenWithAPointFullyOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kForwardHoldPoint] = 1.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::doorStatusUnknownWhileAPointTravels()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::travelThatOutlastsTheDeadlineReadsOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickTimes(aircraft, gateway, kPaxDoorDeadlineTicks - 1);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::travelAfterAClosedSpellStartsItsDeadlineFresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    TickTimes(aircraft, gateway, kPaxDoorDeadlineTicks + 5);

    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::aDoorThatSettlesRestartsItsDeadline()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickTimes(aircraft, gateway, kPaxDoorDeadlineTicks - 5);

    gateway.avars[kEntryDoorPoint] = 0.0;
    TickAircraft(aircraft, gateway);

    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickTimes(aircraft, gateway, kPaxDoorDeadlineTicks - 5);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::mainDeckTravelGetsTheMainDeckDeadline()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kMainDeckDoorPoint] = kPointHalfway;
    TickTimes(aircraft, gateway, kMainDeckDoorDeadlineTicks - 1);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::pointsSettledJustShortOfTheirEndsReadAsTheirEnds()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllDoorPointsAt(gateway, kMeasuredClosedSettle);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.avars[kForwardHoldPoint] = kMeasuredOpenSettle;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::fuelCapacityWaitsForTheWeightPerGallon()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    GiveTanks(gateway);
    gateway.avars.erase(kSimFuelWeightPerGallon);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void Fss727Test::fuelCapacityWaitsForTheThreeTankCapacities()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    GiveTanks(gateway);
    gateway.avars.erase(kRightTankCapacity);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void Fss727Test::fuelCapacitySumsTheThreeTanksInKg()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    GiveTanks(gateway);

    QVERIFY(std::abs(aircraft.GetFuelCapacityKg() - kFuelCapacityKg) < kCapacityToleranceKg);
}

void Fss727Test::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 5000.0);
}

void Fss727Test::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kSimEmptyWeight] = 42306.0;

    QCOMPARE(aircraft.GetEmptyZfwKg(), 42306.0);
}

void Fss727Test::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kSimEmptyWeight] = 42306.0;
    gateway.avars[kSimTotalWeight] = 60000.0;
    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 55000.0);
}

void Fss727Test::currentZfwHoldsAtZeroUntilEmptyWeightArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.avars[kSimTotalWeight] = 60000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 0.0);
}

void Fss727Test::registersThePedestalPhoneForFastRefresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QCOMPARE(gateway.fastRefreshNames.size(), std::size_t{1});
    QCOMPARE(gateway.fastRefreshNames.front(), std::string{kPhone});
}

void Fss727Test::aPhoneTouchFiresOnceFromItsFirstThird()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.lvarSpans[kPhone] = LVarSpan{0.0, kPhoneFirstThird, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvarSpans[kPhone] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
}

void Fss727Test::aHeldPhoneFiresOnceAcrossThreeTicks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.lvarSpans[kPhone] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvarSpans[kPhone] = LVarSpan{1.0, 1.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvarSpans[kPhone] = LVarSpan{1.0, 1.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
}

void Fss727Test::readsThePlanFromTheClientOfp()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(!aircraft.RequiresEfbFlightPlan());
    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.flightPlanStatus = FlightPlanStatus::Ready;
    status.plannedFuelKg = 9000.0;
    status.plannedZfwKg = 61000.0;
    status.plannedPassengers = 2;

    QVERIFY(aircraft.IsFlightPlanLoaded());
    QCOMPARE(aircraft.GetPlannedFuelKg(), 9000.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 61000.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 2);
}

void Fss727Test::loadsThroughTheClientAndBoardsByGsxStairs()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Client);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Client);
    QVERIFY(aircraft.SupportsStairsOrJetways());
    QVERIFY(!aircraft.CarriesItsOwnStairs());
    QVERIFY(!aircraft.CompletesPushbackViaInterruptMenu());
}

void Fss727Test::showsWeightsInPounds()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(aircraft.GetNativeWeightUnit() == WeightUnit::Lb);
}

void Fss727Test::everyVariantIsAFreighter()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    for (const char* variantName : kVariants)
    {
        const Fss727 aircraft(&gateway, &status, variantName);

        QVERIFY2(aircraft.IsCargoVariant(), variantName);
    }
}

void Fss727Test::logsTheProfileNameItWasBuiltWith()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const LogCapture log;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200ReFreighter);

    QVERIFY(LogCapture::Contains("Profile loaded: FSS Boeing 727-200RE Freighter"));
}

void Fss727Test::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllEnginesStopped(gateway);
    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(aircraft.IsReadyToPush());

    gateway.lvars[kAcPowerAvailable] = 0.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.lvars[kAcPowerAvailable] = 1.0;
    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());
}

void Fss727Test::readyToDeboardFollowsSafetyState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AllEnginesStopped(gateway);
    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsReadyToDeboard());

    gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());
}

void Fss727Test::automodeRuleNeverHoldsThePhase()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    AircraftRule* const rule = FindRule(aircraft, kAutomodeRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void Fss727Test::clearsTheFourOwnGroundEquipmentObjectsOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

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
}

void Fss727Test::placesAndRemovesTheChocksByTheEfbLVar()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(aircraft.SupportsChocksControl());

    QVERIFY(aircraft.SetChocks(true));
    QCOMPARE(gateway.Written(kChocks), 1.0);

    QVERIFY(aircraft.SetChocks(false));
    QCOMPARE(gateway.Written(kChocks), 0.0);
    QCOMPARE(gateway.WriteCount(kChocks), 2);
}

void Fss727Test::groundPowerStatusFollowsTheAircraftOwnGpu()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);

    gateway.lvars[kGpuAvailable] = 0.0;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuAvailable] = 1.0;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);
}

void Fss727Test::raisesItsOwnGpuOnceWhenTheGsxUnitConnectsAndStowsItWhenItLeaves()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    gsx.gpuStatus = GroundPowerStatus::Disconnected;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.WriteCount(kGpuAvailable), 0);

    gsx.gpuStatus = GroundPowerStatus::Connected;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.WriteCount(kGpuAvailable), 1);
    QCOMPARE(gateway.Written(kGpuAvailable), 1.0);
    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);

    gsx.gpuStatus = GroundPowerStatus::Disconnected;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.WriteCount(kGpuAvailable), 2);
    QCOMPARE(gateway.Written(kGpuAvailable), 0.0);

    QCOMPARE(gateway.WriteCount(kExtPowerSwitch), 0);
}

void Fss727Test::neverStowsAGroundPowerUnitItDidNotRaise()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    gsx.gpuStatus = GroundPowerStatus::Disconnected;
    gateway.lvars[kGpuAvailable] = 1.0;
    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kGpuAvailable), 0);
    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);
}

void Fss727Test::waitsForTheGsxUnitToBeKnownBeforeTouchingItsOwnGpu()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    gsx.gpuStatus = GroundPowerStatus::Unknown;
    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kGpuAvailable), 0);
    QCOMPARE(gateway.WriteCount(kExtPowerSwitch), 0);
}

void Fss727Test::leavesTheGroundPowerUnitCallToTheGsxMenu()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    QVERIFY(!aircraft.SupportsGroundPowerControl());
}

void Fss727Test::theFrontEntryFollowsTheGsxStairs()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.lvars[kCouatlStarted] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.AVarWriteCount(kEntryDoorGoal), 0);

    gateway.lvars[kFrontStairsState] = kStairsDocked;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.AVarWriteCount(kEntryDoorGoal), 1);
    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 1.0);

    gateway.lvars[kFrontStairsState] = kVehicleGone;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.AVarWriteCount(kEntryDoorGoal), 2);
    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 0.0);
}

void Fss727Test::theFrontEntryFollowsTheJetway()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.AVarWriteCount(kEntryDoorGoal), 1);
    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 1.0);
}

void Fss727Test::theFrontEntryStaysClosedOnceHeldForDeparture()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 1.0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 0.0);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.AVarWriteCount(kEntryDoorGoal), 2);
}

void Fss727Test::neverCommandsTheAftAirstair()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    QVERIFY(!aircraft.CarriesItsOwnStairs());

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kFrontStairsState] = kStairsDocked;
    gsx.gpuStatus = GroundPowerStatus::Connected;
    AllDoorPointsAt(gateway, 1.0);

    aircraft.ClearOwnGroundEquipment();
    static_cast<void>(aircraft.SetChocks(true));
    TickTimes(aircraft, gateway, kTwentyTicks);
    aircraft.HoldDoorsClosed(true);
    aircraft.CloseAllDoors();
    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount(kAftStairLever), 0);
    QCOMPARE(gateway.AVarWriteCount(kAftStairGoal), 0);
}

void Fss727Test::closingEveryDoorSendsTheThreeGoalsAndTheCargoPanel()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    MainDeckOpen(gateway);
    TickAircraft(aircraft, gateway);

    aircraft.CloseAllDoors();

    QCOMPARE(gateway.WrittenAVar(kEntryDoorGoal), 0.0);
    QCOMPARE(gateway.WrittenAVar(kForwardHoldGoal), 0.0);
    QCOMPARE(gateway.WrittenAVar(kAftHoldGoal), 0.0);
    QCOMPARE(gateway.AVarWriteCount(kMainDeckGoal), 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kPanelCover), 1.0);
    QCOMPARE(gateway.Written(kPanelMaster), 1.0);
    QCOMPARE(gateway.Written(kPanelDoorSwitch), 0.0);
    QCOMPARE(gateway.AVarWriteCount(kMainDeckGoal), 0);
}

void Fss727Test::keepsThePanelMasterOnUntilTheMainDeckReadsClosed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    MainDeckOpen(gateway);
    TickAircraft(aircraft, gateway);
    aircraft.CloseAllDoors();
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kPanelMaster), 1.0);

    gateway.avars[kMainDeckDoorPoint] = 0.4;
    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.Written(kPanelMaster), 1.0);
    QCOMPARE(gateway.WriteCount(kPanelMaster), 1);

    gateway.avars[kMainDeckDoorPoint] = kMeasuredClosedSettle;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kPanelMaster), 0.0);
    QCOMPARE(gateway.WriteCount(kPanelMaster), 2);
    QCOMPARE(gateway.WriteCount(kPanelDoorSwitch), 1);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount(kPanelMaster), 2);
}

void Fss727Test::neverTurnsTheCargoDoorSwitchOffWhileTheGsxIsLoading()
{
    for (const GsxStateStatus underway : {GsxStateStatus::Requested, GsxStateStatus::Active})
    {
        for (const bool boarding : {true, false})
        {
            FakeVariableGateway gateway;
            AutomationStatus status;
            FakeGsxService gsx;
            Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

            MainDeckOpen(gateway);
            (boarding ? gsx.boardingState : gsx.deboardingState) = underway;
            TickAircraft(aircraft, gateway);

            aircraft.CloseAllDoors();
            TickTimes(aircraft, gateway, kTwentyTicks);

            QVERIFY(gateway.WriteCount(kPanelDoorSwitch) == 0);
            QVERIFY(gateway.WriteCount(kPanelMaster) == 0);
            QVERIFY(gateway.WriteCount(kPanelCover) == 0);
        }
    }
}

void Fss727Test::closesTheMainDeckOnceTheGsxIsDoneWithTheCargoDoors()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    MainDeckOpen(gateway);
    gsx.deboardingState = GsxStateStatus::Requested;
    TickAircraft(aircraft, gateway);
    aircraft.CloseAllDoors();
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.WriteCount(kPanelDoorSwitch), 0);

    gsx.deboardingState = GsxStateStatus::Active;
    TickTimes(aircraft, gateway, kThreeTicks);

    QCOMPARE(gateway.WriteCount(kPanelDoorSwitch), 0);

    gsx.deboardingState = GsxStateStatus::Completed;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kPanelDoorSwitch), 1);
    QCOMPARE(gateway.Written(kPanelDoorSwitch), 0.0);
}

void Fss727Test::leavesTheCargoPanelAloneWhenTheMainDeckIsAlreadyClosed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FakeGsxService gsx;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, &gsx);

    AllDoorPointsAt(gateway, kMeasuredClosedSettle);
    TickAircraft(aircraft, gateway);

    aircraft.CloseAllDoors();
    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount(kPanelCover), 0);
    QCOMPARE(gateway.WriteCount(kPanelMaster), 0);
    QCOMPARE(gateway.WriteCount(kPanelDoorSwitch), 0);
}

void Fss727Test::observingEvaluatingAndReadingWriteNoVariable()
{
    for (const char* variantName : kVariants)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        Fss727 aircraft(&gateway, &status, variantName);

        QVERIFY(!aircraft.Rules().empty());

        AllDoorPointsAt(gateway, kPointHalfway);
        AllEnginesStopped(gateway);
        GiveTanks(gateway);
        gateway.lvars[kAcPowerAvailable] = 1.0;
        gateway.lvars[kParkBrakeLever] = 1.0;
        gateway.lvars[kChocks] = 1.0;
        gateway.avars[kSimEmptyWeight] = 42306.0;
        gateway.lvarSpans[kPhone] = LVarSpan{0.0, 1.0, true};
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
        static_cast<void>(aircraft.GetDoorStatus());
        static_cast<void>(aircraft.IsFlightPlanLoaded());
        static_cast<void>(aircraft.GetCurrentFuelKg());
        static_cast<void>(aircraft.GetCurrentZfwKg());
        static_cast<void>(aircraft.GetFuelCapacityKg());
        static_cast<void>(aircraft.ConsumeSmartSwitch());

        aircraft.OnLoadingStarted();
        aircraft.SetCurrentFuelKg(9000.0);
        aircraft.SetCurrentZfwKg(61000.0);
        aircraft.HoldDoorsClosed(true);

        QCOMPARE(gateway.setLVarCalls, 0);
        QCOMPARE(gateway.setAVarCalls, 0);
    }
}

void Fss727Test::writesOnlyTheAutomodeKeyOnceAcrossFiftyTicks()
{
    for (const char* variantName : kVariants)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        Fss727 aircraft(&gateway, &status, variantName);

        TickAircraft(aircraft, gateway);

        QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 1);
        QCOMPARE(gateway.Written(kAutomodeDisabled), 1.0);

        TickTimes(aircraft, gateway, kFiftyTicks - 1);

        QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 1);
        for (const char* key : kWasmKeysBornAtZero)
        {
            QCOMPARE(gateway.WriteCount(key), 0);
        }
        QCOMPARE(gateway.setLVarCalls, 1);
        QCOMPARE(gateway.setAVarCalls, 0);
    }
}

void Fss727Test::rewritesTheAutomodeKeyWhenTheEfbTurnsItBackOn()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    TickTimes(aircraft, gateway, kTicksWithoutEcho + 2);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 1);

    gateway.lvars[kAutomodeDisabled] = 0.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 2);
    QCOMPARE(gateway.Written(kAutomodeDisabled), 1.0);
}

void Fss727Test::waitsForTheAutomodeWriteToComeBackBeforeRetrying()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kAutomodeDisabled] = 0.0;
    TickTimes(aircraft, gateway, kTicksWithoutEcho);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 1);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 2);
}

QTEST_APPLESS_MAIN(Fss727Test)

#include "tst_fss_727.moc"
