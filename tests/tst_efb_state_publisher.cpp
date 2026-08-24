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

QTEST_APPLESS_MAIN(EfbStatePublisherTest)

#include "tst_efb_state_publisher.moc"
