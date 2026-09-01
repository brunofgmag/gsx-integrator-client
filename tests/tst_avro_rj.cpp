#include <QtTest/QTest>

#include <array>
#include <string>
#include "AircraftTicks.h"
#include "TestDoubles.h"
#include "../src/infrastructure/aircraft/avrorj/AvroRj.h"
#include "doubles/FakeVariableWriter.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";

    constexpr auto kLeftMainQuantity = "FUEL TANK LEFT MAIN QUANTITY";
    constexpr auto kRightMainQuantity = "FUEL TANK RIGHT MAIN QUANTITY";
    constexpr auto kCenterQuantity = "FUEL TANK CENTER QUANTITY";
    constexpr auto kLeftAuxQuantity = "FUEL TANK LEFT AUX QUANTITY";
    constexpr auto kRightAuxQuantity = "FUEL TANK RIGHT AUX QUANTITY";

    constexpr auto kLeftMainCapacity = "FUEL TANK LEFT MAIN CAPACITY";
    constexpr auto kRightMainCapacity = "FUEL TANK RIGHT MAIN CAPACITY";
    constexpr auto kCenterCapacity = "FUEL TANK CENTER CAPACITY";
    constexpr auto kLeftAuxCapacity = "FUEL TANK LEFT AUX CAPACITY";
    constexpr auto kRightAuxCapacity = "FUEL TANK RIGHT AUX CAPACITY";

    constexpr auto kLeftAuxFitted = "OVHD_FUEL_L_aux_vis";
    constexpr auto kRightAuxFitted = "OVHD_FUEL_R_aux_vis";

    constexpr auto kPlannedBlockFuel = "146_SimBrief_Block_Fuel";
    constexpr auto kPlannedZfw = "146_SimBrief_ZFW";
    constexpr auto kPlannedPassengers = "146_SimBrief_PaxQt";

    constexpr auto kAcBus2 = "JF_RJ_ELEC_AC_2";
    constexpr auto kAcBusEss = "JF_RJ_ELEC_AC_ess";
    constexpr auto kChocks = "EXT_Chocks";
    constexpr auto kSmartSwitch = "PED_FWD_L_Audio_RT";

    constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    constexpr auto kJetway = "FSDT_GSX_JETWAY";
    constexpr auto kStairsFrontState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSFRONT_STATE";
    constexpr auto kStairsRearState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSREAR_STATE";
    constexpr auto kBoardingState = "FSDT_GSX_BOARDING_STATE";
    constexpr double kBoardingActive = 5.0;
    constexpr auto kParkBrakeAnnunciator = "C_ANNUNS_ParkBrake_il";
    constexpr auto kModuleFuelMirror = "146_FuelWeight_KG";
    constexpr auto kAftPaxDoor = "EXT_Door_pax_2L";

    constexpr double kStairsFinalPosition = 3.0;
    constexpr double kStairsWaitingForDoor = 6.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayUnavailable = 2.0;

    constexpr auto kStairArmClickspot = "VC_Stairs_clickspot_LC";
    constexpr auto kGsxStairs = "FSDT_GSX_STAIRS";
    constexpr double kGsxStairsCallable = 1.0;
    constexpr auto kExtGpu = "EXT_GPU";
    constexpr auto kStairExtendSwitch = "CAB_CTRLS_Fwd_StairRetract";
    constexpr auto kStairAccumPressure = "Stairs_accum_press";
    constexpr double kStairPressureFull = 5000.0;
    constexpr double kStairPressureDepleted = 100.0;

    constexpr auto kEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kEng3Combustion = "ENG COMBUSTION:3";
    constexpr auto kEng4Combustion = "ENG COMBUSTION:4";

    constexpr std::array kDoorLVars = {
        "EXT_Door_pax_1L", "EXT_Door_pax_1R", "EXT_Door_pax_2L", "EXT_Door_pax_2R",
        "EXT_Door_cargo_fwd", "EXT_Door_cargo_aft", "EXT_Door_cargo_fuselage"
    };

    constexpr double kKgPerGallon = 3.0;
    constexpr double kMainCapacityGallons = 1000.0;
    constexpr double kCenterCapacityGallons = 500.0;
    constexpr double kAuxCapacityGallons = 250.0;

    void GiveTanks(FakeVariableGateway& gateway)
    {
        gateway.avars[kSimFuelWeightPerGallon] = kKgPerGallon;
        gateway.avars[kLeftMainCapacity] = kMainCapacityGallons;
        gateway.avars[kRightMainCapacity] = kMainCapacityGallons;
        gateway.avars[kCenterCapacity] = kCenterCapacityGallons;
        gateway.avars[kLeftAuxCapacity] = kAuxCapacityGallons;
        gateway.avars[kRightAuxCapacity] = kAuxCapacityGallons;
        gateway.lvars[kLeftAuxFitted] = 1.0;
        gateway.lvars[kRightAuxFitted] = 1.0;
    }

    void AllEnginesStopped(FakeVariableGateway& gateway)
    {
        gateway.avars[kEng1Combustion] = 0.0;
        gateway.avars[kEng2Combustion] = 0.0;
        gateway.avars[kEng3Combustion] = 0.0;
        gateway.avars[kEng4Combustion] = 0.0;
    }

    void AllDoorsClosed(FakeVariableGateway& gateway)
    {
        for (const char* doorLVar : kDoorLVars)
        {
            gateway.lvars[doorLVar] = 0.0;
        }
    }
}

class AvroRjTest final : public QObject
{
    Q_OBJECT

private slots:
    static void evaluatingTheAirstairRuleWritesNoVariable();
    static void evaluatingTheDoorRuleWritesNoVariable();
    static void evaluatingTheModuleLivenessRuleWritesNoVariable();
    static void reportsCargoVariant();
    static void reportsLoadMethods();
    static void requiresTheEfbFlightPlan();
    static void flightPlanLoadedWhenTheImportLandsInTheLVars();
    static void flightPlanUnloadsWhenAFlightReloadZeroesTheLVars();
    static void freighterPlanNeedsNoPassengers();
    static void plannedValuesComeFromTheAircraftLVars();
    static void emptyZfwReadsSimEmptyWeight();
    static void readsCurrentFuelFromSim();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwHoldsAtZeroUntilEmptyWeightArrives();
    static void zfwSetterWritesNothing();
    static void fuelSetterWaitsForTheFuelDensity();
    static void fuelSetterWaitsForTheTankCapacities();
    static void fuelSetterFillsTheMainsFirst();
    static void fuelSetterOverflowsIntoCentreThenAux();
    static void fuelSetterClampsAtTotalCapacity();
    static void fuelSetterWritesOnlyWhenTheTargetChanges();
    static void registersSmartSwitchForFastRefresh();
    static void smartSwitchFiresOnce();
    static void smartSwitchIgnoresTheRadioSide();
    static void doorStatusUnknownUntilTheDoorsArrive();
    static void doorStatusOpenWhileTheForwardDoorTrails();
    static void doorStatusAllClosedWhenEveryDoorReadsZero();
    static void powerFollowsTheAcBuses();
    static void engineAssumedRunningUntilCombustionArrives();
    static void engineRunningDetectsAnyOfFourEngines();
    static void heldInPlaceAcceptsTheBrakeOrTheChocks();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
    static void doorsStayUntouchedUntilTheCouatlStarts();
    static void doorsStayUntouchedWithNoEquipmentAtTheAircraft();
    static void aftDoorStaysClosedEvenWhenGsxParksAStairAtIt();
    static void aftDoorIsClosedAgainEveryTimeSomethingOpensIt();
    static void aftDoorIsNotWrittenWhileItReadsClosed();
    static void frontDoorOpensWithADockedJetway();
    static void departureHoldShutsTheFrontDoor();
    static void airstairStaysStowedUntilTheTurnaroundAsks();
    static void airstairExtendsWhenTheTurnaroundAsksWithPressure();
    static void airstairWaitsWithoutAccumulatorPressure();
    static void airstairIsNotArmedUnderAJetway();
    static void airstairStaysStowedWhileTheJetwayIsStillDriving();
    static void airstairIsNotArmedWhenGsxServesTheFrontDoor();
    static void airstairYieldsToAStairVehicleOnTheWay();
    static void airstairRetractsAndTheDoorClosesWhenTheRequestIsWithdrawn();
    static void airstairStaysOutWhenPressureIsGoneAtDeparture();
    static void airstairAdoptsAnExtensionMadeOnTheEfb();
    static void frontDoorWaitsForThePhysicallyStowedStair();
    static void reportsTheAirstairExtendedOnlyAfterItStopsMoving();
    static void airstairIsNotCommandedWhileItIsStillMoving();
    static void groundPowerIsLeftToGsx();
    static void chocksControlDrivesTheAircraftChocks();
    static void parkingBrakeReadsTheAnnunciatorAndNotTheSimVar();
    static void fuelCapacitySumsTheTanksInKg();
    static void fuelCapacityWaitsForTheFuelDensity();
    static void fuelCapacityWaitsForTheAuxTankFlags();
    static void fuelCapacityLeavesOutAnAuxTankTheAircraftDoesNotHave();
    static void fuelSetterSkipsAnAuxTankTheAircraftDoesNotHave();
    static void moduleLivenessTripsWhenTheFuelMirrorFreezes();
    static void moduleLivenessHoldsWhileTheMirrorFollows();
    static void moduleLivenessIgnoresDivergenceWhileFuelIsStill();
};

void AvroRjTest::reportsCargoVariant()
{
    FakeVariableGateway gateway;
    const AvroRj passenger(&gateway, false);
    const AvroRj freighter(&gateway, true);

    QVERIFY(!passenger.IsCargoVariant());
    QVERIFY(freighter.IsCargoVariant());
}

void AvroRjTest::reportsLoadMethods()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Client);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Self);
    QVERIFY(!aircraft.CompletesPushbackViaInterruptMenu());
    QVERIFY(!aircraft.SupportsStairsOrJetways());
    QVERIFY(aircraft.CarriesItsOwnStairs());
}

void AvroRjTest::requiresTheEfbFlightPlan()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(aircraft.RequiresEfbFlightPlan());
}

void AvroRjTest::flightPlanLoadedWhenTheImportLandsInTheLVars()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    gateway.lvars[kPlannedBlockFuel] = 5776.0;

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    gateway.lvars[kPlannedZfw] = 34999.0;

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    gateway.lvars[kPlannedPassengers] = 55.0;

    QVERIFY(aircraft.IsFlightPlanLoaded());
}

void AvroRjTest::freighterPlanNeedsNoPassengers()
{
    FakeVariableGateway gateway;
    const AvroRj freighter(&gateway, true);

    gateway.lvars[kPlannedBlockFuel] = 5776.0;
    gateway.lvars[kPlannedZfw] = 34999.0;

    QVERIFY(freighter.IsFlightPlanLoaded());
}

void AvroRjTest::flightPlanUnloadsWhenAFlightReloadZeroesTheLVars()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.lvars[kPlannedBlockFuel] = 5776.0;
    gateway.lvars[kPlannedZfw] = 34999.0;
    gateway.lvars[kPlannedPassengers] = 55.0;

    QVERIFY(aircraft.IsFlightPlanLoaded());

    gateway.lvars[kPlannedBlockFuel] = 0.0;
    gateway.lvars[kPlannedZfw] = 0.0;

    QVERIFY(!aircraft.IsFlightPlanLoaded());
}

void AvroRjTest::plannedValuesComeFromTheAircraftLVars()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.lvars[kPlannedBlockFuel] = 5776.0;
    gateway.lvars[kPlannedZfw] = 34999.0;
    gateway.lvars[kPlannedPassengers] = 101.0;

    QCOMPARE(aircraft.GetPlannedFuelKg(), 5776.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 34999.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 101);
}

void AvroRjTest::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimEmptyWeight] = 22174.0;

    QCOMPARE(aircraft.GetEmptyZfwKg(), 22174.0);
}

void AvroRjTest::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimFuelTotalKg] = 5776.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 5776.0);
}

void AvroRjTest::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimEmptyWeight] = 22174.0;
    gateway.avars[kSimTotalWeight] = 33000.0;
    gateway.avars[kSimFuelTotalKg] = 5776.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 27224.0);
}

void AvroRjTest::currentZfwHoldsAtZeroUntilEmptyWeightArrives()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimTotalWeight] = 33000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 0.0);
}

void AvroRjTest::zfwSetterWritesNothing()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.avars[kSimEmptyWeight] = 22174.0;
    const int writesBefore = gateway.setAVarCalls;

    aircraft.SetCurrentZfwKg(30000.0);

    QCOMPARE(gateway.setAVarCalls, writesBefore);
    QCOMPARE(gateway.setLVarCalls, 0);
}

void AvroRjTest::fuelSetterWaitsForTheFuelDensity()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.avars[kLeftMainCapacity] = kMainCapacityGallons;
    gateway.avars[kRightMainCapacity] = kMainCapacityGallons;

    aircraft.SetCurrentFuelKg(3000.0);

    QCOMPARE(gateway.setAVarCalls, 0);
}

void AvroRjTest::fuelSetterWaitsForTheTankCapacities()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.avars[kSimFuelWeightPerGallon] = kKgPerGallon;

    aircraft.SetCurrentFuelKg(3000.0);

    QCOMPARE(gateway.setAVarCalls, 0);
}

void AvroRjTest::fuelSetterFillsTheMainsFirst()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);

    aircraft.SetCurrentFuelKg(3000.0);

    QCOMPARE(gateway.avars[kLeftMainQuantity], 500.0);
    QCOMPARE(gateway.avars[kRightMainQuantity], 500.0);
    QCOMPARE(gateway.avars[kCenterQuantity], 0.0);
    QCOMPARE(gateway.avars[kLeftAuxQuantity], 0.0);
    QCOMPARE(gateway.avars[kRightAuxQuantity], 0.0);
}

void AvroRjTest::fuelSetterOverflowsIntoCentreThenAux()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);

    aircraft.SetCurrentFuelKg(7500.0);

    QCOMPARE(gateway.avars[kLeftMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kRightMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kCenterQuantity], kCenterCapacityGallons);
    QCOMPARE(gateway.avars[kLeftAuxQuantity], 0.0);
    QCOMPARE(gateway.avars[kRightAuxQuantity], 0.0);
}

void AvroRjTest::fuelSetterClampsAtTotalCapacity()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);

    aircraft.SetCurrentFuelKg(999999.0);

    QCOMPARE(gateway.avars[kLeftMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kRightMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kCenterQuantity], kCenterCapacityGallons);
    QCOMPARE(gateway.avars[kLeftAuxQuantity], kAuxCapacityGallons);
    QCOMPARE(gateway.avars[kRightAuxQuantity], kAuxCapacityGallons);
}

void AvroRjTest::fuelSetterWritesOnlyWhenTheTargetChanges()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);

    aircraft.SetCurrentFuelKg(3000.0);
    const int writesAfterFirst = gateway.setAVarCalls;

    aircraft.SetCurrentFuelKg(3000.0);

    QCOMPARE(gateway.setAVarCalls, writesAfterFirst);

    aircraft.SetCurrentFuelKg(3300.0);

    QVERIFY(gateway.setAVarCalls > writesAfterFirst);
}

void AvroRjTest::registersSmartSwitchForFastRefresh()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QCOMPARE(gateway.fastRefreshNames.size(), std::size_t{1});
    QCOMPARE(gateway.fastRefreshNames.front(), std::string{kSmartSwitch});
}

void AvroRjTest::smartSwitchFiresOnce()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvarSpans[kSmartSwitch] = LVarSpan{1.0, 2.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 1.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());
}

void AvroRjTest::smartSwitchIgnoresTheRadioSide()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvarSpans[kSmartSwitch] = LVarSpan{0.0, 1.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
}

void AvroRjTest::doorStatusUnknownUntilTheDoorsArrive()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::Unknown);
}

void AvroRjTest::doorStatusOpenWhileTheForwardDoorTrails()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    AllDoorsClosed(gateway);
    gateway.lvars["EXT_Door_pax_1L"] = 1.0;

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AnyOpen);
}

void AvroRjTest::doorStatusAllClosedWhenEveryDoorReadsZero()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    AllDoorsClosed(gateway);

    QVERIFY(aircraft.GetDoorStatus() == DoorStatus::AllClosed);
}

void AvroRjTest::powerFollowsTheAcBuses()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kAcBusEss] = 1.0;

    QVERIFY(aircraft.IsPowered());

    gateway.lvars[kAcBusEss] = 0.0;
    gateway.lvars[kAcBus2] = 1.0;

    QVERIFY(aircraft.IsPowered());
}

void AvroRjTest::engineAssumedRunningUntilCombustionArrives()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(aircraft.IsEngineRunning());
}

void AvroRjTest::engineRunningDetectsAnyOfFourEngines()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    AllEnginesStopped(gateway);

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kEng4Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void AvroRjTest::heldInPlaceAcceptsTheBrakeOrTheChocks()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kChocks] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kChocks] = 0.0;
    gateway.lvars[kParkBrakeAnnunciator] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
    QVERIFY(aircraft.IsParkingBrakeSet());
}

void AvroRjTest::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kAcBus2] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(aircraft.IsReadyToPush());

    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());
}

void AvroRjTest::readyToDeboardFollowsSafetyState()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kParkBrakeAnnunciator] = 1.0;

    QVERIFY(aircraft.IsReadyToDeboard());

    gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());
}

void AvroRjTest::doorsStayUntouchedUntilTheCouatlStarts()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kStairsRearState] = kStairsFinalPosition;
    gateway.setLVarCalls = 0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void AvroRjTest::doorsStayUntouchedWithNoEquipmentAtTheAircraft()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.setLVarCalls = 0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void AvroRjTest::aftDoorStaysClosedEvenWhenGsxParksAStairAtIt()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kStairsRearState] = kStairsFinalPosition;
    gateway.lvars[kAftPaxDoor] = 1.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kAftPaxDoor), 0.0);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), -1.0);
}

void AvroRjTest::aftDoorIsClosedAgainEveryTimeSomethingOpensIt()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kAftPaxDoor] = 1.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kAftPaxDoor), 0.0);
    QCOMPARE(gateway.WriteCount(kAftPaxDoor), 1);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAftPaxDoor), 1);

    gateway.lvars[kAftPaxDoor] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kAftPaxDoor), 0.0);
    QCOMPARE(gateway.WriteCount(kAftPaxDoor), 2);
}

void AvroRjTest::aftDoorIsNotWrittenWhileItReadsClosed()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kAftPaxDoor] = 0.0;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAftPaxDoor), 0);
}

void AvroRjTest::frontDoorOpensWithADockedJetway()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
}

void AvroRjTest::departureHoldShutsTheFrontDoor()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 0.0);
}

void AvroRjTest::airstairStaysStowedUntilTheTurnaroundAsks()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), -1.0);
    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairExtendsWhenTheTurnaroundAsksWithPressure()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
    QCOMPARE(gateway.Written(kStairArmClickspot), 1.0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
}

void AvroRjTest::airstairWaitsWithoutAccumulatorPressure()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureDepleted;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairIsNotArmedUnderAJetway()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairStaysStowedWhileTheJetwayIsStillDriving()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = 1.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairIsNotArmedWhenGsxServesTheFrontDoor()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairsFrontState] = kStairsWaitingForDoor;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairYieldsToAStairVehicleOnTheWay()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairsFrontState] = 2.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);
}

void AvroRjTest::airstairRetractsAndTheDoorClosesWhenTheRequestIsWithdrawn()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), 1.0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 0.0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 2);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 0.0);
}

void AvroRjTest::airstairStaysOutWhenPressureIsGoneAtDeparture()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    gateway.lvars[kStairAccumPressure] = kStairPressureDepleted;
    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
}

void AvroRjTest::airstairAdoptsAnExtensionMadeOnTheEfb()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 0.0);
}

void AvroRjTest::frontDoorWaitsForThePhysicallyStowedStair()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    gateway.lvars[kStairExtendSwitch] = 0.0;
    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);

    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 0.0);
}

void AvroRjTest::reportsTheAirstairExtendedOnlyAfterItStopsMoving()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QVERIFY(!aircraft.AreAirstairsSettled());

    gateway.lvars["EXT_Door_stairs_pos"] = 120.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(!aircraft.AreAirstairsSettled());

    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;
    TickAircraft(aircraft, gateway);

    QVERIFY(!aircraft.AreAirstairsSettled());

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.AreAirstairsSettled());
}

void AvroRjTest::airstairIsNotCommandedWhileItIsStillMoving()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);

    gateway.lvars["EXT_Door_stairs_pos"] = 100.0;
    TickAircraft(aircraft, gateway);

    gateway.lvars["EXT_Door_stairs_pos"] = 71.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), -1.0);

    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairArmClickspot), 1.0);
}

void AvroRjTest::groundPowerIsLeftToGsx()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    QVERIFY(!aircraft.SupportsGroundPowerControl());
    QVERIFY(!aircraft.GetGroundPowerStatus().has_value());

    aircraft.SetGroundPower(true);

    QCOMPARE(gateway.Written(kExtGpu), -1.0);
}

void AvroRjTest::chocksControlDrivesTheAircraftChocks()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    QVERIFY(aircraft.SupportsChocksControl());
    QVERIFY(aircraft.SetChocks(true));
    QCOMPARE(gateway.Written(kChocks), 1.0);

    QVERIFY(aircraft.SetChocks(false));
    QCOMPARE(gateway.Written(kChocks), 0.0);
}

void AvroRjTest::parkingBrakeReadsTheAnnunciatorAndNotTheSimVar()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimParkingBrake] = 1.0;
    gateway.lvars[kParkBrakeAnnunciator] = 0.0;

    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kParkBrakeAnnunciator] = 1.0;

    QVERIFY(aircraft.IsParkingBrakeSet());
}

void AvroRjTest::fuelCapacitySumsTheTanksInKg()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);

    QCOMPARE(aircraft.GetFuelCapacityKg(), 9000.0);
}

void AvroRjTest::fuelCapacityWaitsForTheFuelDensity()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kLeftMainCapacity] = kMainCapacityGallons;
    gateway.avars[kRightMainCapacity] = kMainCapacityGallons;

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void AvroRjTest::fuelCapacityWaitsForTheAuxTankFlags()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    gateway.avars[kSimFuelWeightPerGallon] = kKgPerGallon;
    gateway.avars[kLeftMainCapacity] = kMainCapacityGallons;
    gateway.avars[kRightMainCapacity] = kMainCapacityGallons;
    gateway.avars[kCenterCapacity] = kCenterCapacityGallons;
    gateway.avars[kLeftAuxCapacity] = kAuxCapacityGallons;
    gateway.avars[kRightAuxCapacity] = kAuxCapacityGallons;

    QCOMPARE(aircraft.GetFuelCapacityKg(), 0.0);
}

void AvroRjTest::fuelCapacityLeavesOutAnAuxTankTheAircraftDoesNotHave()
{
    FakeVariableGateway gateway;
    const AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);
    gateway.lvars[kLeftAuxFitted] = 0.0;
    gateway.lvars[kRightAuxFitted] = 0.0;

    QCOMPARE(aircraft.GetFuelCapacityKg(), 7500.0);
}

void AvroRjTest::fuelSetterSkipsAnAuxTankTheAircraftDoesNotHave()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    GiveTanks(gateway);
    gateway.lvars[kLeftAuxFitted] = 0.0;
    gateway.lvars[kRightAuxFitted] = 0.0;

    aircraft.SetCurrentFuelKg(9000.0);

    QCOMPARE(gateway.avars[kLeftMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kRightMainQuantity], kMainCapacityGallons);
    QCOMPARE(gateway.avars[kCenterQuantity], kCenterCapacityGallons);
    QVERIFY(!gateway.avars.contains(kLeftAuxQuantity));
    QVERIFY(!gateway.avars.contains(kRightAuxQuantity));
}

void AvroRjTest::moduleLivenessTripsWhenTheFuelMirrorFreezes()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kModuleFuelMirror] = 1000.0;
    gateway.avars[kSimFuelTotalKg] = 1000.0;

    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.IsModuleMirroringFuel());

    for (int tick = 1; tick <= 6; ++tick)
    {
        gateway.avars[kSimFuelTotalKg] = 1000.0 + tick * 100.0;
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(!aircraft.IsModuleMirroringFuel());
}

void AvroRjTest::moduleLivenessHoldsWhileTheMirrorFollows()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kModuleFuelMirror] = 1000.0;
    gateway.avars[kSimFuelTotalKg] = 1000.0;

    TickAircraft(aircraft, gateway);

    for (int tick = 1; tick <= 6; ++tick)
    {
        const double fuel = 1000.0 + tick * 100.0;
        gateway.avars[kSimFuelTotalKg] = fuel;
        gateway.lvars[kModuleFuelMirror] = fuel;
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(aircraft.IsModuleMirroringFuel());
}

void AvroRjTest::moduleLivenessIgnoresDivergenceWhileFuelIsStill()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kModuleFuelMirror] = 0.0;
    gateway.avars[kSimFuelTotalKg] = 6000.0;

    for (int tick = 0; tick < 8; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(aircraft.IsModuleMirroringFuel());
}

void AvroRjTest::evaluatingTheAirstairRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    FakeVariableWriter writer;

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-hold-for-own-airstair");

    QVERIFY(rule != nullptr);

    RuleContext context;
    context.phase = TurnaroundPhase::CallServices;
    context.needs.passengerAccess = true;

    const int writesBefore = gateway.setLVarCalls + gateway.setAVarCalls;

    for (int tick = 0; tick < 5; ++tick)
    {
        const RuleVerdict verdict = rule->Evaluate(context);

        QVERIFY(verdict.holds);
        QVERIFY(verdict.holdTicksAllowed > 0);
    }

    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, writesBefore);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);
}

void AvroRjTest::evaluatingTheDoorRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    FakeVariableWriter writer;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    aircraft.Observe();

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-pax-doors-serve-the-airstair");

    QVERIFY(rule != nullptr);

    const RuleContext context{};
    const int writesBefore = gateway.setLVarCalls + gateway.setAVarCalls;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, writesBefore);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);

    rule->Act(context, writer);

    QCOMPARE(writer.Written("EXT_Door_pax_1L"), 1.0);
}

void AvroRjTest::evaluatingTheModuleLivenessRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    FakeVariableWriter writer;

    gateway.lvars[kModuleFuelMirror] = 0.0;

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-watch-module-fuel-mirror");

    QVERIFY(rule != nullptr);

    const RuleContext context{};

    for (int tick = 0; tick < 8; ++tick)
    {
        gateway.avars[kSimFuelTotalKg] = 6000.0 + tick * 100.0;
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QVERIFY(!aircraft.IsModuleMirroringFuel());
    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, 0);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);
}

QTEST_APPLESS_MAIN(AvroRjTest)

#include "tst_avro_rj.moc"
