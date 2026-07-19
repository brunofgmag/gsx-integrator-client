#include <QtTest/QTest>

#include <string>
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/infrastructure/aircraft/TolissA340.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSmartSwitch = "AB_ACP_CPT_RTU_Switch";
    constexpr auto kParkingBrakeLvar = "PARKINGBRAKE_POSITION";
    constexpr auto kBattery1 = "AB_VC_OVH_ELEC_BAT1_OFF_PB";
    constexpr auto kBattery2 = "AB_VC_OVH_ELEC_BAT2_OFF_PB";
    constexpr auto kExtAPb = "AB_VC_OVH_ELEC_EXTA_PB";
    constexpr auto kExtAAuto = "AB_VC_OVH_ELEC_EXTA_AUTO";
    constexpr auto kExtBPb = "AB_VC_OVH_ELEC_EXTB_PB";
    constexpr auto kExtBAuto = "AB_VC_OVH_ELEC_EXTB_AUTO";
    constexpr auto kExtAOn = "AB_VC_OVH_ELEC_EXTA_ON";
    constexpr auto kExtBOn = "AB_VC_OVH_ELEC_EXTB_ON";
    constexpr auto kApuAvail = "AB_VC_OVH_APU_START_AVAIL";

    constexpr double kEmptyWeightKg = 185178.0;

    constexpr auto kMcduMenuKey = "AB_MCDU3_MENU";
    constexpr auto kMcduAtsuKey = "AB_MCDU3_LSK6L";
    constexpr auto kMcduAocMenuKey = "AB_MCDU3_LSK1R";
    constexpr auto kMcduFlightInitKey = "AB_MCDU3_LSK1L";

    constexpr auto kCouatlStarted = gsx::lvars::kCouatlStarted;
    constexpr auto kGsxLoaderFront = gsx::lvars::kBaggageLoaderFrontState;
    constexpr auto kGsxLoaderRear = gsx::lvars::kBaggageLoaderRearState;
    constexpr auto kCargoDoorModeFwd = "TLS_CARGO_DOOR_MODE_FWD";
    constexpr auto kCargoDoorModeAft = "TLS_CARGO_DOOR_MODE_AFT";
    constexpr auto kStairsFront = gsx::lvars::kPassengerStairsFrontState;
    constexpr auto kStairsMiddle = gsx::lvars::kPassengerStairsMiddleState;
    constexpr auto kStairsRear = gsx::lvars::kPassengerStairsRearState;
    constexpr auto kPaxDoorMode1L = "TLS_PAX_DOOR_MODE_1L";
    constexpr auto kPaxDoorMode2L = "TLS_PAX_DOOR_MODE_2L";
    constexpr auto kPaxDoorMode3L = "TLS_PAX_DOOR_MODE_3L";
    constexpr auto kPaxDoorMode4L = "TLS_PAX_DOOR_MODE_4L";
    constexpr auto kGsxCateringFront = gsx::lvars::kCateringFrontState;
    constexpr auto kGsxCateringRear = gsx::lvars::kCateringRearState;
    constexpr auto kPaxDoorMode1R = "TLS_PAX_DOOR_MODE_1R";
    constexpr auto kPaxDoorMode2R = "TLS_PAX_DOOR_MODE_2R";
    constexpr auto kPaxDoorMode3R = "TLS_PAX_DOOR_MODE_3R";
    constexpr auto kPaxDoorMode4R = "TLS_PAX_DOOR_MODE_4R";
    constexpr auto kGsxJetway = gsx::lvars::kJetway;
}

class TolissA340Test final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsNameAndVariant();
    static void readsCurrentFuelFromSim();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwDoesNotDropBelowEmptyWeight();
    static void emptyZfwReadsSimEmptyWeight();
    static void plannedValuesComeFromSession();
    static void flightPlanLoadedWhenSessionReady();
    static void fuelSetterWritesNothing();
    static void uplinkPressesMcduKeysInSequence();
    static void uplinkWaitsForPower();
    static void uplinkFiresOnApuPowerAlone();
    static void uplinkFiresImmediatelyWhenAlreadyPowered();
    static void uplinkRunsOncePerTrigger();
    static void uplinkIdleWithoutTrigger();
    static void zfwSetterWritesNothing();
    static void registersSmartSwitchForFastRefresh();
    static void smartSwitchFiresOnceAndResetsTheSwitch();
    static void smartSwitchStaysQuietUntilDataArrives();
    static void powerIgnoresBatteriesAlone();
    static void powerFollowsApuAvailability();
    static void powerRequiresEnergizedExternalSource();
    static void powerFollowsExternalOnAnnunciator();
    static void engineRunningDetectsAnyOfFourEngines();
    static void engineAssumedRunningUntilFuelFlowDataArrives();
    static void parkingBrakeFollowsEitherSource();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
    static void doorsUntouchedByDefaultWhenGsxAvailable();
    static void cargoDoorsOpenPerLoaderAndCloseWhenDone();
    static void cargoDoorsUntouchedWithoutGsx();
    static void paxDoorsOpenPerStairsAndCloseWhenGone();
    static void paxDoorsOpenOnlyAtFinalPosition();
    static void jetwayOpensOnlyDoor1L();
    static void jetwayAndStairsEitherHoldsDoor1LOpen();
    static void closeAllDoorsForcesEveryDoorClosed();
    static void stairsReopenDoorAfterCloseAllDoors();
    static void paxDoorsUntouchedWithoutGsx();
    static void cateringDoorsOpenWhenVehicleWaitsAndCloseWhenFinished();
    static void cateringDoorsUntouchedBeforeWaitingState();
    static void cateringDoorsUntouchedWithoutGsx();
    static void reportsLoadMethods();
};

void TolissA340Test::reportsNameAndVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 passenger(&gateway, &status, false);
    const TolissA340 freighter(&gateway, &status, true);

    QCOMPARE(QString(passenger.GetName()), QString("ToLiss A340-600"));
    QVERIFY(!passenger.IsCargoVariant());
    QVERIFY(freighter.IsCargoVariant());
}

void TolissA340Test::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.avars[kSimFuelTotalKg] = 41300.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 41300.0);
}

void TolissA340Test::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    gateway.avars[kSimTotalWeight] = 260000.0;
    gateway.avars[kSimFuelTotalKg] = 40000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 220000.0);
}

void TolissA340Test::currentZfwDoesNotDropBelowEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    gateway.avars[kSimTotalWeight] = 190000.0;
    gateway.avars[kSimFuelTotalKg] = 40000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), kEmptyWeightKg);
}

void TolissA340Test::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;

    QCOMPARE(aircraft.GetEmptyZfwKg(), kEmptyWeightKg);
}

void TolissA340Test::plannedValuesComeFromSession()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    status.plannedFuelKg = 52000.0;
    status.plannedZfwKg = 230000.0;
    status.plannedPassengers = 326;

    QCOMPARE(aircraft.GetPlannedFuelKg(), 52000.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 230000.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 326);
}

void TolissA340Test::flightPlanLoadedWhenSessionReady()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    status.flightPlanStatus = FlightPlanStatus::Idle;

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.flightPlanStatus = FlightPlanStatus::Ready;

    QVERIFY(aircraft.IsFlightPlanLoaded());
}

void TolissA340Test::fuelSetterWritesNothing()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    aircraft.SetCurrentFuelKg(30000.0);

    QCOMPARE(gateway.setAVarCalls, 0);
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::uplinkPressesMcduKeysInSequence()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kExtAPb] = 1.0;
    gateway.lvars[kExtAAuto] = 10.0;

    aircraft.OnLoadingStarted();

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduMenuKey], 1.0);
    QCOMPARE(gateway.setLVarCalls, 1);

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduAtsuKey], 1.0);

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduAocMenuKey], 1.0);

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduFlightInitKey], 1.0);
    QCOMPARE(gateway.setLVarCalls, 4);
}

void TolissA340Test::uplinkWaitsForPower()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kBattery1] = 1.0;

    aircraft.OnLoadingStarted();
    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    gateway.lvars[kExtAPb] = 1.0;
    gateway.lvars[kExtAAuto] = 10.0;

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduMenuKey], 1.0);
}

void TolissA340Test::uplinkFiresOnApuPowerAlone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kApuAvail] = 10.0;

    aircraft.OnLoadingStarted();

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduMenuKey], 1.0);
}

void TolissA340Test::uplinkFiresImmediatelyWhenAlreadyPowered()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kExtAPb] = 1.0;
    gateway.lvars[kExtAAuto] = 10.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QCOMPARE(gateway.setLVarCalls, 0);

    aircraft.OnLoadingStarted();

    aircraft.OnTick();

    QCOMPARE(gateway.lvars[kMcduMenuKey], 1.0);
}

void TolissA340Test::uplinkRunsOncePerTrigger()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kExtAPb] = 1.0;
    gateway.lvars[kExtAAuto] = 10.0;

    aircraft.OnLoadingStarted();
    for (int tick = 0; tick < 10; ++tick)
    {
        aircraft.OnTick();
    }

    QCOMPARE(gateway.setLVarCalls, 4);

    aircraft.OnLoadingStarted();
    for (int tick = 0; tick < 10; ++tick)
    {
        aircraft.OnTick();
    }

    QCOMPARE(gateway.setLVarCalls, 8);
}

void TolissA340Test::uplinkIdleWithoutTrigger()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    for (int tick = 0; tick < 20; ++tick)
    {
        aircraft.OnTick();
    }

    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::zfwSetterWritesNothing()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
    aircraft.SetCurrentZfwKg(230000.0);

    QCOMPARE(gateway.setAVarCalls, 0);
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::registersSmartSwitchForFastRefresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    QCOMPARE(gateway.fastRefreshNames.size(), static_cast<std::size_t>(1));
    QVERIFY(gateway.fastRefreshNames.front() == std::string("L:") + kSmartSwitch);
}

void TolissA340Test::smartSwitchFiresOnceAndResetsTheSwitch()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kSmartSwitch] = 1.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.lvars[kSmartSwitch], 1.0);

    gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.lvars[kSmartSwitch], 1.0);

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.lvars[kSmartSwitch], 1.0);
    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 0.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
}

void TolissA340Test::smartSwitchStaysQuietUntilDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::powerIgnoresBatteriesAlone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kBattery1] = 1.0;
    gateway.lvars[kBattery2] = 1.0;

    QVERIFY(!aircraft.IsPowered());
}

void TolissA340Test::powerFollowsApuAvailability()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kApuAvail] = 0.0;

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kApuAvail] = 10.0;

    QVERIFY(aircraft.IsPowered());
}

void TolissA340Test::powerRequiresEnergizedExternalSource()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kExtAPb] = 1.0;

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kExtAAuto] = 10.0;

    QVERIFY(aircraft.IsPowered());

    gateway.lvars[kExtAAuto] = 0.0;
    gateway.lvars[kExtBPb] = 1.0;
    gateway.lvars[kExtBAuto] = 10.0;

    QVERIFY(aircraft.IsPowered());

    gateway.lvars[kExtBPb] = 0.0;

    QVERIFY(!aircraft.IsPowered());
}

void TolissA340Test::powerFollowsExternalOnAnnunciator()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kExtAOn] = 10.0;

    QVERIFY(aircraft.IsPowered());

    gateway.lvars[kExtAOn] = 0.0;
    gateway.lvars[kExtBOn] = 10.0;

    QVERIFY(aircraft.IsPowered());

    gateway.lvars[kExtBOn] = 0.0;

    QVERIFY(!aircraft.IsPowered());
}

void TolissA340Test::engineRunningDetectsAnyOfFourEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    for (int engine = 1; engine <= 4; ++engine)
    {
        gateway.lvars["TLS_ENG" + std::to_string(engine) + "_FUEL_FLOW"] = 0.0;
    }

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.lvars["TLS_ENG4_FUEL_FLOW"] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void TolissA340Test::engineAssumedRunningUntilFuelFlowDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    QVERIFY(aircraft.IsEngineRunning());
}

void TolissA340Test::parkingBrakeFollowsEitherSource()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.avars[kSimParkingBrake] = 1.0;

    QVERIFY(aircraft.IsParkingBrakeSet());

    gateway.avars[kSimParkingBrake] = 0.0;
    gateway.lvars[kParkingBrakeLvar] = 100.0;

    QVERIFY(aircraft.IsParkingBrakeSet());

    gateway.lvars[kParkingBrakeLvar] = 0.0;

    QVERIFY(!aircraft.IsParkingBrakeSet());
}

void TolissA340Test::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kApuAvail] = 10.0;
    gateway.avars[kSimBeaconLight] = 1.0;
    for (int engine = 1; engine <= 4; ++engine)
    {
        gateway.lvars["TLS_ENG" + std::to_string(engine) + "_FUEL_FLOW"] = 0.0;
    }

    QVERIFY(aircraft.IsReadyToPush());

    gateway.avars[kSimBeaconLight] = 0.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.avars[kSimBeaconLight] = 1.0;
    gateway.lvars["TLS_ENG1_FUEL_FLOW"] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.lvars["TLS_ENG1_FUEL_FLOW"] = 0.0;
    gateway.lvars[kApuAvail] = 0.0;

    QVERIFY(!aircraft.IsReadyToPush());
}

void TolissA340Test::readyToDeboardFollowsSafetyState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    for (int engine = 1; engine <= 4; ++engine)
    {
        gateway.lvars["TLS_ENG" + std::to_string(engine) + "_FUEL_FLOW"] = 0.0;
    }
    gateway.avars[kSimParkingBrake] = 1.0;
    gateway.avars[kSimBeaconLight] = 0.0;

    QVERIFY(aircraft.IsReadyToDeboard());

    gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());

    gateway.avars[kSimBeaconLight] = 0.0;
    gateway.avars[kSimParkingBrake] = 0.0;

    QVERIFY(!aircraft.IsReadyToDeboard());

    gateway.avars[kSimParkingBrake] = 1.0;
    gateway.lvars["TLS_ENG2_FUEL_FLOW"] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());
}

void TolissA340Test::doorsUntouchedByDefaultWhenGsxAvailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    aircraft.OnTick();

    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::cargoDoorsOpenPerLoaderAndCloseWhenDone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kGsxLoaderFront] = 6.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoorModeFwd), 2.0);
    QCOMPARE(gateway.Written(kCargoDoorModeAft), -1.0);

    gateway.lvars[kGsxLoaderRear] = 8.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoorModeAft), 2.0);

    const int callsAfterOpen = gateway.setLVarCalls;
    gateway.lvars[kGsxLoaderFront] = 9.0;
    aircraft.OnTick();
    gateway.lvars[kGsxLoaderFront] = 4.0;
    aircraft.OnTick();

    QCOMPARE(gateway.setLVarCalls, callsAfterOpen);
    QCOMPARE(gateway.Written(kCargoDoorModeFwd), 2.0);

    gateway.lvars[kGsxLoaderFront] = 1.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoorModeFwd), 0.0);
}

void TolissA340Test::cargoDoorsUntouchedWithoutGsx()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kGsxLoaderFront] = 6.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QVERIFY(!gateway.HasReceivedLVar(kCargoDoorModeFwd));
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::paxDoorsOpenPerStairsAndCloseWhenGone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode2L));
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode4L));

    gateway.lvars[kStairsMiddle] = 3.0;
    gateway.lvars[kStairsRear] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode2L), 2.0);
    QCOMPARE(gateway.Written(kPaxDoorMode4L), 2.0);

    gateway.lvars[kStairsFront] = 2.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 0.0);
}

void TolissA340Test::paxDoorsOpenOnlyAtFinalPosition()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kStairsFront] = 5.0;
    aircraft.OnTick();

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode1L));

    gateway.lvars[kStairsFront] = 6.0;
    aircraft.OnTick();

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode1L));

    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);
}

void TolissA340Test::jetwayOpensOnlyDoor1L()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kGsxJetway] = 5.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode2L));
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode4L));

    gateway.lvars[kGsxJetway] = 4.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 0.0);
}

void TolissA340Test::jetwayAndStairsEitherHoldsDoor1LOpen()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kGsxJetway] = 5.0;
    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);

    const int callsAfterOpen = gateway.setLVarCalls;
    gateway.lvars[kGsxJetway] = 4.0;
    aircraft.OnTick();
    gateway.lvars[kGsxJetway] = 5.0;
    gateway.lvars[kStairsFront] = 2.0;
    aircraft.OnTick();

    QCOMPARE(gateway.setLVarCalls, callsAfterOpen);
    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);

    gateway.lvars[kGsxJetway] = 4.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 0.0);
}

void TolissA340Test::closeAllDoorsForcesEveryDoorClosed()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    aircraft.CloseAllDoors();

    for (const auto* door : {kPaxDoorMode1L, kPaxDoorMode2L, kPaxDoorMode3L, kPaxDoorMode4L,
                             kPaxDoorMode1R, kPaxDoorMode2R, kPaxDoorMode3R, kPaxDoorMode4R,
                             kCargoDoorModeFwd, kCargoDoorModeAft})
    {
        QCOMPARE(gateway.Written(door), 0.0);
    }
}

void TolissA340Test::stairsReopenDoorAfterCloseAllDoors()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);

    aircraft.CloseAllDoors();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 0.0);

    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1L), 2.0);
}

void TolissA340Test::paxDoorsUntouchedWithoutGsx()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kStairsFront] = 3.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode1L));
}

void TolissA340Test::cateringDoorsOpenWhenVehicleWaitsAndCloseWhenFinished()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kGsxCateringFront] = 6.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1R), 2.0);
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode4R));

    const int callsAfterOpen = gateway.setLVarCalls;
    gateway.lvars[kGsxCateringFront] = 7.0;
    aircraft.OnTick();

    QCOMPARE(gateway.setLVarCalls, callsAfterOpen);
    QCOMPARE(gateway.Written(kPaxDoorMode1R), 2.0);

    gateway.lvars[kGsxCateringFront] = 8.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode1R), 0.0);

    gateway.lvars[kGsxCateringRear] = 6.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode4R), 2.0);

    gateway.lvars[kGsxCateringRear] = 4.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoorMode4R), 0.0);
}

void TolissA340Test::cateringDoorsUntouchedBeforeWaitingState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kGsxCateringFront] = 2.0;
    aircraft.OnTick();
    gateway.lvars[kGsxCateringFront] = 5.0;
    aircraft.OnTick();

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode1R));
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode4R));
}

void TolissA340Test::cateringDoorsUntouchedWithoutGsx()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TolissA340 aircraft(&gateway, &status, false);

    gateway.lvars[kGsxCateringFront] = 6.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoorMode1R));
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TolissA340Test::reportsLoadMethods()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TolissA340 aircraft(&gateway, &status, false);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Self);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Self);
    QVERIFY(aircraft.SupportsStairsOrJetways());
}

QTEST_APPLESS_MAIN(TolissA340Test)

#include "tst_toliss_a340.moc"
