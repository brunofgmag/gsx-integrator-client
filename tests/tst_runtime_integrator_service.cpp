#include <string>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "doubles/FakeSimConnectApi.h"
#include "../src/application/IntegratorRuntime.h"
#include "../src/domain/turnaround/PilotTouch.h"
#include "../src/application/RuntimeIntegratorService.h"

namespace
{
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
    static void connectedCommandsFollowGuardOrder();
    static void subscribeFailureDisconnects();
    static void subscribeFailureKeepsRetrying();
    static void simulatorQuitRearmsTheReconnect();
    static void aTouchWithoutTheFlowRunningIsRefused();
    static void aTouchStampedWithAnotherPhaseIsRefusedAndNothingMoves();
    static void aTouchStampedWithAPhaseThatTakesNoneIsRefused();
    static void aTouchStampedWithTheCurrentPhaseReachesTheFlow();
};

void RuntimeIntegratorServiceTest::init()
{
    FakeSimConnectApi::Reset();
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

QTEST_GUILESS_MAIN(RuntimeIntegratorServiceTest)

#include "tst_runtime_integrator_service.moc"
