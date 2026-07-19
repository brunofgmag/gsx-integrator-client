#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/PlaceGroundEquipmentState.h"

class PlaceGroundEquipmentStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenCallGpuDisabled();
    static void skipsWhenSettingsAreNull();
    static void advancesWithoutToggleWhenGpuAlreadyConnected();
    static void togglesGpuWhenDisconnectedThenAdvances();
    static void waitsWhileGpuStatusUnknown();
    static void prefersAircraftGroundPowerStatus();
    static void togglesWhenAircraftReportsDisconnected();
    static void holdsToggleWhileMenuUnsettled();
    static void retriesWhenToggleRejected();
    static void placesChocksWhenSupported();
    static void placesChocksOnlyOnce();
    static void skipsChocksWhenUnsupported();
    static void placesChocksEvenWhileGpuUnknown();
    static void closesAllDoorsEvenWhenCallGpuDisabled();
    static void closesAllDoorsOnlyOnce();
};

void PlaceGroundEquipmentStateTest::skipsWhenCallGpuDisabled()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceGroundEquipmentStateTest::skipsWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.ctx.settings = nullptr;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceGroundEquipmentStateTest::advancesWithoutToggleWhenGpuAlreadyConnected()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceGroundEquipmentStateTest::togglesGpuWhenDisconnectedThenAdvances()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::waitsWhileGpuStatusUnknown()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(!f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::prefersAircraftGroundPowerStatus()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceGroundEquipmentStateTest::togglesWhenAircraftReportsDisconnected()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::holdsToggleWhileMenuUnsettled()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.menuGateway.menuSettled = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(!f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::retriesWhenToggleRejected()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.menuGateway.toggleGpuResult = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(!f.ctx.data.gpuRequested);

    f.menuGateway.toggleGpuResult = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 2);
    QVERIFY(f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::placesChocksWhenSupported()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.aircraft.chocksPlaced);
    QVERIFY(f.ctx.data.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::placesChocksOnlyOnce()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.setChocksCalls, 1);
}

void PlaceGroundEquipmentStateTest::skipsChocksWhenUnsupported()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
    QVERIFY(!f.ctx.data.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::placesChocksEvenWhileGpuUnknown()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.aircraft.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::closesAllDoorsEvenWhenCallGpuDisabled()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
    QVERIFY(f.ctx.data.doorsClosed);
}

void PlaceGroundEquipmentStateTest::closesAllDoorsOnlyOnce()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
}

QTEST_APPLESS_MAIN(PlaceGroundEquipmentStateTest)

#include "tst_place_ground_equipment_state.moc"
