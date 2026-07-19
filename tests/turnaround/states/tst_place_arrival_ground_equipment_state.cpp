#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/PlaceArrivalGroundEquipmentState.h"

class PlaceArrivalGroundEquipmentStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenOptionDisabled();
    static void skipsWhenSettingsAreNull();
    static void waitsWhileEnginesRunning();
    static void waitsWhileParkingBrakeNotSet();
    static void placesChocksAndGpuWhenParked();
    static void advancesWithoutToggleWhenGpuAlreadyConnected();
    static void waitsWhileGpuStatusUnknown();
    static void holdsToggleWhileMenuUnsettled();
    static void skipsChocksWhenUnsupported();
    static void closesAllDoorsEvenWhenOptionDisabled();
    static void closesAllDoorsOnlyOnce();
};

void PlaceArrivalGroundEquipmentStateTest::skipsWhenOptionDisabled()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::skipsWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.ctx.settings = nullptr;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileEnginesRunning()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = true;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileParkingBrakeNotSet()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::placesChocksAndGpuWhenParked()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = true;
    f.aircraft.supportsChocksControl = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.arrivalGpuRequested);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.aircraft.chocksPlaced);
    QVERIFY(f.ctx.data.arrivalChocksPlaced);
}

void PlaceArrivalGroundEquipmentStateTest::advancesWithoutToggleWhenGpuAlreadyConnected()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::waitsWhileGpuStatusUnknown()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::holdsToggleWhileMenuUnsettled()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.menuGateway.menuSettled = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QVERIFY(!f.ctx.data.arrivalGpuRequested);
}

void PlaceArrivalGroundEquipmentStateTest::skipsChocksWhenUnsupported()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.parkingBrakeSet = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 0);
}

void PlaceArrivalGroundEquipmentStateTest::closesAllDoorsEvenWhenOptionDisabled()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
    QVERIFY(f.ctx.data.arrivalDoorsClosed);
}

void PlaceArrivalGroundEquipmentStateTest::closesAllDoorsOnlyOnce()
{
    TurnaroundStateFixture f;
    PlaceArrivalGroundEquipmentState state;

    f.settings.callGpuOnArrival = true;
    f.aircraft.engineRunning = false;
    f.aircraft.parkingBrakeSet = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
}

QTEST_APPLESS_MAIN(PlaceArrivalGroundEquipmentStateTest)

#include "tst_place_arrival_ground_equipment_state.moc"
