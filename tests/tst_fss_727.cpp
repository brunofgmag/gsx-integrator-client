#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "AircraftTicks.h"
#include "TestDoubles.h"
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
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";

    constexpr auto kEntryDoorPoint = "INTERACTIVE POINT OPEN:0";
    constexpr auto kMainDeckDoorPoint = "INTERACTIVE POINT OPEN:1";
    constexpr auto kForwardHoldPoint = "INTERACTIVE POINT OPEN:2";
    constexpr std::array kDoorPoints = {
        kEntryDoorPoint, kMainDeckDoorPoint, kForwardHoldPoint,
        "INTERACTIVE POINT OPEN:3", "INTERACTIVE POINT OPEN:4"
    };

    constexpr auto kPassengerFwdGalleyPoint = "INTERACTIVE POINT OPEN:1";
    constexpr auto kPassengerAftHoldPoint = "INTERACTIVE POINT OPEN:5";
    constexpr auto kPassengerAftStairPoint = "INTERACTIVE POINT OPEN:6";
    constexpr std::array kPassengerDoorPoints = {
        "INTERACTIVE POINT OPEN:0", kPassengerFwdGalleyPoint, "INTERACTIVE POINT OPEN:2",
        "INTERACTIVE POINT OPEN:3", "INTERACTIVE POINT OPEN:4", kPassengerAftHoldPoint, kPassengerAftStairPoint
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

    struct Variant
    {
        const char* name;
        bool cargo;
    };

    constexpr std::array kVariants = {
        Variant{Fss727::kName200F, true},
        Variant{Fss727::kName200ReFreighter, true},
        Variant{Fss727::kName200RePassenger, false}
    };

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

    void AllPassengerDoorPointsAt(FakeVariableGateway& gateway, const double position)
    {
        for (const char* point : kPassengerDoorPoints)
        {
            gateway.avars[point] = position;
        }
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
    static void passengerReadsItsSevenDoorPoints();
    static void passengerGalleyDoorGetsThePaxDeadline();
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
    static void reportsTheCargoVariantItWasBuiltWith();
    static void logsTheProfileNameItWasBuiltWith();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(aircraft.IsPowered());
}

void Fss727Test::enginesAssumedRunningUntilTheThreeCombustionsArrive()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::doorStatusAllClosedWithEveryPointAtZero()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    AllDoorPointsAt(gateway, 0.0);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);
}

void Fss727Test::doorStatusOpenWithAPointFullyOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kForwardHoldPoint] = 1.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::doorStatusUnknownWhileAPointTravels()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    AllDoorPointsAt(gateway, 0.0);
    gateway.avars[kEntryDoorPoint] = kPointHalfway;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void Fss727Test::travelThatOutlastsTheDeadlineReadsOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    AllDoorPointsAt(gateway, kMeasuredClosedSettle);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.avars[kForwardHoldPoint] = kMeasuredOpenSettle;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::passengerReadsItsSevenDoorPoints()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200RePassenger, false);

    AllPassengerDoorPointsAt(gateway, 0.0);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);

    gateway.avars[kPassengerAftHoldPoint] = 1.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);

    gateway.avars[kPassengerAftHoldPoint] = 0.0;
    gateway.avars[kPassengerAftStairPoint] = 1.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::passengerGalleyDoorGetsThePaxDeadline()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200RePassenger, false);

    AllPassengerDoorPointsAt(gateway, 0.0);
    gateway.avars[kPassengerFwdGalleyPoint] = kPointHalfway;
    TickTimes(aircraft, gateway, kPaxDoorDeadlineTicks - 1);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void Fss727Test::fuelCapacityWaitsForTheWeightPerGallon()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    GiveTanks(gateway);
    gateway.avars.erase(kSimFuelWeightPerGallon);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void Fss727Test::fuelCapacityWaitsForTheThreeTankCapacities()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    GiveTanks(gateway);
    gateway.avars.erase(kRightTankCapacity);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void Fss727Test::fuelCapacitySumsTheThreeTanksInKg()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    GiveTanks(gateway);

    QVERIFY(std::abs(aircraft.GetFuelCapacityKg() - kFuelCapacityKg) < kCapacityToleranceKg);
}

void Fss727Test::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 5000.0);
}

void Fss727Test::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    gateway.avars[kSimEmptyWeight] = 42306.0;

    QCOMPARE(aircraft.GetEmptyZfwKg(), 42306.0);
}

void Fss727Test::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    gateway.avars[kSimEmptyWeight] = 42306.0;
    gateway.avars[kSimTotalWeight] = 60000.0;
    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 55000.0);
}

void Fss727Test::currentZfwHoldsAtZeroUntilEmptyWeightArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    gateway.avars[kSimTotalWeight] = 60000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 0.0);
}

void Fss727Test::registersThePedestalPhoneForFastRefresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    QCOMPARE(gateway.fastRefreshNames.size(), std::size_t{1});
    QCOMPARE(gateway.fastRefreshNames.front(), std::string{kPhone});
}

void Fss727Test::aPhoneTouchFiresOnceFromItsFirstThird()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    QVERIFY(aircraft.GetNativeWeightUnit() == WeightUnit::Lb);
}

void Fss727Test::reportsTheCargoVariantItWasBuiltWith()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 freighter(&gateway, &status, Fss727::kName200ReFreighter, true);
    const Fss727 passenger(&gateway, &status, Fss727::kName200RePassenger, false);

    QVERIFY(freighter.IsCargoVariant());
    QVERIFY(!passenger.IsCargoVariant());
}

void Fss727Test::logsTheProfileNameItWasBuiltWith()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const LogCapture log;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200ReFreighter, true);

    QVERIFY(LogCapture::Contains("Profile loaded: FSS Boeing 727-200RE Freighter"));
}

void Fss727Test::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    const Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    AircraftRule* const rule = FindRule(aircraft, kAutomodeRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void Fss727Test::observingEvaluatingAndReadingWriteNoVariable()
{
    for (const Variant& variant : kVariants)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        Fss727 aircraft(&gateway, &status, variant.name, variant.cargo);

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
        QVERIFY(!aircraft.SetChocks(true));
        aircraft.SetGroundPower(true);
        aircraft.CloseAllDoors();
        aircraft.HoldDoorsClosed(true);
        aircraft.ClearOwnGroundEquipment();

        QCOMPARE(gateway.setLVarCalls, 0);
        QCOMPARE(gateway.setAVarCalls, 0);
    }
}

void Fss727Test::writesOnlyTheAutomodeKeyOnceAcrossFiftyTicks()
{
    for (const Variant& variant : kVariants)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        Fss727 aircraft(&gateway, &status, variant.name, variant.cargo);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

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
    Fss727 aircraft(&gateway, &status, Fss727::kName200F, true);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kAutomodeDisabled] = 0.0;
    TickTimes(aircraft, gateway, kTicksWithoutEcho);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 1);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAutomodeDisabled), 2);
}

QTEST_APPLESS_MAIN(Fss727Test)

#include "tst_fss_727.moc"
