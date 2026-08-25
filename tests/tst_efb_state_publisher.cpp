#include <QtTest/QTest>

#include "TestDoubles.h"
#include "doubles/FakeCommBusBridgeGateway.h"
#include "../src/infrastructure/efb/EfbStatePublisher.h"
#include "../src/viewmodel/OperationsViewModel.h"

class EfbStatePublisherTest final : public QObject
{
    Q_OBJECT

private slots:
    static void publishesTheSnapshotWhenItChanges();
    static void staysSilentWhileTheSnapshotDoesNotChange();
    static void publishesNothingOnMsfs2020();
    static void publishesNothingWhileTheBridgeIsUnavailable();
    static void keepsRetryingAfterTheBridgeRefusesTheWrite();
    static void announcesTheDepartureWhenTheClientQuits();
    static void announcesNoDepartureOnMsfs2020();
    static void announcesNoDepartureWhileTheBridgeIsUnavailable();
    static void subscribesToTheAppHelloChannelOnSetup();
    static void answersTheAppHelloWithAFreshSnapshot();
    static void answersNoHelloOnMsfs2020();
    static void carriesTheLoadingModeFlagTheScreenColoursWith();
    static void carriesTheFlowButtonPermissionsTheWindowDecidesOnce();
    static void carriesTheRefusalTheWindowShowsWithoutASnapshotChange();
};

void EfbStatePublisherTest::publishesTheSnapshotWhenItChanges()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 1);
    QCOMPARE(std::get<1>(bridge.calls.front()), CommBusFlag::kJs);
    QVERIFY(std::get<2>(bridge.calls.front()).find("\"phase\"") != std::string::npos);
}

void EfbStatePublisherTest::staysSilentWhileTheSnapshotDoesNotChange()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();
    publisher.Publish();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 1);
}

void EfbStatePublisherTest::publishesNothingOnMsfs2020()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2020; });

    for (int tick = 0; tick < 5; ++tick)
    {
        service.snapshot.phase = static_cast<TurnaroundPhase>(tick);
        service.Notify();
        publisher.Publish();
    }

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);
}

void EfbStatePublisherTest::publishesNothingWhileTheBridgeIsUnavailable()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;
    bridge.available = false;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);

    bridge.available = true;
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 1);
}

void EfbStatePublisherTest::keepsRetryingAfterTheBridgeRefusesTheWrite()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;
    bridge.callSucceeds = false;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);

    bridge.callSucceeds = true;
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 1);
}

void EfbStatePublisherTest::announcesTheDepartureWhenTheClientQuits()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();
    publisher.PublishDeparture();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 2);
    QCOMPARE(std::get<1>(bridge.calls.back()), CommBusFlag::kJs);
    QCOMPARE(std::get<2>(bridge.calls.back()), std::string(R"({"connected":false})"));
}

void EfbStatePublisherTest::announcesNoDepartureOnMsfs2020()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2020; });

    publisher.PublishDeparture();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);
}

void EfbStatePublisherTest::announcesNoDepartureWhileTheBridgeIsUnavailable()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;
    bridge.available = false;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    publisher.PublishDeparture();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);
}

void EfbStatePublisherTest::subscribesToTheAppHelloChannelOnSetup()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });
    publisher.Setup();

    QCOMPARE(bridge.subscribed, std::vector<std::string>{EfbCommBus::kHelloChannel});
    QCOMPARE(bridge.subscribedFlags, std::vector<int>{CommBusFlag::kJs});
}

void EfbStatePublisherTest::answersTheAppHelloWithAFreshSnapshot()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });
    publisher.Setup();

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 1);

    bridge.Deliver(EfbCommBus::kHelloChannel, "hello");

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 2);
    QCOMPARE(std::get<2>(bridge.calls.back()), std::get<2>(bridge.calls.front()));
}

void EfbStatePublisherTest::answersNoHelloOnMsfs2020()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2020; });
    publisher.Setup();

    bridge.Deliver(EfbCommBus::kHelloChannel, "hello");

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), 0);
}

void EfbStatePublisherTest::carriesTheLoadingModeFlagTheScreenColoursWith()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    display.autoStartLoading = true;
    const OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.phase = TurnaroundPhase::Boarding;
    service.Notify();
    publisher.Publish();

    QVERIFY(std::get<2>(bridge.calls.front()).find(R"("autoStartLoading":true)") != std::string::npos);
}

QTEST_APPLESS_MAIN(EfbStatePublisherTest)

#include "tst_efb_state_publisher.moc"

void EfbStatePublisherTest::carriesTheFlowButtonPermissionsTheWindowDecidesOnce()
{
    FakeIntegratorService service;
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.connected = true;
    service.snapshot.canToggleAutomation = true;
    service.Notify();
    publisher.Publish();

    QVERIFY(std::get<2>(bridge.calls.back()).find(R"("canStartFlow":true)") != std::string::npos);
    QVERIFY(std::get<2>(bridge.calls.back()).find(R"("canRestartFlow":false)") != std::string::npos);

    viewModel.startFlow();
    publisher.Publish();

    QVERIFY(std::get<2>(bridge.calls.back()).find(R"("canStartFlow":false)") != std::string::npos);
    QVERIFY(std::get<2>(bridge.calls.back()).find(R"("canRestartFlow":true)") != std::string::npos);
}

void EfbStatePublisherTest::carriesTheRefusalTheWindowShowsWithoutASnapshotChange()
{
    FakeIntegratorService service;
    service.startLoadingResult = CommandResult::Failure("The turnaround is not waiting to start loading.");
    FakeOperationsDisplaySettings display;
    OperationsViewModel viewModel(&service, &display);
    FakeCommBusBridgeGateway bridge;

    EfbStatePublisher publisher(&bridge, &viewModel, [] { return SimVersion::Msfs2024; });

    service.snapshot.connected = true;
    service.Notify();
    publisher.Publish();

    const int before = bridge.CallCount(EfbCommBus::kStateChannel);

    viewModel.startLoading();
    publisher.Publish();

    QCOMPARE(bridge.CallCount(EfbCommBus::kStateChannel), before + 1);
    QVERIFY(std::get<2>(bridge.calls.back())
                .find("The turnaround is not waiting to start loading.") != std::string::npos);
}
