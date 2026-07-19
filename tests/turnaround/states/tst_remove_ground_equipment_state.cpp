#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RemoveGroundEquipmentState.h"

class RemoveGroundEquipmentStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenGpuManagementDisabled();
    static void skipsWhenGpuNotConnected();
    static void dismissesConnectedGpuThenAdvances();
    static void advancesAfterMaxAttempts();
    static void dismissesWhenAircraftReportsConnected();
    static void skipsWhenAircraftReportsDisconnected();
    static void holdsDismissWhileMenuUnsettled();
    static void runsWhenOnlyArrivalOptionEnabled();
    static void removesChocksWhenSupported();
    static void removesChocksOnlyOnce();
    static void removesChocksEvenWhenGpuDisconnected();
    static void skipsChocksWhenManagementDisabled();
    static void finishesGpuDismissWhenDisabledMidRun();
    static void removesPlacedChocksWhenGpuTogglesDisabledMidRun();
};

void RemoveGroundEquipmentStateTest::skipsWhenGpuManagementDisabled()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.callGpuOnArrival = false;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void RemoveGroundEquipmentStateTest::skipsWhenGpuNotConnected()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void RemoveGroundEquipmentStateTest::dismissesConnectedGpuThenAdvances()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuDismissRequested);

    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void RemoveGroundEquipmentStateTest::advancesAfterMaxAttempts()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 240 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 3);
}

void RemoveGroundEquipmentStateTest::dismissesWhenAircraftReportsConnected()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Connected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuDismissRequested);
}

void RemoveGroundEquipmentStateTest::skipsWhenAircraftReportsDisconnected()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void RemoveGroundEquipmentStateTest::holdsDismissWhileMenuUnsettled()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.menuGateway.menuSettled = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(!f.ctx.data.gpuDismissRequested);
}

void RemoveGroundEquipmentStateTest::runsWhenOnlyArrivalOptionEnabled()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.callGpuOnArrival = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuDismissRequested);
}

void RemoveGroundEquipmentStateTest::removesChocksWhenSupported()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.supportsChocksControl = true;
    f.aircraft.chocksPlaced = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(!f.aircraft.chocksPlaced);
    QVERIFY(f.ctx.data.chocksRemoved);
}

void RemoveGroundEquipmentStateTest::removesChocksOnlyOnce()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.setChocksCalls, 1);
}

void RemoveGroundEquipmentStateTest::removesChocksEvenWhenGpuDisconnected()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;
    f.aircraft.chocksPlaced = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(!f.aircraft.chocksPlaced);
}

void RemoveGroundEquipmentStateTest::skipsChocksWhenManagementDisabled()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.callGpuOnArrival = false;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void RemoveGroundEquipmentStateTest::finishesGpuDismissWhenDisabledMidRun()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.gpuDismissRequested);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);

    f.settings.callGpu = false;
    f.settings.callGpuOnArrival = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void RemoveGroundEquipmentStateTest::removesPlacedChocksWhenGpuTogglesDisabledMidRun()
{
    TurnaroundStateFixture f;
    RemoveGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.callGpuOnArrival = false;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;
    f.aircraft.chocksPlaced = true;
    f.ctx.data.chocksPlaced = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.ctx.data.chocksRemoved);
}

QTEST_APPLESS_MAIN(RemoveGroundEquipmentStateTest)

#include "tst_remove_ground_equipment_state.moc"
