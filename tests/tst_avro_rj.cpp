#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
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
    constexpr double kBoardingRequested = 4.0;
    constexpr double kBoardingActive = 5.0;
    constexpr auto kDeboardingState = "FSDT_GSX_DEBOARDING_STATE";
    constexpr auto kLoaderFrontState = "FSDT_GSX_VEHICLE_BAGGAGELOADERFRONT_STATE";
    constexpr auto kLoaderRearState = "FSDT_GSX_VEHICLE_BAGGAGELOADERREAR_STATE";
    constexpr auto kFwdHold = "EXT_Door_cargo_fwd";
    constexpr auto kAftHold = "EXT_Door_cargo_aft";
    constexpr double kLoaderWaitingForDoor = 6.0;
    constexpr double kLoaderFinishing = 10.0;
    constexpr int kHoldHeadStartTicks = 5;
    constexpr auto kParkBrakeAnnunciator = "C_ANNUNS_ParkBrake_il";
    constexpr auto kModuleFuelMirror = "146_FuelWeight_KG";
    constexpr auto kAftPaxDoor = "EXT_Door_pax_2L";

    constexpr double kStairsFinalPosition = 3.0;
    constexpr double kStairsWaitingForDoor = 6.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayUnavailable = 2.0;
    constexpr double kJetwayApproaching = 3.0;

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

    constexpr auto kStairPosition = "EXT_Door_stairs_pos";
    constexpr auto kFwdPaxDoor = "EXT_Door_pax_1L";
    constexpr double kStairStowedPosition = 50.0;
    constexpr double kStairOutPosition = 190.0;

    TurnaroundFacts ResumedAt(const TurnaroundPhase phase)
    {
        TurnaroundFacts facts;
        facts.phase = phase;

        return facts;
    }

    void TickHolds(AvroRj& aircraft, FakeVariableGateway& gateway, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            TickAircraft(aircraft, gateway);
        }
    }

    MemoryBag MemoryOfAnOpenFrontDoor()
    {
        FakeVariableGateway gateway;
        AvroRj dead(&gateway, false);

        gateway.lvars[kCouatlStarted] = 1.0;
        gateway.lvars[kJetway] = kJetwayDocked;

        TickAircraft(dead, gateway);

        return dead.TurnaroundMemory();
    }

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
    static void holdTheAircraftOpenedIsClosedWhileBoardingHasNoLoaderAtIt();
    static void holdTheAircraftOpenedIsClosedWhileDeboardingHasNoLoaderAtIt();
    static void openHoldIsClosedWithNoLoaderAtItEvenOutsideAService();
    static void holdGivesTheAircraftAHeadStartBeforeCommanding();
    static void holdTheAircraftMovesWithinTheHeadStartIsNotWritten();
    static void holdHeadStartRestartsAfterTheHoldAgreedAgain();
    static void holdOpensWhenItsOwnLoaderReachesItAndTheOtherIsLeftAlone();
    static void holdAlreadyOpenWhenItsLoaderArrivesIsNotWritten();
    static void holdIsClosedAgainWhenItsLoaderFinishes();
    static void holdClosedUnderItsLoaderIsOpenedAgain();
    static void holdThatDoesNotFollowTheCommandIsNotWrittenAgain();
    static void holdsStayUntouchedUntilTheCouatlStarts();
    static void holdIsNotWrittenBeforeItsDoorArrives();
    static void departureHoldKeepsAHoldShutWithItsLoaderAtTheDoor();
    static void evaluatingTheHoldsRuleWritesNoVariable();
    static void aftDoorCloseNamesTheJetwayWhenOneIsDocked();
    static void aftDoorCloseNamesTheOwnAirstairWithoutAJetway();
    static void frontDoorOpensWithADockedJetway();
    static void departureHoldShutsTheFrontDoor();
    static void airstairStaysStowedUntilTheTurnaroundAsks();
    static void airstairExtendsWhenTheTurnaroundAsksWithPressure();
    static void airstairWaitsWithoutAccumulatorPressure();
    static void theOwnStairsReportWaitingForPressureOnlyWhileThePressureStopsThem();
    static void theOwnStairsReportWaitingForPressureWhenTheyMustStowAtDeparture();
    static void theOwnStairsAreNotWaitingForPressureUnderAJetway();
    static void airstairIsNotArmedUnderAJetway();
    static void airstairStaysStowedWhileTheJetwayIsStillDriving();
    static void airstairIsNotArmedWhenGsxServesTheFrontDoor();
    static void airstairYieldsToAStairVehicleOnTheWay();
    static void airstairRetractsAndTheDoorClosesWhenTheRequestIsWithdrawn();
    static void airstairStaysOutWhenPressureIsGoneAtDeparture();
    static void airstairAdoptsAnExtensionMadeOnTheEfb();
    static void airstairIsPutBackOutWhenSomethingElseStowsItMidBoarding();
    static void airstairIsLeftStowedWhenSomethingElseStowsItAtDeparture();
    static void airstairFoundExtendedIsLeftOutBeforeTheTurnaroundAsks();
    static void airstairStaysOutWhileCouatlIsDown();
    static void airstairFoundExtendedIsStowedWhenAGsxStairTakesTheFrontDoor();
    static void airstairFoundExtendedIsStowedUnderAnAvailableJetway();
    static void frontDoorWaitsForThePhysicallyStowedStair();
    static void frontDoorAlreadyReadingClosedIsNotToldToClose();
    static void frontDoorAlreadyReadingOpenIsNotToldToOpen();
    static void frontDoorWithoutAReadingIsCommandedBothWays();
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
    static void isReachableOnceTheFuelTheEmptyWeightAndTheTanksHaveArrived();
    static void isReachableAsksForNoLVarOfTheModule();
    static void aResumedAircraftWithoutAJetwayAsksForItsOwnAirstairFromCallServicesOn();
    static void aResumedAircraftPutsTheAirstairBackOutAfterTheAircraftStowsItAtTheBoardingEdge();
    static void aResumedAircraftBeforeCallServicesDoesNotAskForItsOwnAirstair();
    static void aResumedAircraftUnderAJetwayDoesNotAskForItsOwnAirstair();
    static void aResumedAircraftAsksForItsOwnAirstairOnlyOnceTheJetwayReadingHasArrived();
    static void aResumedAircraftUnderAJetwayWhoseReadingArrivesLateNeverAsksForItsOwnAirstair();
    static void anAircraftNeverAsksForItsOwnAirstairOnAJetwayReadingThatHasNotArrived();
    static void theAirstairRuleHoldsTheFlowUntilTheJetwayReadingSaysAJetwayServesTheDoor();
    static void theAirstairRequestIsForgottenWhenTheNextTurnaroundStarts();
    static void aRestoredOpenFrontDoorIsNotClosedBeforeTheVehicleStatesArrive();
    static void aRestoredOpenFrontDoorStaysOpenUnderTheJetwayOnceTheStatesArrive();
    static void aRestoredOpenFrontDoorClosesOnceTheStatesArriveWithNothingServingIt();
    static void theFrontDoorTargetIsOnlyRememberedOnceTheAircraftHasCommandedIt();
    static void aGsxRestartedSinceTheSaveDistrustsTheVehicleStatesAtTheResume();
    static void holdingTheDoorsClosedOnARelaunchedAircraftWritesNoDoor();
    static void resumingNeverAsksForTheDoorsToBeClosed();
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

void AvroRjTest::holdTheAircraftOpenedIsClosedWhileBoardingHasNoLoaderAtIt()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;
    gateway.lvars[kAftHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kFwdHold), 0.0);
    QCOMPARE(gateway.Written(kAftHold), 0.0);
}

void AvroRjTest::holdTheAircraftOpenedIsClosedWhileDeboardingHasNoLoaderAtIt()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kDeboardingState] = kBoardingRequested;
    gateway.lvars[kFwdHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kFwdHold), 0.0);
    QCOMPARE(gateway.WriteCount(kAftHold), 0);
}

void AvroRjTest::openHoldIsClosedWithNoLoaderAtItEvenOutsideAService()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFwdHold] = 1.0;
    gateway.lvars[kAftHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.Written(kFwdHold), 0.0);
    QCOMPARE(gateway.Written(kAftHold), 0.0);
    QCOMPARE(gateway.WriteCount(kFwdHold), 1);
    QCOMPARE(gateway.WriteCount(kAftHold), 1);
}

void AvroRjTest::holdGivesTheAircraftAHeadStartBeforeCommanding()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks - 1);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kFwdHold), 0.0);
    QCOMPARE(gateway.WriteCount(kFwdHold), 1);
}

void AvroRjTest::holdTheAircraftMovesWithinTheHeadStartIsNotWritten()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 0.0;
    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks - 2);

    gateway.lvars[kFwdHold] = 1.0;
    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
}

void AvroRjTest::holdHeadStartRestartsAfterTheHoldAgreedAgain()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks - 2);

    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;
    TickAircraft(aircraft, gateway);

    gateway.lvars[kLoaderFrontState] = 0.0;
    TickHolds(aircraft, gateway, kHoldHeadStartTicks - 1);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kFwdHold), 1);
}

void AvroRjTest::holdOpensWhenItsOwnLoaderReachesItAndTheOtherIsLeftAlone()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 0.0;
    gateway.lvars[kAftHold] = 0.0;
    gateway.lvars[kLoaderRearState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kAftHold), 1.0);
    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
}

void AvroRjTest::holdAlreadyOpenWhenItsLoaderArrivesIsNotWritten()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;
    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
}

void AvroRjTest::holdIsClosedAgainWhenItsLoaderFinishes()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 0.0;
    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kFwdHold), 1.0);

    gateway.lvars[kLoaderFrontState] = kLoaderFinishing;
    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kFwdHold), 0.0);
    QCOMPARE(gateway.WriteCount(kFwdHold), 2);
}

void AvroRjTest::holdClosedUnderItsLoaderIsOpenedAgain()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kAftHold] = 0.0;
    gateway.lvars[kLoaderRearState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.WriteCount(kAftHold), 1);

    TickAircraft(aircraft, gateway);
    gateway.lvars[kAftHold] = 0.0;
    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.Written(kAftHold), 1.0);
    QCOMPARE(gateway.WriteCount(kAftHold), 2);
}

void AvroRjTest::holdThatDoesNotFollowTheCommandIsNotWrittenAgain()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks);

    QCOMPARE(gateway.WriteCount(kFwdHold), 1);

    for (int tick = 0; tick < kHoldHeadStartTicks * 2; ++tick)
    {
        gateway.lvars[kFwdHold] = 1.0;
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(kFwdHold), 1);
}

void AvroRjTest::holdsStayUntouchedUntilTheCouatlStarts()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;
    gateway.lvars[kAftHold] = 0.0;
    gateway.lvars[kLoaderRearState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
    QCOMPARE(gateway.WriteCount(kAftHold), 0);
}

void AvroRjTest::holdIsNotWrittenBeforeItsDoorArrives()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;

    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
}

void AvroRjTest::departureHoldKeepsAHoldShutWithItsLoaderAtTheDoor()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 0.0;
    gateway.lvars[kLoaderFrontState] = kLoaderWaitingForDoor;
    aircraft.HoldDoorsClosed(true);

    TickHolds(aircraft, gateway, kHoldHeadStartTicks * 2);

    QCOMPARE(gateway.WriteCount(kFwdHold), 0);
}

void AvroRjTest::evaluatingTheHoldsRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    FakeVariableWriter writer;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = kBoardingActive;
    gateway.lvars[kFwdHold] = 1.0;
    gateway.lvars[kLoaderRearState] = kLoaderWaitingForDoor;
    aircraft.Observe();

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-holds-follow-their-loader");

    QVERIFY(rule != nullptr);

    const RuleContext context{};
    const int writesBefore = gateway.setLVarCalls + gateway.setAVarCalls;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, writesBefore);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);
}

void AvroRjTest::aftDoorCloseNamesTheJetwayWhenOneIsDocked()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kAftPaxDoor] = 1.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kAftPaxDoor), 0.0);
    QVERIFY(LogCapture::Contains("Closing the 2L: passengers board through the jetway"));
    QVERIFY(!LogCapture::Contains("own airstair"));
}

void AvroRjTest::aftDoorCloseNamesTheOwnAirstairWithoutAJetway()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kAftPaxDoor] = 1.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kAftPaxDoor), 0.0);
    QVERIFY(LogCapture::Contains("Closing the 2L: this aircraft boards through its own airstair"));
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

void AvroRjTest::theOwnStairsReportWaitingForPressureOnlyWhileThePressureStopsThem()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureDepleted;

    QVERIFY(!aircraft.AreOwnStairsWaitingForPressure());

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QVERIFY(aircraft.AreOwnStairsWaitingForPressure());

    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.Written(kStairArmClickspot), 1.0);
    QVERIFY(!aircraft.AreOwnStairsWaitingForPressure());
}

void AvroRjTest::theOwnStairsReportWaitingForPressureWhenTheyMustStowAtDeparture()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
    QVERIFY(!aircraft.AreOwnStairsWaitingForPressure());

    gateway.lvars[kStairAccumPressure] = kStairPressureDepleted;
    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QVERIFY(aircraft.AreOwnStairsWaitingForPressure());
}

void AvroRjTest::theOwnStairsAreNotWaitingForPressureUnderAJetway()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kStairAccumPressure] = kStairPressureDepleted;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QVERIFY(!aircraft.AreOwnStairsWaitingForPressure());
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

void AvroRjTest::airstairIsPutBackOutWhenSomethingElseStowsItMidBoarding()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);

    gateway.lvars[kStairExtendSwitch] = 0.0;
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QVERIFY(LogCapture::Contains("Something else stowed the airstair while passengers still need it"));

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 2);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 2);
    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
}

void AvroRjTest::airstairIsLeftStowedWhenSomethingElseStowsItAtDeparture()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);

    aircraft.HoldDoorsClosed(true);
    gateway.lvars[kStairExtendSwitch] = 0.0;
    for (int tick = 0; tick < 5; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(!LogCapture::Contains("Something else stowed the airstair"));
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 1);
}

void AvroRjTest::airstairFoundExtendedIsLeftOutBeforeTheTurnaroundAsks()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    for (int tick = 0; tick < 4; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);

    for (int tick = 0; tick < 4; ++tick)
    {
        TickAircraft(aircraft, gateway, kPassengerAccess);
    }

    QVERIFY(aircraft.AreAirstairsSettled());
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
}

void AvroRjTest::airstairStaysOutWhileCouatlIsDown()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    gateway.lvars[kCouatlStarted] = 0.0;
    gateway.lvars[kJetway] = 0.0;
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
}

void AvroRjTest::airstairFoundExtendedIsStowedWhenAGsxStairTakesTheFrontDoor()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);

    gateway.lvars[kStairsFrontState] = 2.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.Written(kStairExtendSwitch), 0.0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
}

void AvroRjTest::airstairFoundExtendedIsStowedUnderAnAvailableJetway()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = 1.0;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);
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

void AvroRjTest::frontDoorAlreadyReadingClosedIsNotToldToClose()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars["EXT_Door_stairs_pos"] = 190.0;

    TickAircraft(aircraft, gateway, kPassengerAccess);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 1);

    gateway.lvars[kStairExtendSwitch] = 0.0;
    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    gateway.lvars["EXT_Door_pax_1L"] = 0.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 1);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 0.0);

    aircraft.HoldDoorsClosed(false);
    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 2);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);
}

void AvroRjTest::frontDoorAlreadyReadingOpenIsNotToldToOpen()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;
    gateway.lvars["EXT_Door_pax_1L"] = 1.0;

    TickAircraft(aircraft, gateway);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 0);

    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 1);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 0.0);
}

void AvroRjTest::frontDoorWithoutAReadingIsCommandedBothWays()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars["EXT_Door_stairs_pos"] = 50.0;

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 1);
    QCOMPARE(gateway.Written("EXT_Door_pax_1L"), 1.0);

    gateway.lvars.erase("EXT_Door_pax_1L");
    aircraft.HoldDoorsClosed(true);
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("EXT_Door_pax_1L"), 2);
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
    const LogCapture log;

    gateway.lvars[kModuleFuelMirror] = 1000.0;
    gateway.avars[kSimFuelTotalKg] = 1000.0;

    TickAircraft(aircraft, gateway);

    QVERIFY(!LogCapture::Contains("stopped mirroring the simulator's fuel"));

    for (int tick = 1; tick <= 6; ++tick)
    {
        gateway.avars[kSimFuelTotalKg] = 1000.0 + tick * 100.0;
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(LogCapture::Contains("stopped mirroring the simulator's fuel"));
}

void AvroRjTest::moduleLivenessHoldsWhileTheMirrorFollows()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

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

    QVERIFY(!LogCapture::Contains("stopped mirroring the simulator's fuel"));
}

void AvroRjTest::moduleLivenessIgnoresDivergenceWhileFuelIsStill()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    const LogCapture log;

    gateway.lvars[kModuleFuelMirror] = 0.0;
    gateway.avars[kSimFuelTotalKg] = 6000.0;

    for (int tick = 0; tick < 8; ++tick)
    {
        TickAircraft(aircraft, gateway);
    }

    QVERIFY(!LogCapture::Contains("stopped mirroring the simulator's fuel"));
}

void AvroRjTest::evaluatingTheAirstairRuleWritesNoVariable()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);
    FakeVariableWriter writer;

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-hold-for-own-airstair");

    QVERIFY(rule != nullptr);

    gateway.lvars[kJetway] = kJetwayUnavailable;

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
    const LogCapture log;

    gateway.lvars[kModuleFuelMirror] = 0.0;

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-watch-module-fuel-mirror");

    QVERIFY(rule != nullptr);

    const RuleContext context{};

    for (int tick = 0; tick < 8; ++tick)
    {
        gateway.avars[kSimFuelTotalKg] = 6000.0 + tick * 100.0;
        QVERIFY(!rule->Evaluate(context).holds);
    }

    QVERIFY(LogCapture::Contains("stopped mirroring the simulator's fuel"));
    QCOMPARE(gateway.setLVarCalls + gateway.setAVarCalls, 0);
    QCOMPARE(writer.setLVarCalls + writer.setAVarCalls, 0);
}

void AvroRjTest::isReachableOnceTheFuelTheEmptyWeightAndTheTanksHaveArrived()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    QVERIFY(!aircraft.IsReachable());

    gateway.avars[kSimFuelTotalKg] = 5000.0;

    QVERIFY(!aircraft.IsReachable());

    gateway.avars[kSimEmptyWeight] = 24000.0;

    QVERIFY(!aircraft.IsReachable());

    GiveTanks(gateway);

    QVERIFY(aircraft.IsReachable());
}

void AvroRjTest::isReachableAsksForNoLVarOfTheModule()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.avars[kSimFuelTotalKg] = 5000.0;
    gateway.avars[kSimEmptyWeight] = 24000.0;
    GiveTanks(gateway);
    gateway.requestedLVars.clear();

    QVERIFY(aircraft.IsReachable());
    QVERIFY(gateway.requestedLVars.empty());
}

void AvroRjTest::aResumedAircraftWithoutAJetwayAsksForItsOwnAirstairFromCallServicesOn()
{
    constexpr std::array phases = {TurnaroundPhase::CallServices, TurnaroundPhase::Loading,
                                   TurnaroundPhase::WaitingReadyToPush, TurnaroundPhase::Deboarding};

    for (const TurnaroundPhase phase : phases)
    {
        FakeVariableGateway gateway;
        AvroRj aircraft(&gateway, false);

        gateway.lvars[kCouatlStarted] = 1.0;
        gateway.lvars[kJetway] = kJetwayUnavailable;
        gateway.lvars[kStairAccumPressure] = kStairPressureFull;
        gateway.lvars[kStairPosition] = kStairStowedPosition;

        aircraft.OnTurnaroundResumed(ResumedAt(phase), MemoryBag{});

        TickHolds(aircraft, gateway, 6);

        QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
        QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
        QCOMPARE(gateway.Written(kFwdPaxDoor), 1.0);
    }
}

void AvroRjTest::aResumedAircraftPutsTheAirstairBackOutAfterTheAircraftStowsItAtTheBoardingEdge()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairExtendSwitch] = 1.0;
    gateway.lvars[kStairPosition] = kStairOutPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    TickHolds(aircraft, gateway, 4);

    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);

    gateway.lvars[kStairExtendSwitch] = 0.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    TickHolds(aircraft, gateway, 8);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 1);
    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
}

void AvroRjTest::aResumedAircraftBeforeCallServicesDoesNotAskForItsOwnAirstair()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::PlaceGroundEquipment), MemoryBag{});

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
}

void AvroRjTest::aResumedAircraftUnderAJetwayDoesNotAskForItsOwnAirstair()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.Written(kFwdPaxDoor), 1.0);

    gateway.lvars[kJetway] = 3.0;

    TickHolds(aircraft, gateway, 5);

    QCOMPARE(gateway.Written(kFwdPaxDoor), 0.0);
}

void AvroRjTest::aResumedAircraftAsksForItsOwnAirstairOnlyOnceTheJetwayReadingHasArrived()
{
    FakeVariableGateway gateway;
    gateway.arrivesATickAfterItIsAsked = true;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    for (int tick = 0; tick < 8; ++tick)
    {
        gateway.DeliverWhatWasAsked();
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 1);
    QCOMPARE(gateway.Written(kStairExtendSwitch), 1.0);
    QCOMPARE(gateway.Written(kFwdPaxDoor), 1.0);
}

void AvroRjTest::aResumedAircraftUnderAJetwayWhoseReadingArrivesLateNeverAsksForItsOwnAirstair()
{
    FakeVariableGateway gateway;
    gateway.arrivesATickAfterItIsAsked = true;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayApproaching;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::Loading), MemoryBag{});

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.setLVarCalls, 0);

    for (int tick = 0; tick < 8; ++tick)
    {
        gateway.DeliverWhatWasAsked();
        TickAircraft(aircraft, gateway);
    }

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
}

void AvroRjTest::anAircraftNeverAsksForItsOwnAirstairOnAJetwayReadingThatHasNotArrived()
{
    FakeVariableGateway gateway;
    gateway.arrivesATickAfterItIsAsked = true;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayApproaching;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    TickAircraft(aircraft, gateway, kPassengerAccess);

    for (int tick = 0; tick < 8; ++tick)
    {
        gateway.DeliverWhatWasAsked();
        TickAircraft(aircraft, gateway, kPassengerAccess);
    }

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
}

void AvroRjTest::theAirstairRuleHoldsTheFlowUntilTheJetwayReadingSaysAJetwayServesTheDoor()
{
    FakeVariableGateway gateway;
    gateway.arrivesATickAfterItIsAsked = true;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kJetway] = kJetwayDocked;

    AircraftRule* const rule = FindRule(aircraft, "avro-rj-hold-for-own-airstair");

    QVERIFY(rule != nullptr);

    RuleContext context;
    context.phase = TurnaroundPhase::CallServices;
    context.needs.passengerAccess = true;

    QVERIFY(rule->Evaluate(context).holds);

    gateway.DeliverWhatWasAsked();

    QVERIFY(!rule->Evaluate(context).holds);
}

void AvroRjTest::theAirstairRequestIsForgottenWhenTheNextTurnaroundStarts()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairAccumPressure] = kStairPressureFull;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    TickAircraft(aircraft, gateway, kPassengerAccess);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);

    aircraft.OnTurnaroundStarted();

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kStairArmClickspot), 0);
    QCOMPARE(gateway.WriteCount(kStairExtendSwitch), 0);
    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
}

void AvroRjTest::aRestoredOpenFrontDoorIsNotClosedBeforeTheVehicleStatesArrive()
{
    const MemoryBag memory = MemoryOfAnOpenFrontDoor();

    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFwdPaxDoor] = 1.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::PlaceGroundEquipment), memory);

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
}

void AvroRjTest::aRestoredOpenFrontDoorStaysOpenUnderTheJetwayOnceTheStatesArrive()
{
    const MemoryBag memory = MemoryOfAnOpenFrontDoor();

    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFwdPaxDoor] = 1.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::PlaceGroundEquipment), memory);

    TickHolds(aircraft, gateway, 3);

    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kStairsFrontState] = 0.0;

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
}

void AvroRjTest::aRestoredOpenFrontDoorClosesOnceTheStatesArriveWithNothingServingIt()
{
    const MemoryBag memory = MemoryOfAnOpenFrontDoor();

    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kFwdPaxDoor] = 1.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::PlaceGroundEquipment), memory);

    TickHolds(aircraft, gateway, 3);

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);

    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairsFrontState] = 0.0;

    TickHolds(aircraft, gateway, 3);

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 1);
    QCOMPARE(gateway.Written(kFwdPaxDoor), 0.0);
}

void AvroRjTest::theFrontDoorTargetIsOnlyRememberedOnceTheAircraftHasCommandedIt()
{
    FakeVariableGateway deadGateway;
    AvroRj dead(&deadGateway, false);

    deadGateway.lvars[kCouatlStarted] = 1.0;

    TickAircraft(dead, deadGateway);

    const MemoryBag memory = dead.TurnaroundMemory();

    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairsFrontState] = 0.0;
    gateway.lvars[kFwdPaxDoor] = 1.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    aircraft.OnTurnaroundResumed(ResumedAt(TurnaroundPhase::PlaceGroundEquipment), memory);

    TickHolds(aircraft, gateway, 10);

    QCOMPARE(gateway.WriteCount(kFwdPaxDoor), 0);
}

void AvroRjTest::aGsxRestartedSinceTheSaveDistrustsTheVehicleStatesAtTheResume()
{
    const auto frontDoorWritesAfterResuming = [](const bool gsxRestartedSinceSave)
    {
        FakeVariableGateway gateway;
        AvroRj aircraft(&gateway, false);

        gateway.lvars[kCouatlStarted] = 1.0;
        gateway.lvars[kJetway] = kJetwayUnavailable;
        gateway.lvars[kStairsFrontState] = kStairsFinalPosition;
        gateway.lvars[kStairPosition] = kStairStowedPosition;

        TurnaroundFacts facts = ResumedAt(TurnaroundPhase::PlaceGroundEquipment);
        facts.gsxRestartedSinceSave = gsxRestartedSinceSave;
        aircraft.OnTurnaroundResumed(facts, MemoryBag{});

        TickHolds(aircraft, gateway, 5);

        return gateway.WriteCount(kFwdPaxDoor);
    };

    QCOMPARE(frontDoorWritesAfterResuming(true), 0);
    QCOMPARE(frontDoorWritesAfterResuming(false), 1);
}

void AvroRjTest::holdingTheDoorsClosedOnARelaunchedAircraftWritesNoDoor()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    AllDoorsClosed(gateway);
    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayUnavailable;
    gateway.lvars[kStairsFrontState] = 0.0;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    TurnaroundFacts facts = ResumedAt(TurnaroundPhase::WaitingReadyToPush);
    facts.departureDoorsHeld = true;
    aircraft.OnTurnaroundResumed(facts, MemoryBag{});
    aircraft.HoldDoorsClosed(true);

    TickHolds(aircraft, gateway, 20);

    QCOMPARE(gateway.setLVarCalls, 0);
}

void AvroRjTest::resumingNeverAsksForTheDoorsToBeClosed()
{
    FakeVariableGateway gateway;
    AvroRj aircraft(&gateway, false);

    AllDoorsClosed(gateway);
    gateway.lvars[kFwdPaxDoor] = 1.0;
    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kStairPosition] = kStairStowedPosition;

    TurnaroundFacts facts = ResumedAt(TurnaroundPhase::Loading);
    facts.loadingStarted = true;
    facts.gsxRestartedSinceSave = true;
    aircraft.OnTurnaroundResumed(facts, MemoryBag{});

    TickHolds(aircraft, gateway, 20);

    for (const char* doorLVar : kDoorLVars)
    {
        QCOMPARE(gateway.WriteCount(doorLVar), 0);
    }
}

QTEST_APPLESS_MAIN(AvroRjTest)

#include "tst_avro_rj.moc"
