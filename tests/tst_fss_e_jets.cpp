#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "AircraftTicks.h"
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/model/FlightPlan.h"
#include "../src/domain/support/Weight.h"
#include "../src/infrastructure/aircraft/fss/FssEJet.h"

namespace
{
    constexpr auto kAcPowerAvailable = "FSS_EXX_ELEC_PWR_AC_AVAIL";
    constexpr auto kEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kParkBrakeLever = "FSS_EXX_PARKBRAKE_BV_LEVER";
    constexpr auto kBeaconSwitch = "FSS_EXX_OVHD_EXLT_RED_BCN_SWITCH";
    constexpr auto kCallRampLeft = "FSS_EXX_AUDIO_L_TEL_RAMP_BTN";
    constexpr auto kCallRampRight = "FSS_EXX_AUDIO_R_TEL_RAMP_BTN";
    constexpr auto kGpuState = "FSS_EXX_EXT_GPU_STATE";
    constexpr auto kBoolUnit = "Bool";

    constexpr auto kChocksF = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_F";
    constexpr auto kChocksL = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_L";
    constexpr auto kChocksR = "FSS_EXX_GNDOBJ_WHEEL_CHOKE_R";
    constexpr std::array kChocks = {kChocksF, kChocksL, kChocksR};

    constexpr auto kAutoDeparture = "FSS_GNDSVC_AUTO_DEPARTURE";
    constexpr auto kAutoDeboarding = "FSS_GNDSVC_AUTO_DEBOARDING";
    constexpr auto kAutoGoodEngstart = "FSS_GNDSVC_AUTO_NOTIFY_GOOD_ENGSTART";
    constexpr std::array kAutomationLVars = {kAutoDeparture, kAutoDeboarding, kAutoGoodEngstart};
    constexpr auto kEnableGsxSupport = "FSS_ENABLE_GSX_SUPPORT";

    constexpr std::array kOwnGroundEquipment = {
        "FSS_EXX_GNDOBJ_CONE_ENG_L", "FSS_EXX_GNDOBJ_CONE_ENG_R", "FSS_EXX_GNDOBJ_CONE_ENTRY",
        "FSS_EXX_GNDOBJ_CONE_TAIL", "FSS_EXX_GNDOBJ_CONE_WING_L", "FSS_EXX_GNDOBJ_CONE_WING_R",
        "FSS_EXX_GNDOBJ_ENG_COVER_L", "FSS_EXX_GNDOBJ_ENG_COVER_R",
        "FSS_EXX_WHEEL_COVER_L", "FSS_EXX_WHEEL_COVER_R",
        "FSS_EXX_SAFETY_PIN_LDGGEAR_L", "FSS_EXX_SAFETY_PIN_LDGGEAR_R", "FSS_EXX_SAFETY_PIN_PITOT_BOTTOM",
        "FSS_EXX_SAFETY_PIN_PITOT_F_L", "FSS_EXX_SAFETY_PIN_PITOT_F_R",
        "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_L", "FSS_EXX_SAFETY_PIN_PITOT_F_TOP_R",
        "FSS_EXX_STAIR_FWD_L_ACTIVE", "FSS_EXX_STAIR_AFT_L_ACTIVE"
    };

    constexpr double kGpuHidden = -1.0;
    constexpr double kGpuInactive = 0.0;
    constexpr double kGpuRequested = 1.0;
    constexpr double kGpuAvailable = 2.0;
    constexpr double kGpuStarting = 3.0;
    constexpr double kGpuStopping = 4.0;
    constexpr double kGpuFeeding = 5.0;

    constexpr int kFiftyTicks = 50;
    constexpr int kTwentyTicks = 20;
    constexpr int kTicksToWaitForTheEcho = 5;
    constexpr auto kAutomationRule = "fss-ejet-keep-vendor-automation-off";
    constexpr auto kGpuRule = "fss-ejet-gpu-follows-request";

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

    void AllEnginesStopped(FakeVariableGateway& gateway)
    {
        gateway.avars[kEng1Combustion] = 0.0;
        gateway.avars[kEng2Combustion] = 0.0;
    }

    void TickTimes(Aircraft& aircraft, FakeVariableGateway& gateway, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            TickAircraft(aircraft, gateway);
        }
    }
}

class FssEJetTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsTheCargoVariantItWasBuiltWith();
    static void logsTheProfileNameWithTheFreighterSuffix();
    static void readsThePlanFromTheClientOfp();
    static void writesKgAsTheNativeUnit();
    static void loadsAndRefuelsThroughTheClient();
    static void energizedFollowsAcAvailable();
    static void engineRunningFollowsCombustion();
    static void defaultsHoldTheStateUntilDataArrives();
    static void parkingBrakeFollowsTheLeverOnly();
    static void heldInPlaceAcceptsTheLeverOrTheChocks();
    static void readyToPushFollowsPowerBeaconAndEngines();
    static void readyToDeboardFollowsSafetyState();
    static void chocksWriteTheSameValueToTheThree();
    static void chocksReadIsAnyOfTheThree();
    static void clearsTheNineteenOwnGroundEquipmentLVarsOnce();
    static void smartSwitchFiresOnceAndWritesBackToZero();
    static void theRightCallRampAlsoFiresOnceAndWritesBackToZero();
    static void aCallRampLeftOffNeverFiresNorWrites();
    static void groundPowerStatusReadsTheThreeSettledValues();
    static void pulsesOnceWhenDisconnectedAndRequestedOn();
    static void pulsesNothingWhenTheStatusAlreadyMatchesTheRequest();
    static void neverPulsesWhileTheStateIsInTransit();
    static void stopsRequestingOnceTheStateSettlesOnTheTarget();
    static void gpuRuleNeverHoldsThePhase();
    static void vendorAutomationTurnsOffTheThreeKeysOnce();
    static void vendorAutomationRewritesAKeyThatDriftsBackOn();
    static void vendorAutomationNeverTouchesEnableGsxSupport();
    static void vendorAutomationRuleNeverHoldsThePhase();
    static void observingEvaluatingAndReadingWriteNoVariable();
};

void FssEJetTest::reportsTheCargoVariantItWasBuiltWith()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    const FssEJet passenger(&gateway, &status, FssEJet::kNameE190, false);
    const FssEJet freighter(&gateway, &status, FssEJet::kNameE190, true);

    QVERIFY(!passenger.IsCargoVariant());
    QVERIFY(freighter.IsCargoVariant());
}

void FssEJetTest::logsTheProfileNameWithTheFreighterSuffix()
{
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        const LogCapture log;
        const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

        QVERIFY(LogCapture::Contains("Profile loaded: FSS Embraer E190"));
        QVERIFY(!LogCapture::Contains("Profile loaded: FSS Embraer E190 Freighter"));
    }
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        const LogCapture log;
        const FssEJet aircraft(&gateway, &status, FssEJet::kNameE195, true);

        QVERIFY(LogCapture::Contains("Profile loaded: FSS Embraer E195 Freighter"));
    }
}

void FssEJetTest::readsThePlanFromTheClientOfp()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.RequiresEfbFlightPlan());
    QVERIFY(!aircraft.IsFlightPlanLoaded());

    status.flightPlanStatus = FlightPlanStatus::Ready;
    status.plannedFuelKg = 6531.0;
    status.plannedZfwKg = 41000.0;
    status.plannedPassengers = 88;

    QVERIFY(aircraft.IsFlightPlanLoaded());
    QCOMPARE(aircraft.GetPlannedFuelKg(), 6531.0);
    QCOMPARE(aircraft.GetPlannedZfwKg(), 41000.0);
    QCOMPARE(aircraft.GetPlannedPassengers(), 88);
}

void FssEJetTest::writesKgAsTheNativeUnit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetNativeWeightUnit() == WeightUnit::Kg);
}

void FssEJetTest::loadsAndRefuelsThroughTheClient()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetRefuelMethod() == RefuelBy::Client);
    QVERIFY(aircraft.GetBoardMethod() == BoardBy::Client);
    QVERIFY(aircraft.SupportsStairsOrJetways());
    QVERIFY(!aircraft.CompletesPushbackViaInterruptMenu());
    QVERIFY(!aircraft.TakesExternalPowerAtTheEngineerPanel());
}

void FssEJetTest::energizedFollowsAcAvailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kAcPowerAvailable] = 0.0;

    QVERIFY(!aircraft.IsPowered());

    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(aircraft.IsPowered());
}

void FssEJetTest::engineRunningFollowsCombustion()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);

    QVERIFY(!aircraft.IsEngineRunning());

    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(aircraft.IsEngineRunning());
}

void FssEJetTest::defaultsHoldTheStateUntilDataArrives()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsPowered());
    QVERIFY(aircraft.IsEngineRunning());
    QVERIFY(!aircraft.IsParkingBrakeSet());
    QVERIFY(!aircraft.IsHeldInPlace());
    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);
}

void FssEJetTest::parkingBrakeFollowsTheLeverOnly()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.avars[kParkBrakeLever] = 1.0;

    QVERIFY(!aircraft.IsParkingBrakeSet());

    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsParkingBrakeSet());
}

void FssEJetTest::heldInPlaceAcceptsTheLeverOrTheChocks()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());

    gateway.lvars[kParkBrakeLever] = 0.0;
    gateway.lvars[kChocksR] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
}

void FssEJetTest::readyToPushFollowsPowerBeaconAndEngines()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kAcPowerAvailable] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());

    gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(aircraft.IsReadyToPush());

    gateway.avars[kEng1Combustion] = 1.0;

    QVERIFY(!aircraft.IsReadyToPush());
}

void FssEJetTest::readyToDeboardFollowsSafetyState()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AllEnginesStopped(gateway);
    gateway.lvars[kParkBrakeLever] = 1.0;

    QVERIFY(aircraft.IsReadyToDeboard());

    gateway.lvars[kBeaconSwitch] = 1.0;

    QVERIFY(!aircraft.IsReadyToDeboard());
}

void FssEJetTest::chocksWriteTheSameValueToTheThree()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.SetChocks(true));

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.Written(lVar), 1.0);
    }

    QVERIFY(aircraft.SetChocks(false));

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.Written(lVar), 0.0);
    }
}

void FssEJetTest::chocksReadIsAnyOfTheThree()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(!aircraft.IsHeldInPlace());

    gateway.lvars[kChocksF] = 0.0;
    gateway.lvars[kChocksL] = 0.0;
    gateway.lvars[kChocksR] = 1.0;

    QVERIFY(aircraft.IsHeldInPlace());
}

void FssEJetTest::clearsTheNineteenOwnGroundEquipmentLVarsOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

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

    for (const char* lVar : kChocks)
    {
        QCOMPARE(gateway.WriteCount(lVar), 0);
    }
}

void FssEJetTest::smartSwitchFiresOnceAndWritesBackToZero()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampLeft), 1);
    QCOMPARE(gateway.Written(kCallRampLeft), 0.0);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampLeft), 1);
}

void FssEJetTest::theRightCallRampAlsoFiresOnceAndWritesBackToZero()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampRight] = LVarSpan{0.0, 1.0, true};

    QVERIFY(aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.WriteCount(kCallRampRight), 1);
    QCOMPARE(gateway.Written(kCallRampRight), 0.0);

    for (int tick = 0; tick < kTwentyTicks; ++tick)
    {
        gateway.lvarSpans[kCallRampRight] = LVarSpan{0.0, 0.0, true};

        QVERIFY(!aircraft.ConsumeSmartSwitch());
    }

    QCOMPARE(gateway.WriteCount(kCallRampRight), 1);
}

void FssEJetTest::aCallRampLeftOffNeverFiresNorWrites()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());

    gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};

    QVERIFY(!aircraft.ConsumeSmartSwitch());
    QCOMPARE(gateway.setLVarCalls, 0);
}

void FssEJetTest::groundPowerStatusReadsTheThreeSettledValues()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = kGpuInactive;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = kGpuHidden;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = kGpuFeeding;

    QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Connected);

    for (const double transit : {kGpuRequested, kGpuAvailable, kGpuStarting, kGpuStopping})
    {
        gateway.lvars[kGpuState] = transit;

        QVERIFY(aircraft.GetGroundPowerStatus() == GroundPowerStatus::Unknown);
    }
}

void FssEJetTest::pulsesOnceWhenDisconnectedAndRequestedOn()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuInactive;
    aircraft.SetGroundPower(true);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);
}

void FssEJetTest::pulsesNothingWhenTheStatusAlreadyMatchesTheRequest()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuFeeding;
    aircraft.SetGroundPower(true);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 0);
}

void FssEJetTest::neverPulsesWhileTheStateIsInTransit()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuAvailable;
    aircraft.SetGroundPower(true);

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 0);
}

void FssEJetTest::stopsRequestingOnceTheStateSettlesOnTheTarget()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    gateway.lvars[kGpuState] = kGpuInactive;
    aircraft.SetGroundPower(true);

    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);

    gateway.lvars[kGpuState] = kGpuFeeding;

    TickTimes(aircraft, gateway, kTwentyTicks);

    QCOMPARE(gateway.WriteCount("FSS_EXX_TOGGLE_CGPU"), 1);
}

void FssEJetTest::gpuRuleNeverHoldsThePhase()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AircraftRule* const rule = FindRule(aircraft, kGpuRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void FssEJetTest::vendorAutomationTurnsOffTheThreeKeysOnce()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickAircraft(aircraft, gateway);

    for (const char* lVar : kAutomationLVars)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
        QCOMPARE(gateway.Written(lVar), 0.0);
    }

    TickTimes(aircraft, gateway, kFiftyTicks - 1);

    for (const char* lVar : kAutomationLVars)
    {
        QCOMPARE(gateway.WriteCount(lVar), 1);
    }
}

void FssEJetTest::vendorAutomationRewritesAKeyThatDriftsBackOn()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickTimes(aircraft, gateway, kTicksToWaitForTheEcho + 2);

    QCOMPARE(gateway.WriteCount(kAutoDeparture), 1);

    gateway.lvars[kAutoDeparture] = 1.0;
    TickAircraft(aircraft, gateway);

    QCOMPARE(gateway.WriteCount(kAutoDeparture), 2);
    QCOMPARE(gateway.Written(kAutoDeparture), 0.0);
}

void FssEJetTest::vendorAutomationNeverTouchesEnableGsxSupport()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    TickTimes(aircraft, gateway, kFiftyTicks);

    QCOMPARE(gateway.WriteCount(kEnableGsxSupport), 0);
}

void FssEJetTest::vendorAutomationRuleNeverHoldsThePhase()
{
    FakeVariableGateway gateway;
    AutomationStatus status;
    const FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, false);

    AircraftRule* const rule = FindRule(aircraft, kAutomationRule);

    QVERIFY(rule != nullptr);

    for (const RuleContext& context : {RuleContext{}, kLoading, kPassengerAccess})
    {
        QVERIFY(!rule->Evaluate(context).holds);
    }
}

void FssEJetTest::observingEvaluatingAndReadingWriteNoVariable()
{
    for (const bool cargo : {false, true})
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        FssEJet aircraft(&gateway, &status, FssEJet::kNameE190, cargo);

        QVERIFY(!aircraft.Rules().empty());

        AllEnginesStopped(gateway);
        gateway.lvars[kAcPowerAvailable] = 1.0;
        gateway.lvars[kParkBrakeLever] = 1.0;
        gateway.lvars[kChocksF] = 1.0;
        gateway.lvars[kGpuState] = kGpuFeeding;
        gateway.lvarSpans[kCallRampLeft] = LVarSpan{0.0, 0.0, true};
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
        static_cast<void>(aircraft.IsFlightPlanLoaded());
        static_cast<void>(aircraft.GetPlannedZfwKg());
        static_cast<void>(aircraft.GetPlannedFuelKg());
        static_cast<void>(aircraft.GetPlannedPassengers());
        static_cast<void>(aircraft.GetEmptyZfwKg());
        static_cast<void>(aircraft.GetCurrentFuelKg());
        static_cast<void>(aircraft.GetCurrentZfwKg());
        static_cast<void>(aircraft.GetGroundPowerStatus());
        static_cast<void>(aircraft.ConsumeSmartSwitch());

        aircraft.OnLoadingStarted();

        QCOMPARE(gateway.setLVarCalls, 0);
        QCOMPARE(gateway.setAVarCalls, 0);
    }
}

QTEST_APPLESS_MAIN(FssEJetTest)

#include "tst_fss_e_jets.moc"
