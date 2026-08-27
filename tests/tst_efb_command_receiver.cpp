#include <QtTest/QTest>

#include "TestDoubles.h"
#include "doubles/FakeCommBusBridgeGateway.h"
#include "../src/infrastructure/efb/EfbCommandReceiver.h"
#include "../src/viewmodel/OperationsViewModel.h"

namespace
{
    std::string Command(const std::string& name)
    {
        return R"({"command":")" + name + R"("})";
    }
}

class EfbCommandReceiverTest final : public QObject
{
    Q_OBJECT

private slots:
    static void subscribesToTheCommandChannelOnSetup();
    static void eachTouchReachesTheSameFacadeCallTheWindowButtonMakes();
    static void aRefusedTouchCarriesTheReasonTheWindowWouldShow();
    static void anUnknownCommandIsDroppedAndNothingElseRuns();
    static void aMalformedPayloadIsDroppedAndNothingElseRuns();
    static void aStampedTouchReachesTheServiceWithThePhaseTheScreenDrew();
    static void aTouchWithoutAPhaseStampIsDropped();
    static void aRefusedStampedTouchCarriesTheReasonToTheScreen();
};

void EfbCommandReceiverTest::subscribesToTheCommandChannelOnSetup()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    QCOMPARE(bridge.subscribed.size(), std::size_t{1});
    QCOMPARE(bridge.subscribed.front(), std::string(EfbCommBus::kCommandChannel));
    QCOMPARE(bridge.subscribedFlags.front(), CommBusFlag::kJs);
}

void EfbCommandReceiverTest::eachTouchReachesTheSameFacadeCallTheWindowButtonMakes()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("startFlow"));

    QCOMPARE(service.automationCalls, 1);
    QVERIFY(service.snapshot.automationEnabled);

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("startLoading"));

    QCOMPARE(service.startLoadingCalls, 1);

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("restartFlow"));

    QCOMPARE(service.restartFlowCalls, 1);

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("reloadSimbrief"));

    QCOMPARE(service.reloadCalls, 1);
}

void EfbCommandReceiverTest::aRefusedTouchCarriesTheReasonTheWindowWouldShow()
{
    FakeIntegratorService service;
    service.startLoadingResult = CommandResult::Failure("The turnaround is not waiting to start loading.");
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("startLoading"));

    QCOMPARE(viewModel.GetCommandError(),
             QStringLiteral("The turnaround is not waiting to start loading."));

    OperationsViewModel windowViewModel(&service, &display);
    windowViewModel.startLoading();

    QCOMPARE(viewModel.GetCommandError(), windowViewModel.GetCommandError());
}

void EfbCommandReceiverTest::anUnknownCommandIsDroppedAndNothingElseRuns()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("openTheGsxMenu"));

    QCOMPARE(service.automationCalls, 0);
    QCOMPARE(service.startLoadingCalls, 0);
    QCOMPARE(service.restartFlowCalls, 0);
    QCOMPARE(service.reloadCalls, 0);
    QVERIFY(viewModel.GetCommandError().isEmpty());
}

void EfbCommandReceiverTest::aMalformedPayloadIsDroppedAndNothingElseRuns()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, "not json at all");
    bridge.Deliver(EfbCommBus::kCommandChannel, "{}");
    bridge.Deliver(EfbCommBus::kCommandChannel, R"({"command":42})");
    bridge.Deliver(EfbCommBus::kCommandChannel, "");

    QCOMPARE(service.automationCalls, 0);
    QCOMPARE(service.startLoadingCalls, 0);
    QCOMPARE(service.restartFlowCalls, 0);
    QCOMPARE(service.reloadCalls, 0);
}

void EfbCommandReceiverTest::aStampedTouchReachesTheServiceWithThePhaseTheScreenDrew()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel,
                   R"({"command":"pilotTouch","phase":12})");

    QCOMPARE(service.pilotTouchCalls, 1);
    QCOMPARE(service.pilotTouchStamp, TurnaroundPhase::WaitingReadyToPush);
}

void EfbCommandReceiverTest::aTouchWithoutAPhaseStampIsDropped()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, Command("pilotTouch"));
    bridge.Deliver(EfbCommBus::kCommandChannel, R"({"command":"pilotTouch","phase":"12"})");
    bridge.Deliver(EfbCommBus::kCommandChannel, R"({"command":"pilotTouch","phase":-1})");
    bridge.Deliver(EfbCommBus::kCommandChannel,
                   R"({"command":"pilotTouch","phase":)"
                   + std::to_string(static_cast<int>(TurnaroundPhase::Count)) + "}");

    QCOMPARE(service.pilotTouchCalls, 0);
    QVERIFY(viewModel.GetCommandError().isEmpty());
}

void EfbCommandReceiverTest::aRefusedStampedTouchCarriesTheReasonToTheScreen()
{
    FakeIntegratorService service;
    service.pilotTouchResult = CommandResult::Failure("The turnaround moved on before your touch arrived.");
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbCommandReceiver receiver(&bridge, &viewModel);
    receiver.Setup();

    bridge.Deliver(EfbCommBus::kCommandChannel, R"({"command":"pilotTouch","phase":25})");

    QCOMPARE(viewModel.GetCommandError(),
             QStringLiteral("The turnaround moved on before your touch arrived."));
}

QTEST_APPLESS_MAIN(EfbCommandReceiverTest)

#include "tst_efb_command_receiver.moc"
