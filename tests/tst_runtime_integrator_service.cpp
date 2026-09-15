#include <algorithm>
#include <string>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "doubles/FakeGsxRemoteApiClient.h"
#include "doubles/FakeSimConnectApi.h"
#include "../src/application/IntegratorRuntime.h"
#include "../src/domain/turnaround/PilotTouch.h"
#include "../src/application/RuntimeIntegratorService.h"
#include "../src/infrastructure/gsx/GsxLVars.h"
#include "../src/infrastructure/simvars/SimVars.h"

namespace
{
    constexpr DWORD kOneSecondEvent = 1;
    constexpr DWORD kFourSecondEvent = 2;
    constexpr DWORD kPauseEvent = 6;
    constexpr DWORD kSimStateRequest = 0x0FFFFFFF;
    constexpr double kWorldMapCamera = 12.0;
    constexpr double kCockpitCamera = 2.0;
    constexpr auto kMsfs2024AppName = "SunRise";
    constexpr auto kTitleDatum = "TITLE";
    constexpr auto kMd11Title = "TFDi Design MD-11 PAX";
    constexpr auto kMd11ProfileId = "tfdi-md11";
    constexpr auto kMd11EfbZfw = "L:MD11_EFB_PAYLOAD_ZFW";
    constexpr double kMd11EmptyWeightKg = 150000.0;
    constexpr double kJetwayInPlace = 5.0;
    constexpr int kFlowTickBudget = 12;
    constexpr int kLoaderNoticeTickBudget = 120;

    void PushSimRunning(const int running)
    {
        SIMCONNECT_RECV_SYSTEM_STATE state{};
        state.dwRequestID = kSimStateRequest;
        state.dwInteger = running;
        FakeSimConnectApi::Push(state, SIMCONNECT_RECV_ID_SYSTEM_STATE);
    }

    void PushOneSecondTick()
    {
        SIMCONNECT_RECV_EVENT tick{};
        tick.uEventID = kOneSecondEvent;
        FakeSimConnectApi::Push(tick, SIMCONNECT_RECV_ID_EVENT);
    }

    void PushSimOpen(const char* appName)
    {
        SIMCONNECT_RECV_OPEN open{};
        strcpy_s(open.szApplicationName, appName);
        FakeSimConnectApi::Push(open, SIMCONNECT_RECV_ID_OPEN);
    }

    void PushUnpaused()
    {
        SIMCONNECT_RECV_EVENT pause{};
        pause.uEventID = kPauseEvent;
        pause.dwData = 0;
        FakeSimConnectApi::Push(pause, SIMCONNECT_RECV_ID_EVENT);
    }

    void PushFourSecondTick()
    {
        SIMCONNECT_RECV_EVENT tick{};
        tick.uEventID = kFourSecondEvent;
        FakeSimConnectApi::Push(tick, SIMCONNECT_RECV_ID_EVENT);
    }

    bool PushDatum(const std::string& datumName, const double value)
    {
        const DWORD defineId = FakeSimConnectApi::DefineIdOf(datumName);
        if (defineId == 0)
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectDouble(defineId, value);

        return true;
    }

    bool PushLVar(const std::string& name, const double value)
    {
        return PushDatum("L:" + name, value);
    }

    bool TickAndWait(QSignalSpy& updated)
    {
        PushOneSecondTick();

        return updated.wait(2000);
    }

    bool DispatchPending()
    {
        return QTest::qWaitFor([] { return FakeSimConnectApi::pendingMessages.empty(); }, 2000);
    }

    bool WasWritten(const std::string& datumName)
    {
        const DWORD defineId = FakeSimConnectApi::DefineIdOf(datumName);

        return defineId != 0
            && std::ranges::any_of(FakeSimConnectApi::writtenSimObjectData,
                                   [defineId](const auto& write) { return write.first == defineId; });
    }

    bool DetectTheMd11WithTheGsxUp(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        PushUnpaused();
        if (!TickAndWait(updated))
        {
            return false;
        }

        const DWORD title = FakeSimConnectApi::DefineIdOf(kTitleDatum);
        if (title == 0 || !PushLVar(gsx::lvars::kCouatlStarted, 1.0))
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectString(title, kMd11Title);

        return TickAndWait(updated) && runtime.GetAircraftProfileId() == kMd11ProfileId;
    }

    bool DriveTheFlowInto(const TurnaroundPhase phase, const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        for (int tick = 0; tick < kFlowTickBudget && runtime.GetPhase() != phase; ++tick)
        {
            for (const char* engine : {simvars::kSimEng1Combustion, simvars::kSimEng2Combustion,
                                       simvars::kSimEng3Combustion})
            {
                PushDatum(engine, 0.0);
            }

            if (!TickAndWait(updated))
            {
                return false;
            }
        }

        return runtime.GetPhase() == phase;
    }

    struct RecordingObserver final : IntegratorServiceObserver
    {
        int notifications = 0;

        void OnIntegratorStateChanged() override
        {
            ++notifications;
        }
    };
}

class RuntimeIntegratorServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void init();

    static void freshSnapshotHasDisconnectedDefaults();
    static void commandsFailWhileOffline();
    static void fixGsxProfileWithoutConflictFails();
    static void fixPmdgOptionsWithoutConflictFails();
    static void applySettingsPushesEffectiveSettings();
    static void observersAreDedupedAndNotified();
    static void automationToggleEmitsOncePerChange();
    static void runtimeGettersOnEmptyRuntime();
    static void setupConnectsThroughFakeSimConnect();
    static void setupStartsTheRemoteApiClientWithoutOpeningASocket();
    static void connectedCommandsFollowGuardOrder();
    static void subscribeFailureDisconnects();
    static void subscribeFailureKeepsRetrying();
    static void simulatorQuitRearmsTheReconnect();
    static void aTouchWithoutTheFlowRunningIsRefused();
    static void aTouchStampedWithAnotherPhaseIsRefusedAndNothingMoves();
    static void aTouchStampedWithAPhaseThatTakesNoneIsRefused();
    static void aTouchStampedWithTheCurrentPhaseReachesTheFlow();
    static void aWorldMapCameraDuringTheLoadDoesNotLeaveTheFlowOff();
    static void theGsxChipFollowsTheGsxWhileThePilotIsOnFoot();
    static void theSnapshotCountsTheJetwayWaitDownWithTheFlow();
    static void theSnapshotCarriesTheLoaderCountdownWhileTheLoaderHoldsBoarding();
    static void theSlowTickWritesNothingWhileTheGsxIsDown();
    static void theFuelWaitsUntilTheRemoteApiAnnouncesItsConnection();
};

void RuntimeIntegratorServiceTest::init()
{
    FakeSimConnectApi::Reset();
    FakeGsxRemoteApi::Reset();
}

void RuntimeIntegratorServiceTest::freshSnapshotHasDisconnectedDefaults()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    const IntegratorSnapshot snapshot = service.GetSnapshot();

    QVERIFY(!snapshot.connected);
    QVERIFY(!snapshot.sessionActive);
    QVERIFY(!snapshot.automationEnabled);
    QVERIFY(!snapshot.gsxAvailable);
    QVERIFY(!snapshot.aircraftSupported);
    QVERIFY(!snapshot.canToggleAutomation);
    QVERIFY(!snapshot.canStartLoading);
    QVERIFY(!snapshot.canReloadSimbrief);
    QVERIFY(!snapshot.refuelByGsx);
    QVERIFY(!snapshot.refuelBySelf);
    QVERIFY(!snapshot.gsxProfileConflict);
    QVERIFY(!snapshot.gsxProfileFixable);
    QVERIFY(!snapshot.pmdgOptionsConflict);
    QVERIFY(!snapshot.pmdgOptionsFixable);
    QVERIFY(!snapshot.cargoAircraft);
    QVERIFY(!snapshot.engineerPanelExternalPower);
    QCOMPARE(snapshot.aircraftName, std::string{});
    QCOMPARE(snapshot.aircraftProfileId, std::string{});
    QCOMPARE(snapshot.phase, TurnaroundPhase::WaitingSupportedAircraft);
    QCOMPARE(snapshot.flightPlanStatus, FlightPlanStatus::Idle);
    QCOMPARE(snapshot.fuelProgress, 0.0);
    QCOMPARE(snapshot.boardingProgress, 0.0);
    QCOMPARE(snapshot.deboardingProgress, 0.0);
    QCOMPARE(snapshot.plannedFuelKg, 0.0);
    QCOMPARE(snapshot.loadedFuelKg, 0.0);
    QCOMPARE(snapshot.plannedZfwKg, 0.0);
    QCOMPARE(snapshot.plannedPax, 0);
    QCOMPARE(snapshot.boardedPax, 0);
    QCOMPARE(snapshot.delayTicksRemaining, 0);
}

void RuntimeIntegratorServiceTest::commandsFailWhileOffline()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const std::string offline = "Simulator is offline.";

    const CommandResult automation = service.SetAutomationEnabled(true);

    QVERIFY(!automation.succeeded);
    QCOMPARE(automation.message, offline);

    const CommandResult loading = service.StartLoading();

    QVERIFY(!loading.succeeded);
    QCOMPARE(loading.message, offline);

    const CommandResult restart = service.RestartFlow();

    QVERIFY(!restart.succeeded);
    QCOMPARE(restart.message, offline);

    const CommandResult reload = service.ReloadSimbrief();

    QVERIFY(!reload.succeeded);
    QCOMPARE(reload.message, offline);

    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingSupportedAircraft);

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, offline);
}

void RuntimeIntegratorServiceTest::fixGsxProfileWithoutConflictFails()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const CommandResult result = service.FixGsxProfile();

    QVERIFY(!result.succeeded);
    QCOMPARE(result.message, std::string("The GSX profile does not need fixing."));
}

void RuntimeIntegratorServiceTest::fixPmdgOptionsWithoutConflictFails()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const CommandResult result = service.FixPmdgOptions();

    QVERIFY(!result.succeeded);
    QCOMPARE(result.message, std::string("The PMDG options file does not need fixing."));
}

void RuntimeIntegratorServiceTest::applySettingsPushesEffectiveSettings()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    AppSettings settings;
    settings.simbriefPilotId = 123;
    settings.fuelRateKgs = 7.5;
    settings.callCatering = true;

    service.ApplySettings(settings);

    QCOMPARE(runtime.Settings().simbriefPilotId, 123);
    QCOMPARE(runtime.Settings().fuelRateKgs, 7.5);
    QCOMPARE(runtime.Settings().callCatering, true);
}

void RuntimeIntegratorServiceTest::observersAreDedupedAndNotified()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    RecordingObserver first;
    RecordingObserver second;

    service.AddObserver(&first);
    service.AddObserver(&first);
    service.AddObserver(nullptr);
    service.AddObserver(&second);

    service.ApplySettings(AppSettings{});

    QCOMPARE(first.notifications, 1);
    QCOMPARE(second.notifications, 1);

    service.RemoveObserver(&second);

    service.ApplySettings(AppSettings{});

    QCOMPARE(first.notifications, 2);
    QCOMPARE(second.notifications, 1);
}

void RuntimeIntegratorServiceTest::automationToggleEmitsOncePerChange()
{
    IntegratorRuntime runtime;
    const QSignalSpy updates(&runtime, &IntegratorRuntime::Updated);

    runtime.SetAutomationEnabled(true);

    QCOMPARE(updates.count(), 1);

    runtime.SetAutomationEnabled(true);

    QCOMPARE(updates.count(), 1);

    runtime.SetAutomationEnabled(false);

    QCOMPARE(updates.count(), 2);
}

void RuntimeIntegratorServiceTest::runtimeGettersOnEmptyRuntime()
{
    IntegratorRuntime runtime;

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QVERIFY(snapshot.aircraftName.empty());
    QCOMPARE(snapshot.aircraftProfileId, std::string{});
    QVERIFY(!snapshot.refuelByGsx);
    QVERIFY(!snapshot.refuelBySelf);
    QVERIFY(!snapshot.cargoAircraft);
    QVERIFY(!snapshot.engineerPanelExternalPower);
    QVERIFY(!snapshot.gsxProfileConflict);
    QVERIFY(!snapshot.gsxProfileFixable);
    QVERIFY(!snapshot.pmdgOptionsConflict);
    QVERIFY(!snapshot.pmdgOptionsFixable);
    QCOMPARE(runtime.GetAircraftProfileId(), std::string{});
    QVERIFY(!runtime.HasGsxProfileConflict());
    QVERIFY(!runtime.FixGsxProfile());
    QVERIFY(!runtime.HasPmdgOptionsConflict());
    QVERIFY(!runtime.FixPmdgOptions());
    QVERIFY(!runtime.ReloadSimbrief());
}

void RuntimeIntegratorServiceTest::setupConnectsThroughFakeSimConnect()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    QVERIFY(runtime.IsConnected());

    const IntegratorSnapshot snapshot = service.GetSnapshot();

    QVERIFY(snapshot.connected);
    QVERIFY(snapshot.canToggleAutomation);
}

void RuntimeIntegratorServiceTest::setupStartsTheRemoteApiClientWithoutOpeningASocket()
{
    IntegratorRuntime runtime;

    QCOMPARE(FakeGsxRemoteApi::startCalls, 0);

    runtime.Setup();

    QCOMPARE(FakeGsxRemoteApi::startCalls, 1);
    QVERIFY(FakeGsxRemoteApi::commandVerbs.empty());
}

void RuntimeIntegratorServiceTest::connectedCommandsFollowGuardOrder()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    const CommandResult automation = service.SetAutomationEnabled(true);

    QVERIFY(automation.succeeded);
    QVERIFY(service.GetSnapshot().automationEnabled);

    const CommandResult loading = service.StartLoading();

    QVERIFY(!loading.succeeded);
    QCOMPARE(loading.message, std::string("The turnaround is not waiting to start loading."));

    const CommandResult reload = service.ReloadSimbrief();

    QVERIFY(!reload.succeeded);
    QCOMPARE(reload.message, std::string("Wait for an active flight session."));

    const CommandResult restart = service.RestartFlow();

    QVERIFY(restart.succeeded);
}

void RuntimeIntegratorServiceTest::subscribeFailureDisconnects()
{
    FakeSimConnectApi::subscribeSucceeds = false;

    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(!runtime.IsConnected());
}

void RuntimeIntegratorServiceTest::subscribeFailureKeepsRetrying()
{
    FakeSimConnectApi::subscribeSucceeds = false;

    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(!runtime.IsConnected());
    QVERIFY(runtime.IsReconnectPending());
}

void RuntimeIntegratorServiceTest::simulatorQuitRearmsTheReconnect()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(runtime.IsConnected());
    QVERIFY(!runtime.IsReconnectPending());

    QSignalSpy quits(&runtime, &IntegratorRuntime::SimulatorQuit);

    constexpr SIMCONNECT_RECV quit{};
    FakeSimConnectApi::Push(quit, SIMCONNECT_RECV_ID_QUIT);

    QVERIFY(quits.wait(2000));
    QVERIFY(!runtime.IsConnected());
    QVERIFY(runtime.IsReconnectPending());
}

void RuntimeIntegratorServiceTest::aTouchWithoutTheFlowRunningIsRefused()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    const CommandResult touch = service.AcceptPilotTouch(runtime.GetPhase());

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("Start the turnaround flow first."));
}

void RuntimeIntegratorServiceTest::aTouchStampedWithAnotherPhaseIsRefusedAndNothingMoves()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    const TurnaroundPhase before = runtime.GetPhase();
    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingNewFlight);

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("The turnaround moved on before your touch arrived."));
    QCOMPARE(runtime.GetPhase(), before);
}

void RuntimeIntegratorServiceTest::aTouchStampedWithAPhaseThatTakesNoneIsRefused()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    QVERIFY(!PilotTouch::Accepts(runtime.GetPhase()));

    const CommandResult touch = service.AcceptPilotTouch(runtime.GetPhase());

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("This step does not wait on the pilot."));
}

void RuntimeIntegratorServiceTest::aTouchStampedWithTheCurrentPhaseReachesTheFlow()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::WaitingNewFlight));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingNewFlight);

    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingNewFlight);

    QVERIFY(touch.succeeded);
    QVERIFY(touch.message.empty());
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::aWorldMapCameraDuringTheLoadDoesNotLeaveTheFlowOff()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    AutomationSettings settings;
    settings.autoStartFlow = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushSimRunning(1);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(runtime.IsSessionActive());
    QVERIFY(service.GetSnapshot().automationEnabled);

    const DWORD camera = FakeSimConnectApi::DefineIdOf("CAMERA STATE");
    QVERIFY(camera != 0);

    FakeSimConnectApi::PushSimObjectDouble(camera, kWorldMapCamera);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(!service.GetSnapshot().sessionReady);

    FakeSimConnectApi::PushSimObjectDouble(camera, kCockpitCamera);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(runtime.IsSessionActive());
    QVERIFY(service.GetSnapshot().sessionReady);
    QVERIFY(service.GetSnapshot().automationEnabled);
}

void RuntimeIntegratorServiceTest::theGsxChipFollowsTheGsxWhileThePilotIsOnFoot()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushSimOpen(kMsfs2024AppName);
    PushUnpaused();
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    const DWORD isAircraft = FakeSimConnectApi::DefineIdOf("IS AIRCRAFT");
    const DWORD isAvatar = FakeSimConnectApi::DefineIdOf("IS AVATAR");
    QVERIFY(isAircraft != 0);
    QVERIFY(isAvatar != 0);

    FakeSimConnectApi::PushSimObjectDouble(isAircraft, 1.0);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(service.GetSnapshot().sessionReady);

    const DWORD couatlStarted = FakeSimConnectApi::DefineIdOf("L:FSDT_GSX_COUATL_STARTED");
    QVERIFY(couatlStarted != 0);
    QVERIFY(!service.GetSnapshot().gsxAvailable);

    FakeSimConnectApi::PushSimObjectDouble(isAvatar, 1.0);
    FakeSimConnectApi::PushSimObjectDouble(couatlStarted, 1.0);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(!service.GetSnapshot().sessionReady);
    QVERIFY(service.GetSnapshot().pilotOnFoot);
    QVERIFY(service.GetSnapshot().gsxAvailable);
}

void RuntimeIntegratorServiceTest::theSnapshotCountsTheJetwayWaitDownWithTheFlow()
{
    IntegratorRuntime runtime;

    AutomationSettings settings;
    settings.skipReposition = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::CallServices, runtime, updated));

    QVERIFY(PushLVar(gsx::lvars::kJetway, static_cast<double>(GsxStateStatus::Requested)));
    QVERIFY(TickAndWait(updated));

    const int firstWaitSeconds = runtime.Snapshot().servicesWaitSeconds;
    QVERIFY(firstWaitSeconds > 0);

    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().servicesWaitSeconds, firstWaitSeconds - 1);
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheLoaderCountdownWhileTheLoaderHoldsBoarding()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::Boarding) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::Boarding);

    QVERIFY(PushLVar(gsx::lvars::kBoardingState, static_cast<double>(GsxStateStatus::Active)));
    QVERIFY(TickAndWait(updated));

    QVERIFY(PushLVar(gsx::lvars::kBaggageLoaderMainState, gsx::states::kLoaderWaitingForDoor));
    for (int tick = 0; tick < kLoaderNoticeTickBudget
         && runtime.Snapshot().loaderHoldingBoarding == CargoLoader::None; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QCOMPARE(snapshot.loaderHoldingBoarding, CargoLoader::MainDeck);
    QVERIFY(snapshot.loaderDoorWaitSeconds > 0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theSlowTickWritesNothingWhileTheGsxIsDown()
{
    IntegratorRuntime runtime;

    AutomationSettings settings;
    settings.skipReposition = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::CallServices, runtime, updated));
    QVERIFY(PushLVar(gsx::lvars::kJetway, kJetwayInPlace));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::WaitingFlightPlan, runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(PushDatum(simvars::kSimEmptyWeight, kMd11EmptyWeightKg));
    QVERIFY(TickAndWait(updated));

    FakeSimConnectApi::writtenSimObjectData.clear();

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 0.0));
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(WasWritten(kMd11EfbZfw));
}

void RuntimeIntegratorServiceTest::theFuelWaitsUntilTheRemoteApiAnnouncesItsConnection()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::Refueling) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::Refueling);

    QVERIFY(PushLVar(gsx::lvars::kRefuelingState, static_cast<double>(GsxStateStatus::Completed)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelProgress, 0.0);

    FakeGsxRemoteApi::AnnounceConnection(true);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelProgress, 100.0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(RuntimeIntegratorServiceTest)

#include "tst_runtime_integrator_service.moc"
