#include <QtTest/QTest>

#include <array>
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/domain/support/Weight.h"
#include "../src/infrastructure/aircraft/TfdiMd11.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSmartSwitch = "MD11_PED_CPT_AUDIO_PNL_INT_RADIO_SW";

    constexpr auto kSimBeacon = "LIGHT BEACON";
    constexpr auto kParkingBrake = "MD11_THR_PARK_LVR";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr auto kChocks = "MD11_EXT_CHOCKS";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kSimEng3Combustion = "ENG COMBUSTION:3";

    constexpr auto kEfbGw = "MD11_EFB_PAYLOAD_GW";
    constexpr auto kEfbZfw = "MD11_EFB_PAYLOAD_ZFW";
    constexpr auto kEfbPayload = "MD11_EFB_PAYLOAD_PAYLOAD";
    constexpr auto kEfbFuel = "MD11_EFB_PAYLOAD_FUEL";
    constexpr auto kEfbLoad = "MD11_EFB_PAYLOAD_LOAD";
    constexpr auto kEfbReadReady = "MD11_EFB_READ_READY";

    constexpr auto kPowerOn = "MD11_CABIN_POWER";
    constexpr auto kBattery = "MD11_OVHD_ELEC_BATT_BT";
    constexpr auto kApu = "MD11_OVHD_ELEC_APU_PWR_ON_LT";
    constexpr auto kExtPower = "MD11_OVHD_ELEC_EXT_PWR_ON_LT";
    constexpr auto kExtGpu = "MD11_EXT_GPU";

    constexpr auto kCouatlStarted = gsx::lvars::kCouatlStarted;
    constexpr auto kGsxLoaderFront = gsx::lvars::kBaggageLoaderFrontState;
    constexpr auto kGsxLoaderRear = gsx::lvars::kBaggageLoaderRearState;
    constexpr auto kGsxLoaderMain = gsx::lvars::kBaggageLoaderMainState;
    constexpr auto kStairsFront = gsx::lvars::kPassengerStairsFrontState;
    constexpr auto kStairsMiddle = gsx::lvars::kPassengerStairsMiddleState;
    constexpr auto kStairsRear = gsx::lvars::kPassengerStairsRearState;
    constexpr auto kPaxDoor1L = "MD11_EXT_DOOR_CMD_PAX_1L";
    constexpr auto kPaxDoor2L = "MD11_EXT_DOOR_CMD_PAX_2L";
    constexpr auto kPaxDoor4L = "MD11_EXT_DOOR_CMD_PAX_4L";
    constexpr auto kCargoDoor1R = "MD11_EXT_DOOR_CMD_CARGO1R";
    constexpr auto kCargoDoor2R = "MD11_EXT_DOOR_CMD_CARGO2R";
    constexpr auto kCargoDoorMain = "MD11_EXT_DOOR_CMD_CARGO_MAIN";

    constexpr double kEmptyOperatingZfwKg = 130000.0;
    constexpr double kMtowKg = 283730.0;

    double LoadPercent(const double payloadKg, const double emptyZfwKg)
    {
        return payloadKg / (kMtowKg - emptyZfwKg) * 100.0;
    }
}

class TfdiMd11Test final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsNameAndVariant();
    static void readsCurrentFuelFromSim();
    static void currentZfwSubtractsFuelFromTotalWeight();
    static void currentZfwDoesNotDropBelowEmptyWeight();
    static void plannedValuesComeFromSession();
    static void flightPlanLoadedWhenSessionReady();
    static void smartSwitchConsumesAndClearsLVar();
    static void smartSwitchRegistersFastRefresh();
    static void smartSwitchReassertsResetWhileSimKeepsOldValue();
    static void smartSwitchReportsSinglePressPerActivation();
    static void emptyZfwReadsSimEmptyWeight();
    static void parkingBrakeRequiresLeverAndSimBrake();
    static void readyToDeboardFollowsSafetyState();
    static void slowTickWithoutPendingCommitDoesNothing();
    static void commitWritesEfbTargetsInPounds();
    static void commitClampsEfbTargetsBeforeWriting();
    static void commitSetsReadReadyMask();
    static void seedsUnsetFuelTargetFromReceivedSimValue();
    static void doesNotSeedFuelTargetBeforeSimDataArrives();
    static void aircraftPowerFollowsElectricalState();
    static void groundPowerStatusFollowsExtGpuLVar();
    static void setChocksWritesChocksLVar();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void engineRunningDetectsAnyCombustion();
    static void engineAssumedRunningUntilCombustionDataArrives();
    static void cargoDoorsClosedByDefaultWhenGsxAvailable();
    static void cargoDoorsOpenPerLoaderAndCloseWhenDone();
    static void mainCargoDoorOpensOnlyOnFreighter();
    static void cargoDoorsUntouchedWithoutGsx();
    static void paxDoorsOpenPerStairsAndCloseWhenGone();
    static void paxDoorsOpenOnlyAtFinalPosition();
    static void paxDoorsUntouchedWithoutGsx();
    static void reportsLoadMethods();
};

void TfdiMd11Test::reportsNameAndVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    const TfdiMd11 passenger(&gateway, &status, false);
    const TfdiMd11 freighter(&gateway, &status, true);

    QCOMPARE(QString(passenger.GetName()), QString("TFDi MD-11"));
    QVERIFY(!passenger.IsCargoVariant());
    QVERIFY(freighter.IsCargoVariant());
}

void TfdiMd11Test::readsCurrentFuelFromSim()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimFuelTotalKg] = 18500.0;

    QCOMPARE(aircraft.GetCurrentFuelKg(), 18500.0);
}

void TfdiMd11Test::currentZfwSubtractsFuelFromTotalWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 200000.0;
    gateway.avars[kSimFuelTotalKg] = 20000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), 180000.0);
}

void TfdiMd11Test::currentZfwDoesNotDropBelowEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 100000.0;

    QCOMPARE(aircraft.GetCurrentZfwKg(), kEmptyOperatingZfwKg);
}

void TfdiMd11Test::plannedValuesComeFromSession()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    status.plannedFuelKg = 12000.0;
    status.plannedZfwKg = 175000.0;
    status.plannedPassengers = 205;

    QCOMPARE(aircraft.GetPlannedFuelKg(), 12000.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 175000.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 205);
}

void TfdiMd11Test::flightPlanLoadedWhenSessionReady()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.flightPlanStatus = FlightPlanStatus::Ready;

    QVERIFY(aircraft.IsFlightPlanLoaded());
}

void TfdiMd11Test::smartSwitchConsumesAndClearsLVar()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.Written(kSmartSwitch), 1.0);
    QVERIFY(!aircraft.ConsumeSmartSwitch());
}

void TfdiMd11Test::smartSwitchRegistersFastRefresh()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    QCOMPARE(gateway.fastRefreshNames.size(), static_cast<std::size_t>(1));
    QVERIFY(gateway.fastRefreshNames.front() == std::string("L:") + kSmartSwitch);
}

void TfdiMd11Test::smartSwitchReassertsResetWhileSimKeepsOldValue()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 1);

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 2);
    QCOMPARE(gateway.Written(kSmartSwitch), 1.0);

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 3);
}

void TfdiMd11Test::smartSwitchReportsSinglePressPerActivation()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvars[kSmartSwitch] = 2.0;

    QVERIFY(aircraft.ConsumeSmartSwitch());
}

void TfdiMd11Test::emptyZfwReadsSimEmptyWeight()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = 128500.0;

    QCOMPARE(aircraft.GetEmptyZfwKg(), 128500.0);
}

void TfdiMd11Test::parkingBrakeRequiresLeverAndSimBrake()
{
    struct TestCase
    {
        const char* name;
        double lever;
        double simBrake;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{"released", 0.0, 0.0, false},
        TestCase{"lever only", 1.0, 0.0, false},
        TestCase{"sim brake only", 0.0, 1.0, false},
        TestCase{"both", 1.0, 1.0, true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        TfdiMd11 aircraft(&gateway, &status, false);

        gateway.lvars[kParkingBrake] = testCase.lever;
        gateway.avars[kSimParkingBrake] = testCase.simBrake;

        QVERIFY2(aircraft.IsParkingBrakeSet() == testCase.expected, testCase.name);
    }
}

void TfdiMd11Test::readyToDeboardFollowsSafetyState()
{
    struct TestCase
    {
        const char* name;
        double parkingBrake;
        double chocks;
        double beacon;
        double engineCombustion;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{"brake set", 1.0, 0.0, 0.0, 0.0, true},
        TestCase{"chocks set", 0.0, 1.0, 0.0, 0.0, true},
        TestCase{"engine running", 1.0, 0.0, 0.0, 1.0, false},
        TestCase{"beacon on", 1.0, 0.0, 1.0, 0.0, false},
        TestCase{"brake released", 0.0, 0.0, 0.0, 0.0, false},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        TfdiMd11 aircraft(&gateway, &status, false);

        gateway.lvars[kParkingBrake] = testCase.parkingBrake;
        gateway.avars[kSimParkingBrake] = testCase.parkingBrake;
        gateway.lvars[kChocks] = testCase.chocks;
        gateway.avars[kSimBeacon] = testCase.beacon;
        gateway.avars[kSimEng1Combustion] = 0.0;
        gateway.avars[kSimEng2Combustion] = testCase.engineCombustion;
        gateway.avars[kSimEng3Combustion] = 0.0;

        QVERIFY2(aircraft.IsReadyToDeboard() == testCase.expected, testCase.name);
    }
}

void TfdiMd11Test::slowTickWithoutPendingCommitDoesNothing()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    aircraft.OnSlowTick();

    QCOMPARE(gateway.setLVarCalls, 0);
}

void TfdiMd11Test::commitWritesEfbTargetsInPounds()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentFuelKg(20000.0);
    aircraft.SetCurrentZfwKg(160000.0);
    aircraft.OnSlowTick();

    constexpr double zfw = 160000.0;
    constexpr double fuel = 20000.0;
    constexpr double payload = zfw - kEmptyOperatingZfwKg;
    constexpr double grossWeight = zfw + fuel;
    const double loadPct = LoadPercent(payload, kEmptyOperatingZfwKg);

    QVERIFY(qFuzzyCompare(gateway.Written(kEfbGw), weight::KgToLb(grossWeight)));
    QVERIFY(qFuzzyCompare(gateway.Written(kEfbZfw), weight::KgToLb(zfw)));
    QVERIFY(qFuzzyCompare(gateway.Written(kEfbPayload), weight::KgToLb(payload)));
    QVERIFY(qFuzzyCompare(gateway.Written(kEfbFuel), weight::KgToLb(fuel)));
    QVERIFY(qFuzzyCompare(gateway.Written(kEfbLoad), loadPct));

    const int callsAfterCommit = gateway.setLVarCalls;
    aircraft.OnSlowTick();

    QCOMPARE(gateway.setLVarCalls, callsAfterCommit);
}

void TfdiMd11Test::commitClampsEfbTargetsBeforeWriting()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    aircraft.SetCurrentFuelKg(-500.0);
    aircraft.SetCurrentZfwKg(100000.0);
    aircraft.OnSlowTick();

    QCOMPARE(gateway.Written(kEfbGw), weight::KgToLb(kEmptyOperatingZfwKg));
    QCOMPARE(gateway.Written(kEfbZfw), weight::KgToLb(kEmptyOperatingZfwKg));
    QCOMPARE(gateway.Written(kEfbPayload), 0.0);
    QCOMPARE(gateway.Written(kEfbFuel), 0.0);
    QCOMPARE(gateway.Written(kEfbLoad), 0.0);
}

void TfdiMd11Test::commitSetsReadReadyMask()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kEfbReadReady] = 2.0;
    aircraft.SetCurrentFuelKg(15000.0);
    aircraft.OnSlowTick();

    QCOMPARE(gateway.Written(kEfbReadReady), 3.0);
}

void TfdiMd11Test::seedsUnsetFuelTargetFromReceivedSimValue()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 200000.0;
    gateway.avars[kSimFuelTotalKg] = 18500.0;
    aircraft.SetCurrentZfwKg(160000.0);
    aircraft.OnSlowTick();

    QVERIFY(qFuzzyCompare(gateway.Written(kEfbFuel), weight::KgToLb(18500.0)));
}

void TfdiMd11Test::doesNotSeedFuelTargetBeforeSimDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.avars[kSimEmptyWeight] = kEmptyOperatingZfwKg;
    gateway.avars[kSimTotalWeight] = 200000.0;
    aircraft.SetCurrentZfwKg(160000.0);
    aircraft.OnSlowTick();

    QCOMPARE(gateway.Written(kEfbFuel), 0.0);
}

void TfdiMd11Test::aircraftPowerFollowsElectricalState()
{
    struct TestCase
    {
        const char* name;
        double cabinPower;
        double battery;
        double externalPower;
        double apu;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{"dark", 0.0, 0.0, 0.0, 0.0, false},
        TestCase{"cabin", 1.0, 0.0, 0.0, 0.0, true},
        TestCase{"battery only", 0.0, 1.0, 0.0, 0.0, false},
        TestCase{"battery external", 0.0, 1.0, 1.0, 0.0, true},
        TestCase{"battery apu", 0.0, 1.0, 0.0, 1.0, true},
        TestCase{"cabin battery no source", 1.0, 1.0, 0.0, 0.0, false},
        TestCase{"cabin battery external", 1.0, 1.0, 1.0, 0.0, true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        TfdiMd11 aircraft(&gateway, &status, false);

        gateway.lvars[kPowerOn] = testCase.cabinPower;
        gateway.lvars[kBattery] = testCase.battery;
        gateway.lvars[kExtPower] = testCase.externalPower;
        gateway.lvars[kApu] = testCase.apu;

        QVERIFY2(aircraft.IsPowered() == testCase.expected, testCase.name);
    }
}

void TfdiMd11Test::groundPowerStatusFollowsExtGpuLVar()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    QCOMPARE(aircraft.GetGroundPowerStatus(), std::optional{GroundPowerStatus::Unknown});

    gateway.lvars[kExtGpu] = 1.0;

    QCOMPARE(aircraft.GetGroundPowerStatus(), std::optional{GroundPowerStatus::Connected});

    gateway.lvars[kExtGpu] = 0.0;

    QCOMPARE(aircraft.GetGroundPowerStatus(), std::optional{GroundPowerStatus::Disconnected});
}

void TfdiMd11Test::setChocksWritesChocksLVar()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    QVERIFY(aircraft.SupportsChocksControl());

    aircraft.SetChocks(true);

    QCOMPARE(gateway.Written(kChocks), 1.0);

    aircraft.SetChocks(false);

    QCOMPARE(gateway.Written(kChocks), 0.0);
}

void TfdiMd11Test::readyToPushFollowsPowerBeaconAndEngines()
{
    struct TestCase
    {
        const char* name;
        double power;
        double beacon;
        double engineCombustion;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{"ready", 1.0, 1.0, 0.0, true},
        TestCase{"dark", 0.0, 1.0, 0.0, false},
        TestCase{"beacon off", 1.0, 0.0, 0.0, false},
        TestCase{"engine running", 1.0, 1.0, 1.0, false},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        TfdiMd11 aircraft(&gateway, &status, false);

        gateway.lvars[kPowerOn] = testCase.power;
        gateway.avars[kSimBeacon] = testCase.beacon;
        gateway.avars[kSimEng1Combustion] = 0.0;
        gateway.avars[kSimEng2Combustion] = testCase.engineCombustion;
        gateway.avars[kSimEng3Combustion] = 0.0;

        QVERIFY2(aircraft.IsReadyToPush() == testCase.expected, testCase.name);
    }
}

void TfdiMd11Test::engineRunningDetectsAnyCombustion()
{
    struct TestCase
    {
        const char* name;
        double eng1;
        double eng2;
        double eng3;
        bool expected;
    };

    constexpr auto cases = std::array{
        TestCase{"stopped", 0.0, 0.0, 0.0, false},
        TestCase{"engine 1", 1.0, 0.0, 0.0, true},
        TestCase{"engine 2", 0.0, 1.0, 0.0, true},
        TestCase{"engine 3", 0.0, 0.0, 1.0, true},
    };

    for (const auto& testCase : cases)
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        TfdiMd11 aircraft(&gateway, &status, false);

        gateway.avars[kSimEng1Combustion] = testCase.eng1;
        gateway.avars[kSimEng2Combustion] = testCase.eng2;
        gateway.avars[kSimEng3Combustion] = testCase.eng3;

        QVERIFY2(aircraft.IsEngineRunning() == testCase.expected, testCase.name);
    }
}

void TfdiMd11Test::engineAssumedRunningUntilCombustionDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    QVERIFY(aircraft.IsEngineRunning());

    gateway.avars[kSimEng1Combustion] = 0.0;
    gateway.avars[kSimEng2Combustion] = 0.0;

    QVERIFY(aircraft.IsEngineRunning());

    gateway.avars[kSimEng3Combustion] = 0.0;

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kSimEng3Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void TfdiMd11Test::cargoDoorsClosedByDefaultWhenGsxAvailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoor1R), 0.0);
    QCOMPARE(gateway.Written(kCargoDoor2R), 0.0);
}

void TfdiMd11Test::cargoDoorsOpenPerLoaderAndCloseWhenDone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kGsxLoaderFront] = 6.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoor1R), 100.0);
    QCOMPARE(gateway.Written(kCargoDoor2R), 0.0);

    gateway.lvars[kGsxLoaderRear] = 8.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoor2R), 100.0);

    const int callsAfterOpen = gateway.setLVarCalls;
    gateway.lvars[kGsxLoaderFront] = 9.0;
    aircraft.OnTick();
    gateway.lvars[kGsxLoaderFront] = 4.0;
    aircraft.OnTick();

    QCOMPARE(gateway.setLVarCalls, callsAfterOpen);
    QCOMPARE(gateway.Written(kCargoDoor1R), 100.0);

    gateway.lvars[kGsxLoaderFront] = 1.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kCargoDoor1R), 0.0);
}

void TfdiMd11Test::mainCargoDoorOpensOnlyOnFreighter()
{
    FakeVariableGateway passengerGateway;
    AutomationStatus passengerStatus;
    TfdiMd11 passenger(&passengerGateway, &passengerStatus, false);

    passengerGateway.lvars[kCouatlStarted] = 1.0;
    passengerGateway.lvars[kGsxLoaderMain] = 6.0;
    passenger.OnTick();

    QVERIFY(!passengerGateway.HasReceivedLVar(kCargoDoorMain));

    FakeVariableGateway freighterGateway;
    AutomationStatus freighterStatus;
    TfdiMd11 freighter(&freighterGateway, &freighterStatus, true);

    freighterGateway.lvars[kCouatlStarted] = 1.0;
    freighterGateway.lvars[kGsxLoaderMain] = 6.0;
    freighter.OnTick();

    QCOMPARE(freighterGateway.Written(kCargoDoorMain), 100.0);
}

void TfdiMd11Test::cargoDoorsUntouchedWithoutGsx()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kGsxLoaderFront] = 6.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QVERIFY(!gateway.HasReceivedLVar(kCargoDoor1R));
    QCOMPARE(gateway.setLVarCalls, 0);
}

void TfdiMd11Test::paxDoorsOpenPerStairsAndCloseWhenGone()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoor1L), 100.0);
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoor2L));
    QVERIFY(!gateway.HasReceivedLVar(kPaxDoor4L));

    gateway.lvars[kStairsMiddle] = 3.0;
    gateway.lvars[kStairsRear] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoor2L), 100.0);
    QCOMPARE(gateway.Written(kPaxDoor4L), 100.0);

    gateway.lvars[kStairsFront] = 2.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoor1L), 0.0);
}

void TfdiMd11Test::paxDoorsOpenOnlyAtFinalPosition()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kCouatlStarted] = 1.0;

    gateway.lvars[kStairsFront] = 5.0;
    aircraft.OnTick();

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoor1L));

    gateway.lvars[kStairsFront] = 6.0;
    aircraft.OnTick();

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoor1L));

    gateway.lvars[kStairsFront] = 3.0;
    aircraft.OnTick();

    QCOMPARE(gateway.Written(kPaxDoor1L), 100.0);
}

void TfdiMd11Test::paxDoorsUntouchedWithoutGsx()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    TfdiMd11 aircraft(&gateway, &status, false);

    gateway.lvars[kStairsFront] = 3.0;

    for (int tick = 0; tick < 5; ++tick)
    {
        aircraft.OnTick();
    }

    QVERIFY(!gateway.HasReceivedLVar(kPaxDoor1L));
}

void TfdiMd11Test::reportsLoadMethods()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const TfdiMd11 aircraft(&gateway, &status, false);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Self);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Self);
}

QTEST_APPLESS_MAIN(TfdiMd11Test)

#include "tst_tfdi_md11.moc"
