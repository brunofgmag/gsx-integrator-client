#include <QtTest/QTest>

#include "doubles/FakeVariableGateway.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>
#include <windows.h>
#include <SimConnect.h>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "ProbeLines.h"
#include "doubles/FakeAircraft.h"
#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/gsx/GsxLVars.h"
#include "../src/infrastructure/probe/ProbeObserver.h"
#include "../src/infrastructure/simconnect/SimConnectVariableGateway.h"

namespace
{
    constexpr auto kSimAVarsLog = "sim-avars.log";
    constexpr auto kGsxLVarsLog = "gsx-lvars.log";
    constexpr int kPastTheChangeFloorMs = 2100;

    constexpr std::array kGsxStateLVars = {
        gsx::lvars::kBoardingState,
        gsx::lvars::kDeboardingState,
        gsx::lvars::kRefuelingState,
        gsx::lvars::kPushbackStatus,
        gsx::lvars::kDeiceState,
        gsx::lvars::kFuelHoseConnected,
        gsx::lvars::kFuelCounter,
        gsx::lvars::kNumPassengersBoardingTotal,
        gsx::lvars::kNumPassengersDeboardingTotal,
        gsx::lvars::kBoardingCargoPercent,
        gsx::lvars::kDeboardingCargoPercent,
        gsx::lvars::kGpuConnected,
        gsx::lvars::kGpuState,
        gsx::lvars::kJetway,
        gsx::lvars::kStairs
    };

    std::string LVarDatum(const char* name)
    {
        return std::string("L:") + name;
    }

    void Deliver(SimConnectVariableGateway& gateway, const char* lvar, const double value)
    {
        std::vector<BYTE> buffer(sizeof(SIMCONNECT_RECV_SIMOBJECT_DATA) + sizeof(double), 0);
        auto* const data = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(buffer.data());
        data->dwRequestID = FakeSimConnectApi::DefineIdOf(LVarDatum(lvar));
        std::memcpy(&data->dwData, &value, sizeof(double));
        gateway.HandleSimObjectData(data);
    }

    bool IsRegistered(const char* lvar)
    {
        return FakeSimConnectApi::DefineIdOf(LVarDatum(lvar)) != 0;
    }

    void SeatTheAircraftAtTheGate(FakeVariableGateway& variables)
    {
        variables.avars = {
            {"FUEL TOTAL QUANTITY WEIGHT", 16660.9519456707},
            {"TOTAL WEIGHT", 61234.5},
            {"EMPTY WEIGHT", 42600.0},
            {"SIM ON GROUND", 1.0},
            {"GROUND VELOCITY", 7.90856993580502e-09},
            {"BRAKE PARKING POSITION", 1.0},
            {"LIGHT BEACON", 0.0},
            {"ENG COMBUSTION:1", 0.0},
            {"ENG COMBUSTION:2", 0.0},
            {"ENG COMBUSTION:3", 0.0},
            {"ENG COMBUSTION:4", 0.0}
        };
    }
}

class ProbeObserverTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    static void init();

    static void theSimVariablesThePredicatesReadLandInTheSimLog();
    static void theSignaturesRoundAwayJitterWhileTheTextKeepsTwoMoreDecimals();
    static void noGsxVariableIsRegisteredBeforeCouatlStarts();
    static void everyGsxVariableIsRegisteredOnceCouatlStarts();
    static void theGsxStateLandsInTheGsxLog();

private:
    QTemporaryDir directory_;
};

void ProbeObserverTest::initTestCase()
{
    qunsetenv("GSXI_PROBE");
    qunsetenv("GSXI_PROBE_WATCH");

    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void ProbeObserverTest::init()
{
#ifndef NDEBUG
    probe::ResetChangeMemoForTest();
    FakeSimConnectApi::Reset();
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeObserverTest::theSimVariablesThePredicatesReadLandInTheSimLog()
{
    FakeVariableGateway variables;
    SeatTheAircraftAtTheGate(variables);
    const FakeAircraft aircraft;
    ProbeObserver observer;
    const qsizetype before = ProbeLines(kSimAVarsLog).size();

    observer.Observe(aircraft, variables, {});

    QCOMPARE(ProbeLines(kSimAVarsLog).mid(before),
             (QStringList{
                 QStringLiteral("avar  FUEL TOTAL QUANTITY WEIGHT=16660.95 unit=kg"),
                 QStringLiteral("avar  TOTAL WEIGHT=61234.50 unit=kg"),
                 QStringLiteral("avar  EMPTY WEIGHT=42600.00 unit=kg"),
                 QStringLiteral("avar  SIM ON GROUND=1 unit=Bool"),
                 QStringLiteral("avar  GROUND VELOCITY=0.000 unit=Knots"),
                 QStringLiteral("avar  BRAKE PARKING POSITION=1 unit=Bool"),
                 QStringLiteral("avar  LIGHT BEACON=0 unit=Bool"),
                 QStringLiteral("avar  ENG COMBUSTION:1=0 unit=Bool"),
                 QStringLiteral("avar  ENG COMBUSTION:2=0 unit=Bool"),
                 QStringLiteral("avar  ENG COMBUSTION:3=0 unit=Bool"),
                 QStringLiteral("avar  ENG COMBUSTION:4=0 unit=Bool"),
                 QStringLiteral("avar  EXTERNAL POWER ON:1=pending unit=Bool")
             }));
}

void ProbeObserverTest::theSignaturesRoundAwayJitterWhileTheTextKeepsTwoMoreDecimals()
{
    FakeVariableGateway variables;
    SeatTheAircraftAtTheGate(variables);
    variables.lvars = {
        {gsx::lvars::kCouatlStarted, 1.0},
        {gsx::lvars::kBoardingState, 5.0},
        {gsx::lvars::kFuelCounter, 1234.56}
    };
    const FakeAircraft aircraft;
    ProbeObserver observer;
    observer.Observe(aircraft, variables, {});
    const qsizetype simBefore = ProbeLines(kSimAVarsLog).size();
    const qsizetype gsxBefore = ProbeLines(kGsxLVarsLog).size();

    variables.avars["FUEL TOTAL QUANTITY WEIGHT"] = 16660.7;
    variables.avars["TOTAL WEIGHT"] = 61234.9;
    variables.avars["GROUND VELOCITY"] = -0.04;
    variables.avars["EMPTY WEIGHT"] = 42601.0;
    variables.avars["LIGHT BEACON"] = 1.0;
    variables.lvars[gsx::lvars::kFuelCounter] = 1234.9;
    variables.lvars[gsx::lvars::kBoardingState] = 6.0;
    QTest::qWait(kPastTheChangeFloorMs);
    observer.Observe(aircraft, variables, {});

    QCOMPARE(ProbeLines(kSimAVarsLog).mid(simBefore),
             (QStringList{
                 QStringLiteral("avar  EMPTY WEIGHT=42601.00 unit=kg"),
                 QStringLiteral("avar  LIGHT BEACON=1 unit=Bool")
             }));
    QCOMPARE(ProbeLines(kGsxLVarsLog).mid(gsxBefore),
             QStringList{QStringLiteral("gsx   FSDT_GSX_BOARDING_STATE=6")});

    variables.avars["GROUND VELOCITY"] = 0.3;
    QTest::qWait(kPastTheChangeFloorMs);
    observer.Observe(aircraft, variables, {});

    QCOMPARE(ProbeLines(kSimAVarsLog).mid(simBefore + 2),
             QStringList{QStringLiteral("avar  GROUND VELOCITY=0.300 unit=Knots")});
}

void ProbeObserverTest::noGsxVariableIsRegisteredBeforeCouatlStarts()
{
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));
    const FakeAircraft aircraft;
    const qsizetype before = ProbeLines(kGsxLVarsLog).size();

    ProbeObserver beforeCouatlAnswers;
    beforeCouatlAnswers.Observe(aircraft, gateway, {});

    QVERIFY(IsRegistered(gsx::lvars::kCouatlStarted));
    QVERIFY(std::ranges::none_of(kGsxStateLVars, IsRegistered));

    Deliver(gateway, gsx::lvars::kCouatlStarted, 0.0);
    ProbeObserver whileCouatlIsDown;
    whileCouatlIsDown.Observe(aircraft, gateway, {});

    QVERIFY(std::ranges::none_of(kGsxStateLVars, IsRegistered));
    QCOMPARE(ProbeLines(kGsxLVarsLog).size(), before);
}

void ProbeObserverTest::everyGsxVariableIsRegisteredOnceCouatlStarts()
{
    SimConnectVariableGateway gateway;
    gateway.Attach(reinterpret_cast<HANDLE>(0x5150));
    const FakeAircraft aircraft;
    gateway.GetLVar(gsx::lvars::kCouatlStarted, 0.0);
    Deliver(gateway, gsx::lvars::kCouatlStarted, 1.0);

    ProbeObserver observer;
    observer.Observe(aircraft, gateway, {});

    QVERIFY(std::ranges::all_of(kGsxStateLVars, IsRegistered));
}

void ProbeObserverTest::theGsxStateLandsInTheGsxLog()
{
    FakeVariableGateway variables;
    variables.lvars = {
        {gsx::lvars::kCouatlStarted, 1.0},
        {gsx::lvars::kBoardingState, 5.0},
        {gsx::lvars::kRefuelingState, 6.0},
        {gsx::lvars::kPushbackStatus, 0.0},
        {gsx::lvars::kDeiceState, 1.0},
        {gsx::lvars::kFuelHoseConnected, 1.0},
        {gsx::lvars::kFuelCounter, 1234.56},
        {gsx::lvars::kNumPassengersBoardingTotal, 87.0},
        {gsx::lvars::kNumPassengersDeboardingTotal, 0.0},
        {gsx::lvars::kBoardingCargoPercent, 42.5},
        {gsx::lvars::kDeboardingCargoPercent, 0.0},
        {gsx::lvars::kGpuConnected, 1.0},
        {gsx::lvars::kGpuState, 5.0},
        {gsx::lvars::kJetway, 5.0},
        {gsx::lvars::kStairs, 1.0}
    };
    const FakeAircraft aircraft;
    ProbeObserver observer;
    const qsizetype before = ProbeLines(kGsxLVarsLog).size();

    observer.Observe(aircraft, variables, {});

    QCOMPARE(ProbeLines(kGsxLVarsLog).mid(before),
             (QStringList{
                 QStringLiteral("gsx   FSDT_GSX_BOARDING_STATE=5"),
                 QStringLiteral("gsx   FSDT_GSX_DEBOARDING_STATE=pending"),
                 QStringLiteral("gsx   FSDT_GSX_REFUELING_STATE=6"),
                 QStringLiteral("gsx   FSDT_GSX_PUSHBACK_STATUS=0"),
                 QStringLiteral("gsx   FSDT_GSX_DEICE_STATE=1"),
                 QStringLiteral("gsx   FSDT_GSX_FUELHOSE_CONNECTED=1"),
                 QStringLiteral("gsx   FSDT_GSX_FUEL_COUNTER=1234.56"),
                 QStringLiteral("gsx   FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL=87"),
                 QStringLiteral("gsx   FSDT_GSX_NUMPASSENGERS_DEBOARDING_TOTAL=0"),
                 QStringLiteral("gsx   FSDT_GSX_BOARDING_CARGO_PERCENT=42.5"),
                 QStringLiteral("gsx   FSDT_GSX_DEBOARDING_CARGO_PERCENT=0"),
                 QStringLiteral("gsx   FSDT_GSX_GPU_CONNECTED=1"),
                 QStringLiteral("gsx   FSDT_GSX_GPU_STATE=5"),
                 QStringLiteral("gsx   FSDT_GSX_JETWAY=5"),
                 QStringLiteral("gsx   FSDT_GSX_STAIRS=1")
             }));
}

QTEST_GUILESS_MAIN(ProbeObserverTest)

#include "tst_probe_observer.moc"
