#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/viewmodel/OperationsViewModel.h"

class OperationsViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void exposesUpdatedSnapshot();
    static void emitsOneSignalForSnapshotChanges();
    static void doesNotEmitWhenSnapshotIsUnchanged();
    static void ignoresInsignificantFloatingPointChanges();
    static void reportsRejectedCommands();
    static void mapsFlightPlanStatusToText();
    static void simbriefReadyAndErrorFlags();
    static void noopWhenSettingSameEnabledValue();
    static void startLoadingDelegatesToService();
    static void startLoadingReportsRejectedCommands();
    static void exposesCanStartLoadingFromSnapshot();
    static void reloadSimbriefDelegatesToService();
    static void exposesAircraftPropertiesFromSnapshot();
    static void successfulCommandClearsPreviousError();
    static void exposesPhaseIndexCountAndTip();
    static void exposesGsxProfileConflictFromSnapshot();
    static void fixGsxProfileDelegatesToService();
    static void fixGsxProfileReportsRejectedCommands();
    static void restartFlowDelegatesToService();
    static void restartFlowReportsRejectedCommands();
    static void exposesInDeboardingPhaseFromSnapshot();
};

void OperationsViewModelTest::exposesUpdatedSnapshot()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    service.snapshot.connected = true;
    service.snapshot.phase = TurnaroundPhase::WaitingAircraftReady;
    service.Notify();

    QVERIFY(viewModel.IsConnected());
    QCOMPARE(viewModel.GetStateText(), QStringLiteral("Waiting for aircraft ready"));
}

void OperationsViewModelTest::emitsOneSignalForSnapshotChanges()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);
    const QSignalSpy spy(&viewModel, &OperationsViewModel::SnapshotChanged);

    service.snapshot.connected = true;
    service.snapshot.sessionActive = true;
    service.snapshot.plannedFuelKg = 120;
    service.Notify();

    QCOMPARE(spy.count(), 1);
}

void OperationsViewModelTest::doesNotEmitWhenSnapshotIsUnchanged()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);
    const QSignalSpy spy(&viewModel, &OperationsViewModel::SnapshotChanged);

    service.Notify();

    QCOMPARE(spy.count(), 0);
}

void OperationsViewModelTest::ignoresInsignificantFloatingPointChanges()
{
    FakeIntegratorService service;
    service.snapshot.fuelProgress = 10.0;

    const OperationsViewModel viewModel(&service);
    const QSignalSpy spy(&viewModel, &OperationsViewModel::SnapshotChanged);

    service.snapshot.fuelProgress = 10.00001;
    service.Notify();

    QCOMPARE(spy.count(), 0);
}

void OperationsViewModelTest::reportsRejectedCommands()
{
    FakeIntegratorService service;
    service.snapshot.connected = true;
    service.snapshot.canToggleAutomation = true;
    service.automationResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.startFlow();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(service.automationCalls, 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
    QVERIFY(!viewModel.IsEnabled());
}

void OperationsViewModelTest::mapsFlightPlanStatusToText()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    service.snapshot.flightPlanStatus = FlightPlanStatus::Idle;
    service.Notify();

    QCOMPARE(viewModel.GetSimbriefStatusText(), QStringLiteral("Inactive"));

    service.snapshot.flightPlanStatus = FlightPlanStatus::Fetching;
    service.Notify();

    QCOMPARE(viewModel.GetSimbriefStatusText(), QStringLiteral("Fetching"));

    service.snapshot.flightPlanStatus = FlightPlanStatus::Ready;
    service.Notify();

    QCOMPARE(viewModel.GetSimbriefStatusText(), QStringLiteral("Ready"));

    service.snapshot.flightPlanStatus = FlightPlanStatus::Error;
    service.Notify();

    QCOMPARE(viewModel.GetSimbriefStatusText(), QStringLiteral("Error"));
}

void OperationsViewModelTest::simbriefReadyAndErrorFlags()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    service.snapshot.flightPlanStatus = FlightPlanStatus::Ready;
    service.Notify();

    QVERIFY(viewModel.IsSimbriefReady());
    QVERIFY(!viewModel.HasSimbriefError());

    service.snapshot.flightPlanStatus = FlightPlanStatus::Error;
    service.Notify();

    QVERIFY(!viewModel.IsSimbriefReady());
    QVERIFY(viewModel.HasSimbriefError());
}

void OperationsViewModelTest::noopWhenSettingSameEnabledValue()
{
    FakeIntegratorService service;
    service.snapshot.automationEnabled = false;
    OperationsViewModel viewModel(&service);

    viewModel.SetEnabled(false);

    QCOMPARE(service.automationCalls, 0);
}

void OperationsViewModelTest::startLoadingDelegatesToService()
{
    FakeIntegratorService service;
    service.snapshot.canStartLoading = true;
    OperationsViewModel viewModel(&service);

    viewModel.startLoading();

    QCOMPARE(service.startLoadingCalls, 1);
    QVERIFY(viewModel.GetCommandError().isEmpty());
    QVERIFY(!viewModel.CanStartLoading());
}

void OperationsViewModelTest::startLoadingReportsRejectedCommands()
{
    FakeIntegratorService service;
    service.startLoadingResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.startLoading();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
}

void OperationsViewModelTest::exposesCanStartLoadingFromSnapshot()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    QVERIFY(!viewModel.CanStartLoading());

    service.snapshot.canStartLoading = true;
    service.Notify();

    QVERIFY(viewModel.CanStartLoading());
}

void OperationsViewModelTest::reloadSimbriefDelegatesToService()
{
    FakeIntegratorService service;
    OperationsViewModel viewModel(&service);

    viewModel.reloadSimbrief();

    QCOMPARE(service.reloadCalls, 1);
}

void OperationsViewModelTest::exposesAircraftPropertiesFromSnapshot()
{
    FakeIntegratorService service;
    service.snapshot.aircraftName = "TFDi MD-11";
    service.snapshot.plannedFuelKg = 12000.0;
    service.snapshot.plannedZfwKg = 180000.0;
    service.snapshot.plannedPax = 210;
    service.snapshot.gsxAvailable = true;
    service.snapshot.aircraftSupported = true;
    service.snapshot.sessionActive = true;
    service.snapshot.refuelBySelf = true;

    const OperationsViewModel viewModel(&service);

    QCOMPARE(viewModel.GetAircraftName(), QStringLiteral("TFDi MD-11"));
    QCOMPARE(viewModel.GetPlannedFuelKg(), 12000.0);
    QCOMPARE(viewModel.GetPlannedZfwKg(), 180000.0);
    QCOMPARE(viewModel.GetPlannedPax(), 210);
    QVERIFY(viewModel.IsGsxAvailable());
    QVERIFY(viewModel.IsAircraftSupported());
    QVERIFY(viewModel.IsSessionActive());
    QVERIFY(!viewModel.RefuelByGsx());
    QVERIFY(viewModel.RefuelBySelf());
}

void OperationsViewModelTest::successfulCommandClearsPreviousError()
{
    FakeIntegratorService service;
    service.snapshot.connected = true;
    service.snapshot.canToggleAutomation = true;
    service.automationResult = CommandResult::Failure("Rejected");
    OperationsViewModel viewModel(&service);

    viewModel.startFlow();

    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));

    service.automationResult = CommandResult::Success();
    const QSignalSpy spy(&viewModel, &OperationsViewModel::CommandErrorChanged);
    viewModel.startFlow();

    QVERIFY(viewModel.GetCommandError().isEmpty());
    QCOMPARE(spy.count(), 1);
}

void OperationsViewModelTest::exposesPhaseIndexCountAndTip()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    QCOMPARE(viewModel.GetPhaseCount(), static_cast<int>(TurnaroundPhase::Count));

    service.snapshot.phase = TurnaroundPhase::WaitingPushbackToStart;
    service.Notify();

    QCOMPARE(viewModel.GetPhase(), static_cast<int>(TurnaroundPhase::WaitingPushbackToStart));
    QCOMPARE(viewModel.GetPhaseTip(),
             QStringLiteral("Select the final pushback position in the GSX menu."));
    QCOMPARE(viewModel.phaseLabelAt(static_cast<int>(TurnaroundPhase::Boarding)),
             QStringLiteral("Boarding"));
    QVERIFY(viewModel.phaseLabelAt(-1).isEmpty());
}

void OperationsViewModelTest::exposesGsxProfileConflictFromSnapshot()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    QVERIFY(!viewModel.HasGsxProfileConflict());
    QVERIFY(!viewModel.IsGsxProfileFixable());

    service.snapshot.gsxProfileConflict = true;
    service.snapshot.gsxProfileFixable = true;
    service.Notify();

    QVERIFY(viewModel.HasGsxProfileConflict());
    QVERIFY(viewModel.IsGsxProfileFixable());
}

void OperationsViewModelTest::fixGsxProfileDelegatesToService()
{
    FakeIntegratorService service;
    service.snapshot.gsxProfileConflict = true;
    service.snapshot.gsxProfileFixable = true;
    OperationsViewModel viewModel(&service);

    viewModel.fixGsxProfile();

    QCOMPARE(service.fixGsxProfileCalls, 1);
    QVERIFY(viewModel.GetCommandError().isEmpty());
    QVERIFY(!viewModel.HasGsxProfileConflict());
}

void OperationsViewModelTest::fixGsxProfileReportsRejectedCommands()
{
    FakeIntegratorService service;
    service.fixGsxProfileResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.fixGsxProfile();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
}

void OperationsViewModelTest::restartFlowDelegatesToService()
{
    FakeIntegratorService service;
    OperationsViewModel viewModel(&service);

    viewModel.restartFlow();

    QCOMPARE(service.restartFlowCalls, 1);
    QCOMPARE(viewModel.GetCommandError(), QString());
}

void OperationsViewModelTest::restartFlowReportsRejectedCommands()
{
    FakeIntegratorService service;
    OperationsViewModel viewModel(&service);

    service.restartFlowResult = CommandResult::Failure("Simulator is offline.");

    viewModel.restartFlow();

    QCOMPARE(service.restartFlowCalls, 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Simulator is offline."));
}

void OperationsViewModelTest::exposesInDeboardingPhaseFromSnapshot()
{
    FakeIntegratorService service;
    const OperationsViewModel viewModel(&service);

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();

    QVERIFY(!viewModel.IsInDeboardingPhase());

    service.snapshot.phase = TurnaroundPhase::WaitingEngineShutdown;
    service.Notify();

    QVERIFY(viewModel.IsInDeboardingPhase());

    service.snapshot.phase = TurnaroundPhase::Deboarding;
    service.Notify();

    QVERIFY(viewModel.IsInDeboardingPhase());
}

QTEST_APPLESS_MAIN(OperationsViewModelTest)

#include "tst_operations_viewmodel.moc"
