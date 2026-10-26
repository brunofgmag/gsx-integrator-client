#include <QtTest/QTest>

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include "AircraftTicks.h"
#include "doubles/FakeVariableGateway.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/domain/model/MemoryBag.h"
#include "../src/domain/support/Weight.h"
#include "../src/domain/turnaround/TurnaroundFacts.h"
#include "../src/infrastructure/aircraft/bbs/Bbs757.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSetTotalFuel = "BBS_EFBX_SET_TOTAL_FUEL_LBS";
    constexpr auto kSetPassengerWeight = "BBS_EFBX_SET_PAX_WEIGHT_LBS";
    constexpr auto kSetCargoForward = "BBS_EFBX_SET_CARGO_FWD_LBS";
    constexpr auto kSetCargoAft = "BBS_EFBX_SET_CARGO_AFT_LBS";
    constexpr auto kSetCargoWeight = "BBS_EFBX_SET_CARGO_WEIGHT_LBS";
    constexpr std::array kEfbxLVars = {kSetTotalFuel, kSetPassengerWeight, kSetCargoForward, kSetCargoAft};
    constexpr std::array kLoadLVars = {kSetPassengerWeight, kSetCargoForward, kSetCargoAft};
    constexpr double kEfbxIdle = -1.0;

    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimFuelWeightPerGallon = "FUEL WEIGHT PER GALLON";
    constexpr auto kSimGroundVelocity = "GROUND VELOCITY";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kStationPrefix = "PAYLOAD STATION WEIGHT:";

    constexpr auto kCentreTankCapacity = "FUELSYSTEM TANK CAPACITY:1";
    constexpr auto kLeftTankCapacity = "FUELSYSTEM TANK CAPACITY:2";
    constexpr auto kRightTankCapacity = "FUELSYSTEM TANK CAPACITY:3";
    constexpr std::array kTankCapacities = {kCentreTankCapacity, kLeftTankCapacity, kRightTankCapacity};
    constexpr std::array kTankQuantities = {
        "FUELSYSTEM TANK QUANTITY:1", "FUELSYSTEM TANK QUANTITY:2", "FUELSYSTEM TANK QUANTITY:3"
    };

    constexpr double kFuelPoundsPerGallon = 6.7;
    constexpr double kCentreTankGallons = 6924.0;
    constexpr double kWingTankGallons = 2176.0;
    constexpr double kWingsUnusableGallons = 8.0;
    constexpr double kTanksUnusableGallons = 20.0;
    constexpr double kWingsUsableGallons = (2.0 * kWingTankGallons) - kWingsUnusableGallons;
    constexpr double kUsableCapacityGallons =
        kCentreTankGallons + (2.0 * kWingTankGallons) - kTanksUnusableGallons;
    constexpr double kJustBesideTheWingLimitGallons = 0.001;

    struct MeasuredFuelWrite
    {
        double targetLb;
        double writtenLb;
    };

    constexpr std::array kMeasuredFuelWrites = {
        MeasuredFuelWrite{.targetLb = 10000.0, .writtenLb = 10053.6},
        MeasuredFuelWrite{.targetLb = 21000.0, .writtenLb = 21053.6},
        MeasuredFuelWrite{.targetLb = 40000.0, .writtenLb = 40134.0}
    };

    constexpr double kEmptyWeightKg = 45000.0;
    constexpr double kCrewStationLb = 200.0;
    constexpr double kCrewTotalLb = 2.0 * kCrewStationLb;
    constexpr double kThirdStationLb = 50.0;
    constexpr double kTaxiingKnots = 5.0;
    constexpr double kParkedKnots = 0.0;

    constexpr double kPlanFuelKg = 9627.0;
    constexpr double kPlanOperatingEmptyKg = 45400.0;
    constexpr double kPlanPayloadKg = 15000.0;
    constexpr int kPlanPassengers = 120;
    constexpr double kPassengerLb = 185.0;
    constexpr double kPlanPayloadLb = weight::KgToLb(kPlanPayloadKg);
    constexpr double kPlanPassengersLb = kPlanPassengers * kPassengerLb;
    constexpr double kPlanCargoLb = kPlanPayloadLb - kPlanPassengersLb;
    constexpr double kForwardHoldCapacityLb = 10300.0;
    constexpr double kAftHoldCapacityLb = 16300.0;
    constexpr double kHoldCapacitiesLb = kForwardHoldCapacityLb + kAftHoldCapacityLb;
    constexpr double kSmallPayloadKg = 5000.0;
    constexpr double kBeyondAnyPlanKg = 100000.0;
    constexpr double kBeyondTheHoldsPayloadKg = 25000.0;
    constexpr int kBeyondTheHoldsPassengers = 100;

    constexpr double kFuelTargetKg = weight::LbToKg(21000.0);
    constexpr double kPilotFuelKg = weight::LbToKg(30000.0);

    constexpr int kFirstLoadStation = 3;
    constexpr std::array kLoadStations = {3, 4, 5, 6};
    constexpr std::array kOnBoardStationsLb = {2963.0, 28334.0, 1000.0, 1500.0};
    constexpr double kOnBoardPassengersLb = kOnBoardStationsLb[0] + kOnBoardStationsLb[1];
    constexpr double kOnBoardCargoLb = kOnBoardStationsLb[2] + kOnBoardStationsLb[3];
    constexpr double kOnBoardPayloadLb = kOnBoardPassengersLb + kOnBoardCargoLb;

    constexpr std::array kDoorPositions = {
        "B757_DOORS_PAX_L1", "B757_DOORS_PAX_L2", "B757_DOORS_PAX_L3",
        "B757_DOORS_PAX_R1", "B757_DOORS_PAX_R2", "B757_DOORS_PAX_R3",
        "B757_DOORS_CARGO_FWD", "B757_DOORS_CARGO_AFT"
    };
    constexpr auto kL1Switch = "L1_PAX_DOOR_STATE_SWITCH";
    constexpr auto kL2Switch = "L2_PAX_DOOR_STATE_SWITCH";
    constexpr auto kL3Switch = "L3_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR1Switch = "R1_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR2Switch = "R2_PAX_DOOR_STATE_SWITCH";
    constexpr auto kR3Switch = "R3_PAX_DOOR_STATE_SWITCH";
    constexpr auto kForwardCargoSwitch = "FWD_CARGO_DOOR_STATE_SWITCH";
    constexpr auto kAftCargoSwitch = "AFT_CARGO_DOOR_STATE_SWITCH";
    constexpr std::array kDoorSwitches = {
        kL1Switch, kL2Switch, kL3Switch, kR1Switch, kR2Switch, kR3Switch, kForwardCargoSwitch, kAftCargoSwitch
    };

    struct VehicleOpensSwitch
    {
        const char* vehicleLVar;
        double servingState;
        const char* doorSwitch;
    };

    constexpr double kCouatlUp = 1.0;
    constexpr double kCouatlDown = 0.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayAway = 2.0;
    constexpr double kVehicleAway = 1.0;
    constexpr double kSwitchOpen = 1.0;
    constexpr double kSwitchClosed = 0.0;

    constexpr std::array kVehicleSwitches = {
        VehicleOpensSwitch{gsx::lvars::kJetway, kJetwayDocked, kL1Switch},
        VehicleOpensSwitch{gsx::lvars::kPassengerStairsFrontState, gsx::states::kStairsFinalPosition, kL1Switch},
        VehicleOpensSwitch{gsx::lvars::kPassengerStairsMiddleState, gsx::states::kStairsFinalPosition, kL2Switch},
        VehicleOpensSwitch{gsx::lvars::kPassengerStairsRearState, gsx::states::kStairsFinalPosition, kL3Switch},
        VehicleOpensSwitch{gsx::lvars::kCateringFrontState, gsx::states::kCateringInProgress, kR1Switch},
        VehicleOpensSwitch{gsx::lvars::kCateringRearState, gsx::states::kCateringInProgress, kR3Switch},
        VehicleOpensSwitch{gsx::lvars::kBaggageLoaderFrontState, gsx::states::kLoaderLoading, kForwardCargoSwitch},
        VehicleOpensSwitch{gsx::lvars::kBaggageLoaderRearState, gsx::states::kLoaderLoading, kAftCargoSwitch}
    };
    constexpr std::array kInteractivePoints = {
        "INTERACTIVE POINT OPEN:0", "INTERACTIVE POINT OPEN:1", "EXIT OPEN:0", "EXIT OPEN:1"
    };
    constexpr std::size_t kPaxDoorCount = 6;
    constexpr double kPaxDoorFullyOpen = 100.0;
    constexpr double kCargoDoorFullyOpen = 200.0;
    constexpr double kDoorShut = 0.0;

    constexpr auto kCaptainPtt = "B757_PED_AUDIO_1_PTT_SWITCH";
    constexpr auto kFirstOfficerPtt = "B757_PED_AUDIO_2_PTT_SWITCH";

    constexpr auto kBusNine = "ELECTRICAL MAIN BUS VOLTAGE:9";
    constexpr auto kBusEleven = "ELECTRICAL MAIN BUS VOLTAGE:11";
    constexpr double kMeasuredBusVolts = 32.0;

    constexpr auto kGpuCart = "BBS_757_GPU_CART_VISIBLE";
    constexpr auto kParkBrake = "B757_FC_PARK_BRAKE";
    constexpr auto kChocks = "BBS757_vis_chocks";
    constexpr auto kBeaconSwitch = "B757_OVHD_BEACON_LIGHT";
    constexpr auto kNavLightsButton = "B757_OVHD_BEACON";

    constexpr auto kEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kEng3Combustion = "ENG COMBUSTION:3";

    constexpr double kPoundTolerance = 1e-3;
    constexpr double kKgTolerance = 1e-6;

    struct Rig
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        Bbs757 aircraft{&gateway, &status};
    };

    std::string StationVar(const int station)
    {
        return std::string(kStationPrefix) + std::to_string(station);
    }

    bool Near(const double actual, const double expected, const double tolerance = kPoundTolerance)
    {
        return std::abs(actual - expected) < tolerance;
    }

    void ArmTheEfbxLVars(FakeVariableGateway& gateway)
    {
        for (const char* lVar : kEfbxLVars)
        {
            gateway.lvars[lVar] = kEfbxIdle;
        }
    }

    void GiveTanks(FakeVariableGateway& gateway)
    {
        gateway.avars[kSimFuelWeightPerGallon] = kFuelPoundsPerGallon;
        gateway.avars[kCentreTankCapacity] = kCentreTankGallons;
        gateway.avars[kLeftTankCapacity] = kWingTankGallons;
        gateway.avars[kRightTankCapacity] = kWingTankGallons;
    }

    void ParkWithTheCrew(FakeVariableGateway& gateway)
    {
        gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
        gateway.avars[kSimGroundVelocity] = kParkedKnots;
        gateway.avars[StationVar(1)] = kCrewStationLb;
        gateway.avars[StationVar(2)] = kCrewStationLb;
    }

    void PlanTheFlight(AutomationStatus& status, const double payloadKg = kPlanPayloadKg,
                       const int passengers = kPlanPassengers)
    {
        status.flightPlanStatus = FlightPlanStatus::Ready;
        status.plannedFuelKg = kPlanFuelKg;
        status.plannedOperatingEmptyKg = kPlanOperatingEmptyKg;
        status.plannedPayloadKg = payloadKg;
        status.plannedPassengers = passengers;
    }

    void ReadyToBoard(Rig& rig)
    {
        ArmTheEfbxLVars(rig.gateway);
        ParkWithTheCrew(rig.gateway);
        PlanTheFlight(rig.status);
    }

    void MakeReachable(FakeVariableGateway& gateway)
    {
        ArmTheEfbxLVars(gateway);
        GiveTanks(gateway);
        gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
        gateway.avars[kSimFuelTotalKg] = 0.0;
    }

    void AllDoorsAt(FakeVariableGateway& gateway, const double position)
    {
        for (const char* door : kDoorPositions)
        {
            gateway.lvars[door] = position;
        }
    }

    void StartGsx(FakeVariableGateway& gateway)
    {
        gateway.lvars[gsx::lvars::kCouatlStarted] = kCouatlUp;
    }

    void DockTheJetway(FakeVariableGateway& gateway)
    {
        StartGsx(gateway);
        gateway.lvars[gsx::lvars::kJetway] = kJetwayDocked;
    }

    void AllEnginesStopped(FakeVariableGateway& gateway)
    {
        gateway.avars[kEng1Combustion] = 0.0;
        gateway.avars[kEng2Combustion] = 0.0;
    }

    double ZfwWithPayloadLb(const double payloadLb)
    {
        return kEmptyWeightKg + weight::LbToKg(kCrewTotalLb + payloadLb);
    }

    double WrittenPayloadLb(const FakeVariableGateway& gateway)
    {
        double payloadLb = 0.0;
        for (const char* lVar : kLoadLVars)
        {
            payloadLb += gateway.Written(lVar);
        }

        return payloadLb;
    }

    double WrittenCargoLb(const FakeVariableGateway& gateway)
    {
        return gateway.Written(kSetCargoForward) + gateway.Written(kSetCargoAft);
    }

    TurnaroundFacts ResumedPlanFacts()
    {
        TurnaroundFacts facts;
        facts.emptyZfwKg = kEmptyWeightKg;
        facts.plannedZfwKg = kEmptyWeightKg + weight::LbToKg(kCrewTotalLb) + kPlanPayloadKg;
        facts.plannedPassengers = kPlanPassengers;

        return facts;
    }

    void LoadTheStations(FakeVariableGateway& gateway, const std::array<double, kLoadStations.size()>& stationsLb)
    {
        for (std::size_t index = 0; index < stationsLb.size(); ++index)
        {
            gateway.avars[StationVar(kFirstLoadStation + static_cast<int>(index))] = stationsLb[index];
        }
    }

    void ParkWithTheLoadOnBoardAndNoPlan(Rig& rig)
    {
        ArmTheEfbxLVars(rig.gateway);
        ParkWithTheCrew(rig.gateway);
        LoadTheStations(rig.gateway, kOnBoardStationsLb);
    }

    void SettleTheStationsOnTheWrites(FakeVariableGateway& gateway)
    {
        gateway.avars[StationVar(kLoadStations[0])] = gateway.Written(kSetPassengerWeight);
        gateway.avars[StationVar(kLoadStations[1])] = 0.0;
        gateway.avars[StationVar(kLoadStations[2])] = gateway.Written(kSetCargoForward);
        gateway.avars[StationVar(kLoadStations[3])] = gateway.Written(kSetCargoAft);
        ArmTheEfbxLVars(gateway);
    }

    void ReadyToFuelAndBoard(Rig& rig)
    {
        MakeReachable(rig.gateway);
        ParkWithTheCrew(rig.gateway);
        PlanTheFlight(rig.status);
    }

    void WriteTheFuelAndTheLoad(Rig& rig)
    {
        rig.aircraft.SetCurrentFuelKg(kFuelTargetKg);
        rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));
    }

    void ExpectEveryEfbxLVarWrittenTimes(const FakeVariableGateway& gateway, const int times)
    {
        for (const char* lVar : kEfbxLVars)
        {
            QVERIFY2(gateway.WriteCount(lVar) == times, lVar);
        }
    }
}

class Bbs757Test final : public QObject
{
    Q_OBJECT

private slots:
    static void isAPassengerAircraft();
    static void refuelsByTheClient();
    static void boardsByTheClient();
    static void supportsStairsOrJetways();
    static void completesPushbackWithoutTheInterruptMenu();
    static void cannotCommandTheChocks();
    static void cannotCommandTheGroundPower();
    static void doesNotRequireTheEfbFlightPlan();
    static void worksInPounds();
    static void isReachableOnceEverythingArrives();
    static void isNotReachableWithoutTheEmptyWeight();
    static void isNotReachableWithoutTheFuelTotal();
    static void isNotReachableWithoutThePoundsPerGallon();
    static void isNotReachableWithoutAnyTankCapacity();
    static void isNotReachableWithoutTheFuelLVar();

    static void writesTheGrossFuelTheVendorShortsByItsUnusableGallons();
    static void writesZeroForAnEmptyOrNegativeFuelTarget();
    static void keepsTheCentreUnusableGallonsOutBelowTheWingLimit();
    static void takesTheCentreUnusableGallonsPastTheWingLimit();
    static void clampsTheFuelTargetToTheUsableCapacity();
    static void waitsForTheFuelLVarBeforeWriting();
    static void waitsForTheTanksBeforeWritingFuel();
    static void writesNoFuelWithoutAPositiveFuelDensity();
    static void writesNoFuelWithoutUsableCapacity();
    static void doesNotRepeatTheSameFuelTarget();
    static void reportsTheUsableCapacityOnceTheTanksArrive();
    static void readsTheCurrentFuelFromTheTotalQuantity();
    static void neverWritesTheTankQuantities();

    static void loadsPassengersAndCargoTogetherByThePlanRatio();
    static void leavesTheCrewOutOfThePayload();
    static void neverLoadsMoreThanThePlannedPayload();
    static void loadsNothingBelowTheEmptyWeightPlusCrew();
    static void splitsTheCargoByHoldCapacityIntoTheTwoHoldLVars();
    static void givesPassengersTheWholePayloadWhenTheirWeightExceedsIt();
    static void loadsOnlyCargoWhenThePlanHasNoPassengers();
    static void loadsNothingWithoutAPlanNorTheStations();
    static void unloadsWhatIsOnBoardWhenThereIsNoPlan();
    static void keepsThePassengerShareWhileTheLoadOnBoardFalls();
    static void loadsNothingWhenAStationOfTheLoadOnBoardHasNotArrived();
    static void prefersTheLivePlanOverTheLoadOnBoard();
    static void prefersTheResumedPlanOverTheLoadOnBoard();
    static void capsTheCargoAtTheHoldCapacities();
    static void usesTheResumedPlanWhenTheStatusHasNoPayload();
    static void forgetsTheResumedPlanWhenANewTurnaroundStarts();
    static void prefersTheLivePlanOverTheResumedOne();
    static void skipsTheLVarsThatHaveNotArrived();
    static void writesAgainOnceAnLVarArrivesAfterASkippedWrite();
    static void doesNotRepeatTheSameLoad();
    static void sendsTheSameWritesAgainWhenANewTurnaroundStarts();
    static void sendsTheSameWritesAgainWhenATurnaroundResumes();
    static void doesNotRepeatAWriteTheVendorConsumedWithinATurnaround();
    static void rewritesOnlyTheLVarsWhoseValueChanged();
    static void waitsForTheEmptyWeightBeforeLoading();
    static void readsTheCurrentZfwFromTheTotalWeightLessTheFuel();

    static void loadsTheFlightPlanWhenTheOfpPayloadEmptyWeightAndCrewAreThere();
    static void waitsForTheOfpToBeReady();
    static void waitsForThePlannedPayload();
    static void waitsForTheEmptyWeightBeforeTheFlightPlan();
    static void waitsForTheCrewBeforeTheFlightPlan();
    static void plansTheZfwAsEmptyPlusCrewPlusPayload();
    static void reportsThePlannedFuel();
    static void reportsThePlannedOperatingEmptyWeight();
    static void reportsThePlannedPassengers();
    static void reportsTheEmptyWeight();
    static void countsOnlyThePilotStationsAsCrew();
    static void countsTheCrewOnlyWhileStopped();
    static void countsNoCrewUntilBothPilotStationsArrive();
    static void countsNoCrewNorLoadsThePlanUntilTheGroundVelocityArrives();

    static void doorStatusIsUnknownUntilEveryDoorLVarArrives();
    static void doorStatusIsAllClosedWithEveryDoorAtZero();
    static void anyDoorAboveZeroReportsOpen();
    static void anOpenDoorWinsOverADoorThatHasNotArrived();
    static void doorStatusIgnoresTheInteractivePointsAndExits();
    static void closeAllDoorsCommandsTheEightSwitchesToZero();
    static void closeAllDoorsTellsTheSyncSoItDoesNotCloseThemAgain();
    static void registersTheDoorsFollowGsxRule();
    static void theJetwayDockingOpensTheForwardPassengerSwitch();
    static void theJetwayLeavingClosesTheForwardPassengerSwitch();
    static void theFrontLoaderServingOpensTheForwardCargoSwitch();
    static void everyGsxVehicleOpensItsOwnDoorSwitchAndNoOther();
    static void holdingTheDoorsClosedKeepsTheDockedJetwayFromOpeningThem();
    static void holdingThePassengerDoorsClosedStillLetsTheCargoDoorOpen();
    static void noDoorMovesBeforeCouatlHasStarted();
    static void aGsxRestartDistrustsTheVehicleStatesItLeftBehind();
    static void aResumedAircraftClosesTheDoorWhoseVehicleLeftWhileTheClientWasGone();

    static void registersBothPushToTalkSwitchesForFastRefresh();
    static void aCaptainPushToTalkPulseFiresOnce();
    static void aFirstOfficerPushToTalkPulseFiresOnce();
    static void firingLeavesThePushToTalkSwitchesAlone();
    static void anIdlePushToTalkNeverFiresNorWrites();

    static void poweredFromEitherMainBus();
    static void notPoweredByDefaultNorAtZeroVolts();
    static void groundPowerIsUnknownUntilTheCartLVarArrives();
    static void groundPowerIsConnectedWithTheCart();
    static void groundPowerDefersToTheGsxWithoutTheCart();
    static void parkingBrakeFollowsTheVendorLever();
    static void chocksHoldInPlaceWithoutTheBrake();
    static void ignoresTheSimParkingBrakeTheChocksForce();
    static void ownGroundEquipmentIsAttachedUntilBothLVarsArrive();
    static void ownGroundEquipmentIsAttachedWithTheChocksOnly();
    static void ownGroundEquipmentIsAttachedWithTheGpuCartOnly();
    static void ownGroundEquipmentIsGoneWhenBothLVarsReadZero();
    static void beaconFollowsTheOverheadSwitchAndNotTheNavLightsButton();
    static void enginesAreAssumedRunningUntilBothCombustionsArrive();
    static void aThirdEngineIsNotCounted();
    static void readyToPushNeedsPower();
    static void readyToPushNeedsStoppedEngines();
    static void readyToPushNeedsTheBeacon();
    static void readyToDeboardNeedsStoppedEngines();
    static void readyToDeboardNeedsHoldingInPlace();
    static void readyToDeboardNeedsTheBeaconOff();
    static void readyToDeboardHoldsUntilTheBeaconLVarArrives();
};

void Bbs757Test::isAPassengerAircraft()
{
    Rig rig;

    QVERIFY(!rig.aircraft.IsCargoVariant());
}

void Bbs757Test::refuelsByTheClient()
{
    Rig rig;

    QCOMPARE(rig.aircraft.GetRefuelMethod(), RefuelBy::Client);
}

void Bbs757Test::boardsByTheClient()
{
    Rig rig;

    QCOMPARE(rig.aircraft.GetBoardMethod(), BoardBy::Client);
}

void Bbs757Test::supportsStairsOrJetways()
{
    Rig rig;

    QVERIFY(rig.aircraft.SupportsStairsOrJetways());
}

void Bbs757Test::completesPushbackWithoutTheInterruptMenu()
{
    Rig rig;

    QVERIFY(!rig.aircraft.CompletesPushbackViaInterruptMenu());
}

void Bbs757Test::cannotCommandTheChocks()
{
    Rig rig;

    QVERIFY(!rig.aircraft.SupportsChocksControl());
}

void Bbs757Test::cannotCommandTheGroundPower()
{
    Rig rig;

    QVERIFY(!rig.aircraft.SupportsGroundPowerControl());
}

void Bbs757Test::doesNotRequireTheEfbFlightPlan()
{
    Rig rig;

    QVERIFY(!rig.aircraft.RequiresEfbFlightPlan());
}

void Bbs757Test::worksInPounds()
{
    Rig rig;

    QVERIFY(rig.aircraft.GetNativeWeightUnit().has_value());
    QCOMPARE(*rig.aircraft.GetNativeWeightUnit(), WeightUnit::Lb);
}

void Bbs757Test::isReachableOnceEverythingArrives()
{
    Rig rig;

    QVERIFY(!rig.aircraft.IsReachable());

    MakeReachable(rig.gateway);

    QVERIFY(rig.aircraft.IsReachable());
}

void Bbs757Test::isNotReachableWithoutTheEmptyWeight()
{
    Rig rig;
    MakeReachable(rig.gateway);
    rig.gateway.avars.erase(kSimEmptyWeight);

    QVERIFY(!rig.aircraft.IsReachable());
}

void Bbs757Test::isNotReachableWithoutTheFuelTotal()
{
    Rig rig;
    MakeReachable(rig.gateway);
    rig.gateway.avars.erase(kSimFuelTotalKg);

    QVERIFY(!rig.aircraft.IsReachable());
}

void Bbs757Test::isNotReachableWithoutThePoundsPerGallon()
{
    Rig rig;
    MakeReachable(rig.gateway);
    rig.gateway.avars.erase(kSimFuelWeightPerGallon);

    QVERIFY(!rig.aircraft.IsReachable());
}

void Bbs757Test::isNotReachableWithoutAnyTankCapacity()
{
    for (const char* capacity : kTankCapacities)
    {
        Rig rig;
        MakeReachable(rig.gateway);
        rig.gateway.avars.erase(capacity);

        QVERIFY2(!rig.aircraft.IsReachable(), capacity);
    }
}

void Bbs757Test::isNotReachableWithoutTheFuelLVar()
{
    Rig rig;
    MakeReachable(rig.gateway);
    rig.gateway.lvars.erase(kSetTotalFuel);

    QVERIFY(!rig.aircraft.IsReachable());
}

void Bbs757Test::writesTheGrossFuelTheVendorShortsByItsUnusableGallons()
{
    for (const MeasuredFuelWrite& measured : kMeasuredFuelWrites)
    {
        Rig rig;
        MakeReachable(rig.gateway);

        rig.aircraft.SetCurrentFuelKg(weight::LbToKg(measured.targetLb));

        QVERIFY2(Near(rig.gateway.Written(kSetTotalFuel), measured.writtenLb), std::to_string(measured.targetLb).c_str());
    }
}

void Bbs757Test::writesZeroForAnEmptyOrNegativeFuelTarget()
{
    for (const double targetKg : {0.0, -50.0})
    {
        Rig rig;
        MakeReachable(rig.gateway);

        rig.aircraft.SetCurrentFuelKg(targetKg);

        QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 1);
        QCOMPARE(rig.gateway.Written(kSetTotalFuel), 0.0);
    }
}

void Bbs757Test::keepsTheCentreUnusableGallonsOutBelowTheWingLimit()
{
    Rig rig;
    MakeReachable(rig.gateway);

    const double wingsFullLb = (kWingsUsableGallons - kJustBesideTheWingLimitGallons) * kFuelPoundsPerGallon;
    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(wingsFullLb));

    QVERIFY(Near(rig.gateway.Written(kSetTotalFuel), wingsFullLb + (kWingsUnusableGallons * kFuelPoundsPerGallon)));
}

void Bbs757Test::takesTheCentreUnusableGallonsPastTheWingLimit()
{
    Rig rig;
    MakeReachable(rig.gateway);

    const double centreStartedLb = (kWingsUsableGallons + kJustBesideTheWingLimitGallons) * kFuelPoundsPerGallon;
    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(centreStartedLb));

    QVERIFY(Near(rig.gateway.Written(kSetTotalFuel), centreStartedLb + (kTanksUnusableGallons * kFuelPoundsPerGallon)));
}

void Bbs757Test::clampsTheFuelTargetToTheUsableCapacity()
{
    Rig rig;
    MakeReachable(rig.gateway);

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(1000000.0));

    const double grossCapacityLb =
        (kUsableCapacityGallons + kTanksUnusableGallons) * kFuelPoundsPerGallon;

    QVERIFY(Near(rig.gateway.Written(kSetTotalFuel), grossCapacityLb));
}

void Bbs757Test::waitsForTheFuelLVarBeforeWriting()
{
    Rig rig;
    GiveTanks(rig.gateway);

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

    QCOMPARE(rig.gateway.setLVarCalls, 0);

    rig.gateway.lvars[kSetTotalFuel] = kEfbxIdle;
    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

    QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 1);
    QVERIFY(Near(rig.gateway.Written(kSetTotalFuel), 21053.6));
}

void Bbs757Test::waitsForTheTanksBeforeWritingFuel()
{
    for (const char* avar : {kSimFuelWeightPerGallon, kCentreTankCapacity, kLeftTankCapacity, kRightTankCapacity})
    {
        Rig rig;
        MakeReachable(rig.gateway);
        rig.gateway.avars.erase(avar);

        rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

        QVERIFY2(rig.gateway.WriteCount(kSetTotalFuel) == 0, avar);
    }
}

void Bbs757Test::writesNoFuelWithoutAPositiveFuelDensity()
{
    Rig rig;
    MakeReachable(rig.gateway);
    rig.gateway.avars[kSimFuelWeightPerGallon] = 0.0;

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

    QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 0);
}

void Bbs757Test::writesNoFuelWithoutUsableCapacity()
{
    Rig rig;
    MakeReachable(rig.gateway);
    for (const char* capacity : kTankCapacities)
    {
        rig.gateway.avars[capacity] = 0.0;
    }

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

    QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 0);
}

void Bbs757Test::doesNotRepeatTheSameFuelTarget()
{
    Rig rig;
    MakeReachable(rig.gateway);

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));
    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));

    QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 1);

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(22000.0));

    QCOMPARE(rig.gateway.WriteCount(kSetTotalFuel), 2);
}

void Bbs757Test::reportsTheUsableCapacityOnceTheTanksArrive()
{
    Rig rig;

    QCOMPARE(rig.aircraft.GetFuelCapacityKg(), 0.0);

    GiveTanks(rig.gateway);

    QVERIFY(Near(rig.aircraft.GetFuelCapacityKg(),
                 weight::LbToKg(kUsableCapacityGallons * kFuelPoundsPerGallon), kKgTolerance));

    rig.gateway.avars.erase(kRightTankCapacity);

    QCOMPARE(rig.aircraft.GetFuelCapacityKg(), 0.0);
}

void Bbs757Test::readsTheCurrentFuelFromTheTotalQuantity()
{
    Rig rig;

    rig.gateway.avars[kSimFuelTotalKg] = 12345.0;

    QCOMPARE(rig.aircraft.GetCurrentFuelKg(), 12345.0);
}

void Bbs757Test::neverWritesTheTankQuantities()
{
    Rig rig;
    ReadyToBoard(rig);
    MakeReachable(rig.gateway);

    rig.aircraft.SetCurrentFuelKg(weight::LbToKg(21000.0));
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QCOMPARE(rig.gateway.setAVarCalls, 0);
    for (const char* quantity : kTankQuantities)
    {
        QCOMPARE(rig.gateway.AVarWriteCount(quantity), 0);
    }
}

void Bbs757Test::loadsPassengersAndCargoTogetherByThePlanRatio()
{
    Rig rig;
    ReadyToBoard(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb / 2.0));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb / 2.0));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), kPlanCargoLb / 2.0));

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), kPlanCargoLb));
}

void Bbs757Test::leavesTheCrewOutOfThePayload()
{
    Rig rig;
    ReadyToBoard(rig);
    constexpr double payloadLb = 10000.0;

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(payloadLb));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), payloadLb));
}

void Bbs757Test::neverLoadsMoreThanThePlannedPayload()
{
    Rig rig;
    ReadyToBoard(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb * 2.0));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), kPlanPayloadLb));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb));
}

void Bbs757Test::loadsNothingBelowTheEmptyWeightPlusCrew()
{
    Rig rig;
    ReadyToBoard(rig);

    rig.aircraft.SetCurrentZfwKg(kEmptyWeightKg);

    for (const char* lVar : kLoadLVars)
    {
        QVERIFY2(rig.gateway.WriteCount(lVar) == 1, lVar);
        QVERIFY2(rig.gateway.Written(lVar) == 0.0, lVar);
    }
}

void Bbs757Test::splitsTheCargoByHoldCapacityIntoTheTwoHoldLVars()
{
    Rig rig;
    ReadyToBoard(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QVERIFY(Near(rig.gateway.Written(kSetCargoForward), kPlanCargoLb * kForwardHoldCapacityLb / kHoldCapacitiesLb));
    QVERIFY(Near(rig.gateway.Written(kSetCargoAft), kPlanCargoLb * kAftHoldCapacityLb / kHoldCapacitiesLb));
    QCOMPARE(rig.gateway.WriteCount(kSetCargoWeight), 0);
}

void Bbs757Test::givesPassengersTheWholePayloadWhenTheirWeightExceedsIt()
{
    Rig rig;
    ReadyToBoard(rig);
    PlanTheFlight(rig.status, kSmallPayloadKg, kPlanPassengers);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(weight::KgToLb(kBeyondAnyPlanKg)));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), weight::KgToLb(kSmallPayloadKg)));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), 0.0));
}

void Bbs757Test::loadsOnlyCargoWhenThePlanHasNoPassengers()
{
    Rig rig;
    ReadyToBoard(rig);
    PlanTheFlight(rig.status, kSmallPayloadKg, 0);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(weight::KgToLb(kSmallPayloadKg)));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), 0.0));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), weight::KgToLb(kSmallPayloadKg)));
}

void Bbs757Test::loadsNothingWithoutAPlanNorTheStations()
{
    for (const int passengers : {0, kPlanPassengers})
    {
        Rig rig;
        ArmTheEfbxLVars(rig.gateway);
        ParkWithTheCrew(rig.gateway);
        rig.status.plannedPassengers = passengers;

        rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

        for (const char* lVar : kLoadLVars)
        {
            QVERIFY2(std::isfinite(rig.gateway.Written(lVar)), lVar);
            QVERIFY2(rig.gateway.Written(lVar) == 0.0, lVar);
        }
    }
}

void Bbs757Test::unloadsWhatIsOnBoardWhenThereIsNoPlan()
{
    Rig rig;
    ParkWithTheLoadOnBoardAndNoPlan(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kOnBoardPayloadLb / 2.0));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kOnBoardPassengersLb / 2.0));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), kOnBoardCargoLb / 2.0));
    QVERIFY(Near(rig.gateway.Written(kSetCargoForward),
                 kOnBoardCargoLb / 2.0 * kForwardHoldCapacityLb / kHoldCapacitiesLb));
}

void Bbs757Test::keepsThePassengerShareWhileTheLoadOnBoardFalls()
{
    Rig rig;
    ParkWithTheLoadOnBoardAndNoPlan(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kOnBoardPayloadLb / 2.0));
    SettleTheStationsOnTheWrites(rig.gateway);
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kOnBoardPayloadLb / 4.0));

    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kOnBoardPassengersLb / 4.0));
    QVERIFY(Near(WrittenCargoLb(rig.gateway), kOnBoardCargoLb / 4.0));
}

void Bbs757Test::loadsNothingWhenAStationOfTheLoadOnBoardHasNotArrived()
{
    for (const int missing : kLoadStations)
    {
        Rig rig;
        ParkWithTheLoadOnBoardAndNoPlan(rig);
        rig.gateway.avars.erase(StationVar(missing));

        rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kOnBoardPayloadLb));

        for (const char* lVar : kLoadLVars)
        {
            QVERIFY2(std::isfinite(rig.gateway.Written(lVar)), lVar);
            QVERIFY2(rig.gateway.Written(lVar) == 0.0, lVar);
        }
    }
}

void Bbs757Test::prefersTheLivePlanOverTheLoadOnBoard()
{
    Rig rig;
    ReadyToBoard(rig);
    LoadTheStations(rig.gateway, kOnBoardStationsLb);
    PlanTheFlight(rig.status, kSmallPayloadKg, 0);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(weight::KgToLb(kBeyondAnyPlanKg)));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), weight::KgToLb(kSmallPayloadKg)));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), 0.0));
}

void Bbs757Test::prefersTheResumedPlanOverTheLoadOnBoard()
{
    Rig rig;
    ParkWithTheLoadOnBoardAndNoPlan(rig);

    rig.aircraft.OnTurnaroundResumed(ResumedPlanFacts(), MemoryBag{});
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb * 2.0));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), kPlanPayloadLb));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb));
}

void Bbs757Test::capsTheCargoAtTheHoldCapacities()
{
    Rig rig;
    ReadyToBoard(rig);
    PlanTheFlight(rig.status, kBeyondTheHoldsPayloadKg, kBeyondTheHoldsPassengers);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(weight::KgToLb(kBeyondTheHoldsPayloadKg)));

    QVERIFY(Near(rig.gateway.Written(kSetCargoForward), kForwardHoldCapacityLb));
    QVERIFY(Near(rig.gateway.Written(kSetCargoAft), kAftHoldCapacityLb));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kBeyondTheHoldsPassengers * kPassengerLb));
}

void Bbs757Test::usesTheResumedPlanWhenTheStatusHasNoPayload()
{
    Rig rig;
    ArmTheEfbxLVars(rig.gateway);
    ParkWithTheCrew(rig.gateway);

    rig.aircraft.OnTurnaroundResumed(ResumedPlanFacts(), MemoryBag{});
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb * 2.0));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), kPlanPayloadLb));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb));
}

void Bbs757Test::forgetsTheResumedPlanWhenANewTurnaroundStarts()
{
    Rig rig;
    ArmTheEfbxLVars(rig.gateway);
    ParkWithTheCrew(rig.gateway);

    rig.aircraft.OnTurnaroundResumed(ResumedPlanFacts(), MemoryBag{});
    rig.aircraft.OnTurnaroundStarted();
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QCOMPARE(WrittenPayloadLb(rig.gateway), 0.0);
}

void Bbs757Test::prefersTheLivePlanOverTheResumedOne()
{
    Rig rig;
    ReadyToBoard(rig);
    PlanTheFlight(rig.status, kSmallPayloadKg, 0);

    rig.aircraft.OnTurnaroundResumed(ResumedPlanFacts(), MemoryBag{});
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QVERIFY(Near(WrittenPayloadLb(rig.gateway), weight::KgToLb(kSmallPayloadKg)));
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), 0.0));
}

void Bbs757Test::skipsTheLVarsThatHaveNotArrived()
{
    for (const char* missing : kLoadLVars)
    {
        Rig rig;
        ReadyToBoard(rig);
        rig.gateway.lvars.erase(missing);

        rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

        for (const char* lVar : kLoadLVars)
        {
            const bool isMissing = std::string(lVar) == missing;

            QVERIFY2(rig.gateway.WriteCount(lVar) == (isMissing ? 0 : 1), lVar);
        }
    }
}

void Bbs757Test::writesAgainOnceAnLVarArrivesAfterASkippedWrite()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.gateway.lvars.erase(kSetPassengerWeight);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QCOMPARE(rig.gateway.WriteCount(kSetPassengerWeight), 0);

    rig.gateway.lvars[kSetPassengerWeight] = kEfbxIdle;
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QCOMPARE(rig.gateway.WriteCount(kSetPassengerWeight), 1);
    QVERIFY(Near(rig.gateway.Written(kSetPassengerWeight), kPlanPassengersLb));
    QCOMPARE(rig.gateway.WriteCount(kSetCargoForward), 1);
}

void Bbs757Test::doesNotRepeatTheSameLoad()
{
    Rig rig;
    ReadyToBoard(rig);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    for (const char* lVar : kLoadLVars)
    {
        QVERIFY2(rig.gateway.WriteCount(lVar) == 1, lVar);
    }
}

void Bbs757Test::sendsTheSameWritesAgainWhenANewTurnaroundStarts()
{
    Rig rig;
    ReadyToFuelAndBoard(rig);

    WriteTheFuelAndTheLoad(rig);
    ArmTheEfbxLVars(rig.gateway);
    rig.aircraft.OnTurnaroundStarted();
    WriteTheFuelAndTheLoad(rig);

    ExpectEveryEfbxLVarWrittenTimes(rig.gateway, 2);
}

void Bbs757Test::sendsTheSameWritesAgainWhenATurnaroundResumes()
{
    Rig rig;
    ReadyToFuelAndBoard(rig);

    WriteTheFuelAndTheLoad(rig);
    ArmTheEfbxLVars(rig.gateway);
    rig.aircraft.OnTurnaroundResumed(ResumedPlanFacts(), MemoryBag{});
    WriteTheFuelAndTheLoad(rig);

    ExpectEveryEfbxLVarWrittenTimes(rig.gateway, 2);
}

void Bbs757Test::doesNotRepeatAWriteTheVendorConsumedWithinATurnaround()
{
    Rig rig;
    ReadyToFuelAndBoard(rig);

    WriteTheFuelAndTheLoad(rig);
    ArmTheEfbxLVars(rig.gateway);
    rig.gateway.avars[kSimFuelTotalKg] = kFuelTargetKg;
    WriteTheFuelAndTheLoad(rig);

    ExpectEveryEfbxLVarWrittenTimes(rig.gateway, 1);

    rig.gateway.avars[kSimFuelTotalKg] = kPilotFuelKg;
    WriteTheFuelAndTheLoad(rig);

    ExpectEveryEfbxLVarWrittenTimes(rig.gateway, 1);
}

void Bbs757Test::rewritesOnlyTheLVarsWhoseValueChanged()
{
    Rig rig;
    ReadyToBoard(rig);
    PlanTheFlight(rig.status, kPlanPayloadKg, 0);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb / 4.0));
    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb / 2.0));

    QCOMPARE(rig.gateway.WriteCount(kSetPassengerWeight), 1);
    QCOMPARE(rig.gateway.WriteCount(kSetCargoForward), 2);
    QCOMPARE(rig.gateway.WriteCount(kSetCargoAft), 2);
}

void Bbs757Test::waitsForTheEmptyWeightBeforeLoading()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.gateway.avars.erase(kSimEmptyWeight);

    rig.aircraft.SetCurrentZfwKg(ZfwWithPayloadLb(kPlanPayloadLb));

    QCOMPARE(rig.gateway.setLVarCalls, 0);
}

void Bbs757Test::readsTheCurrentZfwFromTheTotalWeightLessTheFuel()
{
    Rig rig;

    QCOMPARE(rig.aircraft.GetCurrentZfwKg(), 0.0);

    rig.gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    rig.gateway.avars[kSimTotalWeight] = 90000.0;
    rig.gateway.avars[kSimFuelTotalKg] = 10000.0;

    QCOMPARE(rig.aircraft.GetCurrentZfwKg(), 80000.0);
}

void Bbs757Test::loadsTheFlightPlanWhenTheOfpPayloadEmptyWeightAndCrewAreThere()
{
    Rig rig;
    ReadyToBoard(rig);

    QVERIFY(rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::waitsForTheOfpToBeReady()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.status.flightPlanStatus = FlightPlanStatus::Fetching;

    QVERIFY(!rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::waitsForThePlannedPayload()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.status.plannedPayloadKg.reset();

    QVERIFY(!rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::waitsForTheEmptyWeightBeforeTheFlightPlan()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.gateway.avars.erase(kSimEmptyWeight);

    QVERIFY(!rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::waitsForTheCrewBeforeTheFlightPlan()
{
    Rig rig;
    ReadyToBoard(rig);
    rig.gateway.avars.erase(StationVar(2));

    QVERIFY(!rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::plansTheZfwAsEmptyPlusCrewPlusPayload()
{
    Rig rig;
    ReadyToBoard(rig);

    const double expectedKg = kEmptyWeightKg + weight::LbToKg(kCrewTotalLb) + kPlanPayloadKg;

    QVERIFY(Near(rig.aircraft.GetPlannedZfwKg(), expectedKg, kKgTolerance));
}

void Bbs757Test::reportsThePlannedFuel()
{
    Rig rig;
    ReadyToBoard(rig);

    QCOMPARE(rig.aircraft.GetPlannedFuelKg(), kPlanFuelKg);
}

void Bbs757Test::reportsThePlannedOperatingEmptyWeight()
{
    Rig rig;
    ReadyToBoard(rig);

    QCOMPARE(rig.aircraft.GetPlannedOperatingEmptyKg(), kPlanOperatingEmptyKg);
}

void Bbs757Test::reportsThePlannedPassengers()
{
    Rig rig;
    ReadyToBoard(rig);

    QCOMPARE(rig.aircraft.GetPlannedPassengers(), kPlanPassengers);
}

void Bbs757Test::reportsTheEmptyWeight()
{
    Rig rig;
    ReadyToBoard(rig);

    QCOMPARE(rig.aircraft.GetEmptyZfwKg(), kEmptyWeightKg);
}

void Bbs757Test::countsOnlyThePilotStationsAsCrew()
{
    Rig rig;
    ParkWithTheCrew(rig.gateway);
    rig.gateway.avars[StationVar(3)] = kThirdStationLb;

    QVERIFY(Near(rig.aircraft.GetCrewOnBoardKg(), weight::LbToKg(kCrewTotalLb), kKgTolerance));
}

void Bbs757Test::countsTheCrewOnlyWhileStopped()
{
    Rig rig;
    ParkWithTheCrew(rig.gateway);
    rig.gateway.avars[kSimGroundVelocity] = kTaxiingKnots;

    QCOMPARE(rig.aircraft.GetCrewOnBoardKg(), 0.0);
}

void Bbs757Test::countsNoCrewUntilBothPilotStationsArrive()
{
    Rig rig;
    ParkWithTheCrew(rig.gateway);
    rig.gateway.avars.erase(StationVar(1));

    QCOMPARE(rig.aircraft.GetCrewOnBoardKg(), 0.0);
}

void Bbs757Test::countsNoCrewNorLoadsThePlanUntilTheGroundVelocityArrives()
{
    Rig rig;
    ReadyToBoard(rig);

    QVERIFY(rig.aircraft.IsFlightPlanLoaded());

    rig.gateway.avars.erase(kSimGroundVelocity);

    QCOMPARE(rig.aircraft.GetCrewOnBoardKg(), 0.0);
    QVERIFY(!rig.aircraft.IsFlightPlanLoaded());
}

void Bbs757Test::doorStatusIsUnknownUntilEveryDoorLVarArrives()
{
    Rig rig;

    QCOMPARE(rig.aircraft.GetDoorStatus(), DoorStatus::Unknown);

    for (const char* missing : kDoorPositions)
    {
        AllDoorsAt(rig.gateway, kDoorShut);
        rig.gateway.lvars.erase(missing);

        QVERIFY2(rig.aircraft.GetDoorStatus() == DoorStatus::Unknown, missing);
    }
}

void Bbs757Test::doorStatusIsAllClosedWithEveryDoorAtZero()
{
    Rig rig;

    AllDoorsAt(rig.gateway, kDoorShut);

    QCOMPARE(rig.aircraft.GetDoorStatus(), DoorStatus::AllClosed);
}

void Bbs757Test::anyDoorAboveZeroReportsOpen()
{
    for (std::size_t door = 0; door < kDoorPositions.size(); ++door)
    {
        Rig rig;
        AllDoorsAt(rig.gateway, kDoorShut);
        rig.gateway.lvars[kDoorPositions[door]] = door < kPaxDoorCount ? kPaxDoorFullyOpen : kCargoDoorFullyOpen;

        QVERIFY2(rig.aircraft.GetDoorStatus() == DoorStatus::AnyOpen, kDoorPositions[door]);
    }
}

void Bbs757Test::anOpenDoorWinsOverADoorThatHasNotArrived()
{
    Rig rig;
    AllDoorsAt(rig.gateway, kDoorShut);
    rig.gateway.lvars.erase(kDoorPositions.back());
    rig.gateway.lvars[kDoorPositions.front()] = kPaxDoorFullyOpen;

    QCOMPARE(rig.aircraft.GetDoorStatus(), DoorStatus::AnyOpen);
}

void Bbs757Test::doorStatusIgnoresTheInteractivePointsAndExits()
{
    Rig rig;
    AllDoorsAt(rig.gateway, kDoorShut);
    for (const char* point : kInteractivePoints)
    {
        rig.gateway.avars[point] = 1.0;
    }

    QCOMPARE(rig.aircraft.GetDoorStatus(), DoorStatus::AllClosed);
}

void Bbs757Test::closeAllDoorsCommandsTheEightSwitchesToZero()
{
    Rig rig;
    for (const char* doorSwitch : kDoorSwitches)
    {
        rig.gateway.lvars[doorSwitch] = 1.0;
    }

    rig.aircraft.CloseAllDoors();

    QCOMPARE(rig.gateway.setLVarCalls, static_cast<int>(kDoorSwitches.size()));
    for (const char* doorSwitch : kDoorSwitches)
    {
        QVERIFY2(rig.gateway.WriteCount(doorSwitch) == 1, doorSwitch);
        QVERIFY2(rig.gateway.Written(doorSwitch) == 0.0, doorSwitch);
    }
}

void Bbs757Test::closeAllDoorsTellsTheSyncSoItDoesNotCloseThemAgain()
{
    Rig rig;
    DockTheJetway(rig.gateway);
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL1Switch), kSwitchOpen);

    rig.aircraft.CloseAllDoors();
    rig.gateway.lvars[gsx::lvars::kJetway] = kJetwayAway;
    TickAircraft(rig.aircraft, rig.gateway);
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL1Switch), kSwitchClosed);
    QCOMPARE(rig.gateway.WriteCount(kL1Switch), 2);
}

void Bbs757Test::registersTheDoorsFollowGsxRule()
{
    Rig rig;

    QVERIFY(FindRule(rig.aircraft, "bbs-757-doors-follow-gsx") != nullptr);
}

void Bbs757Test::theJetwayDockingOpensTheForwardPassengerSwitch()
{
    Rig rig;
    DockTheJetway(rig.gateway);

    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL1Switch), kSwitchOpen);
    QCOMPARE(rig.gateway.WriteCount(kL1Switch), 1);
}

void Bbs757Test::theJetwayLeavingClosesTheForwardPassengerSwitch()
{
    Rig rig;
    DockTheJetway(rig.gateway);
    TickAircraft(rig.aircraft, rig.gateway);

    rig.gateway.lvars[gsx::lvars::kJetway] = kJetwayAway;
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL1Switch), kSwitchClosed);
    QCOMPARE(rig.gateway.WriteCount(kL1Switch), 2);
}

void Bbs757Test::theFrontLoaderServingOpensTheForwardCargoSwitch()
{
    Rig rig;
    StartGsx(rig.gateway);
    rig.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;

    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kForwardCargoSwitch), kSwitchOpen);
}

void Bbs757Test::everyGsxVehicleOpensItsOwnDoorSwitchAndNoOther()
{
    for (const VehicleOpensSwitch& vehicle : kVehicleSwitches)
    {
        Rig rig;
        StartGsx(rig.gateway);
        rig.gateway.lvars[vehicle.vehicleLVar] = vehicle.servingState;

        TickAircraft(rig.aircraft, rig.gateway);

        for (const char* doorSwitch : kDoorSwitches)
        {
            const bool isTheOwn = std::string(doorSwitch) == vehicle.doorSwitch;
            const std::string message = std::string(vehicle.vehicleLVar) + " -> " + doorSwitch;

            QVERIFY2(rig.gateway.WriteCount(doorSwitch) == (isTheOwn ? 1 : 0), message.c_str());
            QVERIFY2(!isTheOwn || rig.gateway.Written(doorSwitch) == kSwitchOpen, message.c_str());
        }
    }
}

void Bbs757Test::holdingTheDoorsClosedKeepsTheDockedJetwayFromOpeningThem()
{
    Rig rig;
    DockTheJetway(rig.gateway);
    rig.aircraft.HoldDoorsClosed(true);

    TickAircraft(rig.aircraft, rig.gateway);
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.WriteCount(kL1Switch), 0);

    rig.aircraft.HoldDoorsClosed(false);
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL1Switch), kSwitchOpen);
}

void Bbs757Test::holdingThePassengerDoorsClosedStillLetsTheCargoDoorOpen()
{
    Rig rig;
    DockTheJetway(rig.gateway);
    rig.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;
    rig.aircraft.HoldPassengerDoorsClosed(true);

    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.WriteCount(kL1Switch), 0);
    QCOMPARE(rig.gateway.Written(kForwardCargoSwitch), kSwitchOpen);
}

void Bbs757Test::noDoorMovesBeforeCouatlHasStarted()
{
    Rig rig;
    rig.gateway.lvars[gsx::lvars::kJetway] = kJetwayDocked;
    rig.gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;

    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.setLVarCalls, 0);
}

void Bbs757Test::aGsxRestartDistrustsTheVehicleStatesItLeftBehind()
{
    Rig rig;
    StartGsx(rig.gateway);
    rig.gateway.lvars[gsx::lvars::kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL3Switch), kSwitchOpen);

    rig.gateway.lvars[gsx::lvars::kCouatlStarted] = kCouatlDown;
    TickAircraft(rig.aircraft, rig.gateway);
    rig.gateway.lvars[gsx::lvars::kCouatlStarted] = kCouatlUp;
    TickAircraft(rig.aircraft, rig.gateway);

    QCOMPARE(rig.gateway.Written(kL3Switch), kSwitchClosed);
}

void Bbs757Test::aResumedAircraftClosesTheDoorWhoseVehicleLeftWhileTheClientWasGone()
{
    Rig before;
    DockTheJetway(before.gateway);
    TickAircraft(before.aircraft, before.gateway);

    QCOMPARE(before.gateway.Written(kL1Switch), kSwitchOpen);

    const MemoryBag memory = before.aircraft.TurnaroundMemory();

    Rig after;
    StartGsx(after.gateway);
    after.gateway.lvars[gsx::lvars::kJetway] = kJetwayAway;
    after.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = kVehicleAway;
    after.aircraft.OnTurnaroundResumed(TurnaroundFacts{}, memory);

    TickAircraft(after.aircraft, after.gateway);
    TickAircraft(after.aircraft, after.gateway);

    QCOMPARE(after.gateway.Written(kL1Switch), kSwitchClosed);
}

void Bbs757Test::registersBothPushToTalkSwitchesForFastRefresh()
{
    Rig rig;

    QCOMPARE(rig.gateway.fastRefreshNames.size(), std::size_t{2});
    QCOMPARE(rig.gateway.fastRefreshNames.front(), std::string{kCaptainPtt});
    QCOMPARE(rig.gateway.fastRefreshNames.back(), std::string{kFirstOfficerPtt});
}

void Bbs757Test::aCaptainPushToTalkPulseFiresOnce()
{
    Rig rig;
    rig.gateway.lvarSpans[kCaptainPtt] = LVarSpan{.min = 0.0, .max = 1.0, .received = true};

    QVERIFY(rig.aircraft.ConsumeSmartSwitch());

    rig.gateway.lvarSpans[kCaptainPtt] = LVarSpan{.min = 0.0, .max = 0.0, .received = true};

    QVERIFY(!rig.aircraft.ConsumeSmartSwitch());
}

void Bbs757Test::aFirstOfficerPushToTalkPulseFiresOnce()
{
    Rig rig;
    rig.gateway.lvarSpans[kFirstOfficerPtt] = LVarSpan{.min = 0.0, .max = 1.0, .received = true};

    QVERIFY(rig.aircraft.ConsumeSmartSwitch());

    rig.gateway.lvarSpans[kFirstOfficerPtt] = LVarSpan{.min = 0.0, .max = 0.0, .received = true};

    QVERIFY(!rig.aircraft.ConsumeSmartSwitch());
}

void Bbs757Test::firingLeavesThePushToTalkSwitchesAlone()
{
    Rig rig;
    rig.gateway.lvarSpans[kCaptainPtt] = LVarSpan{.min = 0.0, .max = 1.0, .received = true};

    QVERIFY(rig.aircraft.ConsumeSmartSwitch());
    QCOMPARE(rig.gateway.setLVarCalls, 0);
}

void Bbs757Test::anIdlePushToTalkNeverFiresNorWrites()
{
    Rig rig;

    rig.gateway.lvarSpans[kCaptainPtt] = LVarSpan{.min = 0.0, .max = 0.0, .received = true};
    rig.gateway.lvarSpans[kFirstOfficerPtt] = LVarSpan{.min = 0.0, .max = 0.0, .received = true};

    QVERIFY(!rig.aircraft.ConsumeSmartSwitch());
    QCOMPARE(rig.gateway.setLVarCalls, 0);
}

void Bbs757Test::poweredFromEitherMainBus()
{
    for (const char* bus : {kBusNine, kBusEleven})
    {
        Rig rig;
        rig.gateway.avars[bus] = kMeasuredBusVolts;

        QVERIFY2(rig.aircraft.IsPowered(), bus);
    }
}

void Bbs757Test::notPoweredByDefaultNorAtZeroVolts()
{
    Rig rig;

    QVERIFY(!rig.aircraft.IsPowered());

    rig.gateway.avars[kBusNine] = 0.0;
    rig.gateway.avars[kBusEleven] = 0.0;

    QVERIFY(!rig.aircraft.IsPowered());
}

void Bbs757Test::groundPowerIsUnknownUntilTheCartLVarArrives()
{
    Rig rig;

    QVERIFY(rig.aircraft.GetGroundPowerStatus().has_value());
    QCOMPARE(*rig.aircraft.GetGroundPowerStatus(), GroundPowerStatus::Unknown);
}

void Bbs757Test::groundPowerIsConnectedWithTheCart()
{
    Rig rig;

    rig.gateway.lvars[kGpuCart] = 1.0;

    QVERIFY(rig.aircraft.GetGroundPowerStatus().has_value());
    QCOMPARE(*rig.aircraft.GetGroundPowerStatus(), GroundPowerStatus::Connected);
}

void Bbs757Test::groundPowerDefersToTheGsxWithoutTheCart()
{
    Rig rig;

    rig.gateway.lvars[kGpuCart] = 0.0;

    QVERIFY(!rig.aircraft.GetGroundPowerStatus().has_value());
}

void Bbs757Test::parkingBrakeFollowsTheVendorLever()
{
    Rig rig;

    QVERIFY(!rig.aircraft.IsHeldInPlace());
    QVERIFY(!rig.aircraft.IsParkingBrakeSet());

    rig.gateway.lvars[kParkBrake] = 1.0;

    QVERIFY(rig.aircraft.IsParkingBrakeSet());
    QVERIFY(rig.aircraft.IsHeldInPlace());
}

void Bbs757Test::chocksHoldInPlaceWithoutTheBrake()
{
    Rig rig;
    rig.gateway.lvars[kParkBrake] = 0.0;
    rig.gateway.lvars[kChocks] = 1.0;

    QVERIFY(!rig.aircraft.IsParkingBrakeSet());
    QVERIFY(rig.aircraft.IsHeldInPlace());
}

void Bbs757Test::ignoresTheSimParkingBrakeTheChocksForce()
{
    Rig rig;

    rig.gateway.avars[kSimParkingBrake] = 1.0;

    QVERIFY(!rig.aircraft.IsParkingBrakeSet());
    QVERIFY(!rig.aircraft.IsHeldInPlace());
}

void Bbs757Test::ownGroundEquipmentIsAttachedUntilBothLVarsArrive()
{
    Rig rig;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());

    rig.gateway.lvars[kChocks] = 0.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());

    rig.gateway.lvars.erase(kChocks);
    rig.gateway.lvars[kGpuCart] = 0.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());

    rig.gateway.lvars[kChocks] = 0.0;

    QVERIFY(!rig.aircraft.IsOwnGroundEquipmentAttached());
}

void Bbs757Test::ownGroundEquipmentIsAttachedWithTheChocksOnly()
{
    Rig rig;
    rig.gateway.lvars[kGpuCart] = 0.0;
    rig.gateway.lvars[kChocks] = 1.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());
}

void Bbs757Test::ownGroundEquipmentIsAttachedWithTheGpuCartOnly()
{
    Rig rig;
    rig.gateway.lvars[kGpuCart] = 1.0;
    rig.gateway.lvars[kChocks] = 0.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());
}

void Bbs757Test::ownGroundEquipmentIsGoneWhenBothLVarsReadZero()
{
    Rig rig;
    rig.gateway.lvars[kGpuCart] = 1.0;
    rig.gateway.lvars[kChocks] = 1.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());

    rig.gateway.lvars[kGpuCart] = 0.0;

    QVERIFY(rig.aircraft.IsOwnGroundEquipmentAttached());

    rig.gateway.lvars[kChocks] = 0.0;

    QVERIFY(!rig.aircraft.IsOwnGroundEquipmentAttached());
}

void Bbs757Test::beaconFollowsTheOverheadSwitchAndNotTheNavLightsButton()
{
    Rig rig;
    rig.gateway.avars[kBusNine] = kMeasuredBusVolts;
    AllEnginesStopped(rig.gateway);

    rig.gateway.lvars[kNavLightsButton] = 1.0;
    rig.gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!rig.aircraft.IsReadyToPush());

    rig.gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(rig.aircraft.IsReadyToPush());
}

void Bbs757Test::enginesAreAssumedRunningUntilBothCombustionsArrive()
{
    Rig rig;

    QVERIFY(rig.aircraft.IsEngineRunning());

    rig.gateway.avars[kEng1Combustion] = 0.0;

    QVERIFY(rig.aircraft.IsEngineRunning());

    rig.gateway.avars[kEng2Combustion] = 0.0;

    QVERIFY(!rig.aircraft.IsEngineRunning());

    rig.gateway.avars[kEng2Combustion] = 1.0;

    QVERIFY(rig.aircraft.IsEngineRunning());
}

void Bbs757Test::aThirdEngineIsNotCounted()
{
    Rig rig;
    AllEnginesStopped(rig.gateway);
    rig.gateway.avars[kEng3Combustion] = 1.0;

    QVERIFY(!rig.aircraft.IsEngineRunning());
}

void Bbs757Test::readyToPushNeedsPower()
{
    Rig rig;
    rig.gateway.lvars[kBeaconSwitch] = 1.0;
    AllEnginesStopped(rig.gateway);

    QVERIFY(!rig.aircraft.IsReadyToPush());

    rig.gateway.avars[kBusEleven] = kMeasuredBusVolts;

    QVERIFY(rig.aircraft.IsReadyToPush());
}

void Bbs757Test::readyToPushNeedsStoppedEngines()
{
    Rig rig;
    rig.gateway.avars[kBusEleven] = kMeasuredBusVolts;
    rig.gateway.lvars[kBeaconSwitch] = 1.0;
    AllEnginesStopped(rig.gateway);
    rig.gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(!rig.aircraft.IsReadyToPush());
}

void Bbs757Test::readyToPushNeedsTheBeacon()
{
    Rig rig;
    rig.gateway.avars[kBusEleven] = kMeasuredBusVolts;
    AllEnginesStopped(rig.gateway);

    QVERIFY(!rig.aircraft.IsReadyToPush());
}

void Bbs757Test::readyToDeboardNeedsStoppedEngines()
{
    Rig rig;
    rig.gateway.lvars[kChocks] = 1.0;
    rig.gateway.lvars[kBeaconSwitch] = 0.0;
    AllEnginesStopped(rig.gateway);

    QVERIFY(rig.aircraft.IsReadyToDeboard());

    rig.gateway.avars[kEng2Combustion] = 1.0;

    QVERIFY(!rig.aircraft.IsReadyToDeboard());
}

void Bbs757Test::readyToDeboardNeedsHoldingInPlace()
{
    Rig rig;
    rig.gateway.lvars[kBeaconSwitch] = 0.0;
    AllEnginesStopped(rig.gateway);

    QVERIFY(!rig.aircraft.IsReadyToDeboard());
}

void Bbs757Test::readyToDeboardNeedsTheBeaconOff()
{
    Rig rig;
    rig.gateway.lvars[kParkBrake] = 1.0;
    rig.gateway.lvars[kBeaconSwitch] = 1.0;
    AllEnginesStopped(rig.gateway);

    QVERIFY(!rig.aircraft.IsReadyToDeboard());
}

void Bbs757Test::readyToDeboardHoldsUntilTheBeaconLVarArrives()
{
    Rig rig;
    rig.gateway.lvars[kChocks] = 1.0;
    AllEnginesStopped(rig.gateway);

    QVERIFY(!rig.aircraft.IsReadyToDeboard());

    rig.gateway.lvars[kBeaconSwitch] = 0.0;

    QVERIFY(rig.aircraft.IsReadyToDeboard());

    rig.gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(!rig.aircraft.IsReadyToDeboard());
}

QTEST_APPLESS_MAIN(Bbs757Test)

#include "tst_bbs_757.moc"
