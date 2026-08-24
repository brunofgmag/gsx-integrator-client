#include <QtCore/QLocale>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/viewmodel/OperationsViewModel.h"

class OperationsViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void distinguishesAPhaseUnlockedByThePilotFromOneAdvancedByReading();
    static void exposesUpdatedSnapshot();
    static void emitsOneSignalForSnapshotChanges();
    static void doesNotEmitWhenSnapshotIsUnchanged();
    static void ignoresInsignificantFloatingPointChanges();
    static void reportsRejectedCommands();
    static void mapsFlightPlanStatusToText();
    static void simbriefReadyAndErrorFlags();
    static void simbriefRefusalReachesTheScreen();
    static void aRefusedPlanIsNotReadyOnTheCard();
    static void noopWhenSettingSameEnabledValue();
    static void startLoadingDelegatesToService();
    static void startLoadingReportsRejectedCommands();
    static void exposesCanStartLoadingFromSnapshot();
    static void waitingForLoadingOverridesStateTextAndTip();
    static void reloadSimbriefDelegatesToService();
    static void exposesAircraftPropertiesFromSnapshot();
    static void successfulCommandClearsPreviousError();
    static void exposesPhaseIndexCountAndTip();
    static void flightPlanTipFollowsPlanSource();
    static void exposesGsxProfileConflictFromSnapshot();
    static void fixGsxProfileDelegatesToService();
    static void fixGsxProfileReportsRejectedCommands();
    static void fixPmdgOptionsDelegatesToService();
    static void fixPmdgOptionsReportsRejectedCommands();
    static void restartFlowDelegatesToService();
    static void restartFlowReportsRejectedCommands();
    static void nextPhaseTextNamesThePhaseThatFollows();
    static void nextPhaseTextOnTheLastPhaseAnnouncesANewSession();
    static void holdCountdownTextCountsTheRemainingSeconds();
    static void holdCountdownTextIsEmptyWhenNothingIsHolding();
    static void aircraftNameTextStandsByWhileTheAircraftIsUnsupported();
    static void plannedFuelTextFollowsTheDisplayWeightUnit();
    static void eachWeightTextReadsItsOwnSnapshotField();
    static void exposesInDeboardingPhaseFromSnapshot();
};

void OperationsViewModelTest::waitingForLoadingOverridesStateTextAndTip()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.phase = TurnaroundPhase::RequestFuel;
    service.Notify();

    QCOMPARE(viewModel.GetStateText(), QStringLiteral("Requesting fuel"));

    service.snapshot.canStartLoading = true;
    service.Notify();

    QCOMPARE(viewModel.GetStateText(), QStringLiteral("Waiting for start loading"));
    QVERIFY(viewModel.GetPhaseTip().contains(QStringLiteral("START LOADING")));
}

void OperationsViewModelTest::exposesUpdatedSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.connected = true;
    service.snapshot.phase = TurnaroundPhase::WaitingAircraftReady;
    service.Notify();

    QVERIFY(viewModel.IsConnected());
    QCOMPARE(viewModel.GetStateText(), QStringLiteral("Waiting for aircraft ready"));
}

void OperationsViewModelTest::emitsOneSignalForSnapshotChanges()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
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
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    const QSignalSpy spy(&viewModel, &OperationsViewModel::SnapshotChanged);

    service.Notify();

    QCOMPARE(spy.count(), 0);
}

void OperationsViewModelTest::ignoresInsignificantFloatingPointChanges()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.snapshot.fuelProgress = 10.0;

    const OperationsViewModel viewModel(&service, &display);
    const QSignalSpy spy(&viewModel, &OperationsViewModel::SnapshotChanged);

    service.snapshot.fuelProgress = 10.00001;
    service.Notify();

    QCOMPARE(spy.count(), 0);
}

void OperationsViewModelTest::reportsRejectedCommands()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.snapshot.connected = true;
    service.snapshot.canToggleAutomation = true;
    service.automationResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service, &display);
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
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

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
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

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
    FakeOperationsDisplaySettings display;
    service.snapshot.automationEnabled = false;
    OperationsViewModel viewModel(&service, &display);

    viewModel.SetEnabled(false);

    QCOMPARE(service.automationCalls, 0);
}

void OperationsViewModelTest::startLoadingDelegatesToService()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.snapshot.canStartLoading = true;
    OperationsViewModel viewModel(&service, &display);

    viewModel.startLoading();

    QCOMPARE(service.startLoadingCalls, 1);
    QVERIFY(viewModel.GetCommandError().isEmpty());
    QVERIFY(!viewModel.CanStartLoading());
}

void OperationsViewModelTest::startLoadingReportsRejectedCommands()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.startLoadingResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service, &display);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.startLoading();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
}

void OperationsViewModelTest::exposesCanStartLoadingFromSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    QVERIFY(!viewModel.CanStartLoading());

    service.snapshot.canStartLoading = true;
    service.Notify();

    QVERIFY(viewModel.CanStartLoading());
}

void OperationsViewModelTest::reloadSimbriefDelegatesToService()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);

    viewModel.reloadSimbrief();

    QCOMPARE(service.reloadCalls, 1);
}

void OperationsViewModelTest::exposesAircraftPropertiesFromSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.snapshot.aircraftName = "TFDi MD-11";
    service.snapshot.plannedFuelKg = 12000.0;
    service.snapshot.plannedZfwKg = 180000.0;
    service.snapshot.plannedPax = 210;
    service.snapshot.gsxAvailable = true;
    service.snapshot.aircraftSupported = true;
    service.snapshot.sessionActive = true;
    service.snapshot.refuelBySelf = true;

    const OperationsViewModel viewModel(&service, &display);

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
    FakeOperationsDisplaySettings display;
    service.snapshot.connected = true;
    service.snapshot.canToggleAutomation = true;
    service.automationResult = CommandResult::Failure("Rejected");
    OperationsViewModel viewModel(&service, &display);

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
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    QCOMPARE(viewModel.GetPhaseCount(), static_cast<int>(TurnaroundPhase::Count));

    service.snapshot.phase = TurnaroundPhase::WaitingPushbackToStart;
    service.Notify();

    QCOMPARE(viewModel.GetPhase(), static_cast<int>(TurnaroundPhase::WaitingPushbackToStart));
    QCOMPARE(viewModel.GetPhaseTip(),
             QStringLiteral("Select the final pushback position in the GSX menu."));
}

void OperationsViewModelTest::flightPlanTipFollowsPlanSource()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.phase = TurnaroundPhase::WaitingFlightPlan;
    service.Notify();

    QCOMPARE(viewModel.GetPhaseTip(),
             QStringLiteral("Check that SimBrief is loaded in GSX and in this app."));

    service.snapshot.efbFlightPlan = true;
    service.Notify();

    QCOMPARE(viewModel.GetPhaseTip(),
             QStringLiteral("Import your SimBrief flight plan on the aircraft EFB."));
}

void OperationsViewModelTest::exposesGsxProfileConflictFromSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

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
    FakeOperationsDisplaySettings display;
    service.snapshot.gsxProfileConflict = true;
    service.snapshot.gsxProfileFixable = true;
    OperationsViewModel viewModel(&service, &display);

    viewModel.fixGsxProfile();

    QCOMPARE(service.fixGsxProfileCalls, 1);
    QVERIFY(viewModel.GetCommandError().isEmpty());
    QVERIFY(!viewModel.HasGsxProfileConflict());
}

void OperationsViewModelTest::fixGsxProfileReportsRejectedCommands()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.fixGsxProfileResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service, &display);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.fixGsxProfile();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
}

void OperationsViewModelTest::fixPmdgOptionsDelegatesToService()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.snapshot.pmdgOptionsConflict = true;
    service.snapshot.pmdgOptionsFixable = true;
    OperationsViewModel viewModel(&service, &display);

    viewModel.fixPmdgOptions();

    QCOMPARE(service.fixPmdgOptionsCalls, 1);
    QVERIFY(viewModel.GetCommandError().isEmpty());
    QVERIFY(!viewModel.HasPmdgOptionsConflict());
}

void OperationsViewModelTest::fixPmdgOptionsReportsRejectedCommands()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    service.fixPmdgOptionsResult = CommandResult::Failure("Rejected");

    OperationsViewModel viewModel(&service, &display);
    const QSignalSpy errorSpy(&viewModel, &OperationsViewModel::CommandErrorChanged);

    viewModel.fixPmdgOptions();

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Rejected"));
}

void OperationsViewModelTest::restartFlowDelegatesToService()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);

    viewModel.restartFlow();

    QCOMPARE(service.restartFlowCalls, 1);
    QCOMPARE(viewModel.GetCommandError(), QString());
}

void OperationsViewModelTest::restartFlowReportsRejectedCommands()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);

    service.restartFlowResult = CommandResult::Failure("Simulator is offline.");

    viewModel.restartFlow();

    QCOMPARE(service.restartFlowCalls, 1);
    QCOMPARE(viewModel.GetCommandError(), QStringLiteral("Simulator is offline."));
}

void OperationsViewModelTest::exposesInDeboardingPhaseFromSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

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

void OperationsViewModelTest::simbriefRefusalReachesTheScreen()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    QVERIFY(viewModel.GetSimbriefRefusal().isEmpty());

    service.snapshot.simbriefRefusal = "SimBrief aircraft A320 doesn't match MSFS aircraft A321";
    service.Notify();

    QCOMPARE(viewModel.GetSimbriefRefusal(),
             QString("SimBrief aircraft A320 doesn't match MSFS aircraft A321"));
}

void OperationsViewModelTest::aRefusedPlanIsNotReadyOnTheCard()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.flightPlanStatus = FlightPlanStatus::Ready;
    service.Notify();

    QVERIFY(viewModel.IsSimbriefReady());
    QVERIFY(!viewModel.HasSimbriefError());

    service.snapshot.simbriefRefusal =
        "The loaded flight plan from CYVR doesn't match the one on SimBrief, from SBFZ to SBTE";
    service.Notify();

    QVERIFY(!viewModel.IsSimbriefReady());
    QVERIFY(viewModel.HasSimbriefError());
    QVERIFY(viewModel.GetSimbriefStatusText() != QString("Ready"));
}

void OperationsViewModelTest::distinguishesAPhaseUnlockedByThePilotFromOneAdvancedByReading()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    QVERIFY(!viewModel.AdvancedByPilot());

    service.snapshot.phaseOrigin = TransitionOrigin::Pilot;
    service.Notify();

    QVERIFY(viewModel.AdvancedByPilot());

    service.snapshot.phaseOrigin = TransitionOrigin::Reading;
    service.Notify();

    QVERIFY(!viewModel.AdvancedByPilot());
}

void OperationsViewModelTest::nextPhaseTextNamesThePhaseThatFollows()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.phase = TurnaroundPhase::WaitingReadyToPush;
    service.Notify();

    QCOMPARE(viewModel.GetNextPhaseText(), QStringLiteral("Next \u25B8 Waiting for catering"));
}

void OperationsViewModelTest::nextPhaseTextOnTheLastPhaseAnnouncesANewSession()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.phase = TurnaroundPhase::WaitingNewFlight;
    service.Notify();

    QCOMPARE(viewModel.GetNextPhaseText(), QStringLiteral("Next \u25B8 New session"));
}

void OperationsViewModelTest::holdCountdownTextCountsTheRemainingSeconds()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.delayTicksRemaining = 12;
    service.Notify();

    QCOMPARE(viewModel.GetHoldCountdownText(), QStringLiteral("Next state in 12s"));
}

void OperationsViewModelTest::holdCountdownTextIsEmptyWhenNothingIsHolding()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.delayTicksRemaining = 0;
    service.Notify();

    QVERIFY(viewModel.GetHoldCountdownText().isEmpty());
}

void OperationsViewModelTest::aircraftNameTextStandsByWhileTheAircraftIsUnsupported()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.aircraftName = "PMDG 737-800";
    service.snapshot.aircraftSupported = false;
    service.Notify();

    QCOMPARE(viewModel.GetAircraftNameText(), QStringLiteral("Standby"));

    service.snapshot.aircraftSupported = true;
    service.Notify();

    QCOMPARE(viewModel.GetAircraftNameText(), QStringLiteral("PMDG 737-800"));
}

void OperationsViewModelTest::plannedFuelTextFollowsTheDisplayWeightUnit()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.plannedFuelKg = 12000.0;
    service.Notify();

    QCOMPARE(viewModel.GetPlannedFuelText(), QLocale().toString(12000) + QStringLiteral(" kg"));

    display.weightIsLb = true;

    QCOMPARE(viewModel.GetPlannedFuelText(), QLocale().toString(26455) + QStringLiteral(" lb"));
}

void OperationsViewModelTest::eachWeightTextReadsItsOwnSnapshotField()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);

    service.snapshot.loadedFuelKg = 1000.0;
    service.snapshot.targetFuelKg = 2000.0;
    service.snapshot.targetZfwKg = 3000.0;
    service.snapshot.plannedFuelKg = 4000.0;
    service.snapshot.plannedZfwKg = 5000.0;
    service.Notify();

    QCOMPARE(viewModel.GetLoadedFuelText(), QLocale().toString(1000) + QStringLiteral(" kg"));
    QCOMPARE(viewModel.GetTargetFuelText(), QLocale().toString(2000) + QStringLiteral(" kg"));
    QCOMPARE(viewModel.GetTargetZfwText(), QLocale().toString(3000) + QStringLiteral(" kg"));
    QCOMPARE(viewModel.GetPlannedFuelText(), QLocale().toString(4000) + QStringLiteral(" kg"));
    QCOMPARE(viewModel.GetPlannedZfwText(), QLocale().toString(5000) + QStringLiteral(" kg"));
}

QTEST_APPLESS_MAIN(OperationsViewModelTest)

#include "tst_operations_viewmodel.moc"
