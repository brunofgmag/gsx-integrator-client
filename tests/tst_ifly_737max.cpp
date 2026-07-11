#include <QtTest/QTest>

#include <array>
#include <cmath>
#include <string>
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/infrastructure/aircraft/IFly737Max.h"

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

    constexpr double kEmptyOperatingZfwKg = 45070.0;
}

class IFly737MaxTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsNameAndVariant();
    static void reportsSupportedProgressiveModes();
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
    static void parkingBrakeRequiresSwitchAndSimBrake();
    static void readyToDeboardFollowsSafetyState();
    static void aircraftPowerFollowsAvionicsBus();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void engineRunningDetectsAnyCombustion();
    static void engineAssumedRunningUntilCombustionDataArrives();
};

void IFly737MaxTest::reportsNameAndVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const IFly737Max aircraft(&gateway, &status);

    QCOMPARE(QString(aircraft.GetName()), QString("iFly 737 MAX 8"));
    QVERIFY(!aircraft.IsCargoVariant());
}

void IFly737MaxTest::reportsSupportedProgressiveModes()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    IFly737Max aircraft(&gateway, &status);

    QVERIFY(aircraft.SupportsProgressiveFuel());
    QVERIFY(aircraft.SupportsProgressiveLoad());
    QVERIFY(aircraft.SupportsStairsOrJetways());
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
    QVERIFY(gateway.fastRefreshNames.front() == std::string("L:") + kSmartSwitch);
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
    aircraft.OnSlowTick();

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

    QVERIFY(std::abs(paxB - 20000.0 * 5250.0 / 34070.0) < 0.001);
    QVERIFY(std::abs(cargoAft - 20000.0 * 8018.0 / 34070.0) < 0.001);
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

void IFly737MaxTest::parkingBrakeRequiresSwitchAndSimBrake()
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
        TestCase{"switch only", 1.0, 0.0, false},
        TestCase{"sim brake only", 0.0, 1.0, false},
        TestCase{"both", 1.0, 1.0, true},
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
        TestCase{"dark", 0.0, false},
        TestCase{"powered", 28.0, true},
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
        TestCase{"ready", 28.0, 1.0, 0.0, true},
        TestCase{"dark", 0.0, 1.0, 0.0, false},
        TestCase{"beacon off", 28.0, 0.0, 0.0, false},
        TestCase{"engine running", 28.0, 1.0, 1.0, false},
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
        TestCase{"stopped", 0.0, 0.0, false},
        TestCase{"engine 1", 1.0, 0.0, true},
        TestCase{"engine 2", 0.0, 1.0, true},
        TestCase{"both", 1.0, 1.0, true},
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

QTEST_APPLESS_MAIN(IFly737MaxTest)

#include "tst_ifly_737max.moc"
