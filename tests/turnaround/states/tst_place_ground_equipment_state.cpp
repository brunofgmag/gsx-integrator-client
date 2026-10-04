#include <QtTest/QTest>

#include <algorithm>
#include <string>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/PlaceGroundEquipmentState.h"

class PlaceGroundEquipmentStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenCallGpuDisabled();
    static void skipsWhenSettingsAreNull();
    static void advancesWithoutToggleWhenGpuAlreadyConnected();
    static void togglesGpuWhenDisconnectedThenWaitsForTheCart();
    static void givesUpWhenTheGpuCartNeverReachesTheAircraft();
    static void waitsWhileGpuStatusUnknown();
    static void prefersAircraftGroundPowerStatus();
    static void togglesWhenAircraftReportsDisconnected();
    static void commandsAircraftGpuWhenControlSupported();
    static void leavesAircraftGpuAloneWhenAlreadyConnected();
    static void placesChocksWhenSupported();
    static void placesChocksOnlyOnce();
    static void skipsChocksWhenUnsupported();
    static void placesChocksEvenWhileGpuUnknown();
    static void placesChocksWithoutGpuAndAdvances();
    static void callsGpuWithoutPlacingChocks();
    static void placesChocksAndCallsGpuWhenBothAreOn();
    static void ignoresPlaceChocksWhenTheAircraftCannotControlThem();
    static void closesAllDoorsEvenWhenCallGpuDisabled();
    static void closesAllDoorsOnlyOnce();
    static void leavesTheDoorsAloneWhileAGsxServiceIsUnderway();
    static void holdsThePhaseWithoutTouchingTheDoorsWhileTheServiceStateIsUnknown();
    static void closesTheDoorsOnceTheUnknownServiceStateResolvesToIdle();
    static void leavesTheDoorsAloneOnceTheUnknownServiceStateResolvesToUnderway();
    static void closesTheDoorsWhenTheServiceStateNeverArrives();
    static void releasesTheDepartureDoorHoldOnANewTurnaround();
    static void clearsTheGroundEquipmentTheAircraftPlacedItself();
    static void clearsTheAircraftGroundEquipmentOnlyOnce();
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

void PlaceGroundEquipmentStateTest::togglesGpuWhenDisconnectedThenWaitsForTheCart()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuRequested);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);

    f.gsxService.gpuStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
}

void PlaceGroundEquipmentStateTest::givesUpWhenTheGpuCartNeverReachesTheAircraft()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.ctx.data.stateTickCount = 240;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
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

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuRequested);
}

void PlaceGroundEquipmentStateTest::commandsAircraftGpuWhenControlSupported()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Disconnected;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
    QVERIFY(f.aircraft.groundPowerOn);
    QVERIFY(f.ctx.data.gpuRequested);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);

    f.aircraft.groundPowerStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 1);
}

void PlaceGroundEquipmentStateTest::leavesAircraftGpuAloneWhenAlreadyConnected()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.aircraft.supportsGroundPowerControl = true;
    f.aircraft.groundPowerStatus = GroundPowerStatus::Connected;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 0);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void PlaceGroundEquipmentStateTest::placesChocksWhenSupported()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.settings.placeChocks = true;
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
    f.settings.placeChocks = true;
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
    f.settings.placeChocks = true;
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
    f.settings.placeChocks = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.aircraft.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::placesChocksWithoutGpuAndAdvances()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.placeChocks = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QVERIFY(f.ctx.data.chocksPlaced);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.aircraft.setGroundPowerCalls, 0);
}

void PlaceGroundEquipmentStateTest::callsGpuWithoutPlacingChocks()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.settings.placeChocks = false;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.aircraft.setChocksCalls, 0);
    QVERIFY(!f.ctx.data.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::placesChocksAndCallsGpuWhenBothAreOn()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.settings.placeChocks = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Disconnected;
    f.aircraft.supportsChocksControl = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.setChocksCalls, 1);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void PlaceGroundEquipmentStateTest::ignoresPlaceChocksWhenTheAircraftCannotControlThem()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.settings.placeChocks = true;
    f.aircraft.supportsChocksControl = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QVERIFY(!f.ctx.data.chocksPlaced);
}

void PlaceGroundEquipmentStateTest::releasesTheDepartureDoorHoldOnANewTurnaround()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.aircraft.doorsHeldClosed = true;
    f.settings.callGpu = false;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.aircraft.doorsHeldClosed);
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

void PlaceGroundEquipmentStateTest::leavesTheDoorsAloneWhileAGsxServiceIsUnderway()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.gsxService.serviceUnderway = true;
    f.aircraft.doorsHeldClosed = true;

    const auto first = state.Evaluate(f.ctx);
    const auto second = state.Evaluate(f.ctx);
    const auto third = state.Evaluate(f.ctx);

    QVERIFY(first.has_value());
    QCOMPARE(first->next, TurnaroundPhase::CallServices);
    QVERIFY(second.has_value());
    QVERIFY(third.has_value());
    QCOMPARE(f.aircraft.closeAllDoorsCalls, 0);
    QVERIFY(!f.aircraft.doorsHeldClosed);
    QVERIFY(f.ctx.data.doorsClosed);
    QCOMPARE(
        std::count(
            f.logger.messages.begin(),
            f.logger.messages.end(),
            std::string("A GSX service is underway: leaving the doors as they are because closing them would "
                        "interrupt it")),
        1);
}

void PlaceGroundEquipmentStateTest::holdsThePhaseWithoutTouchingTheDoorsWhileTheServiceStateIsUnknown()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.gsxService.serviceUnderway = std::nullopt;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 0);
    QVERIFY(!f.ctx.data.doorsClosed);
    QCOMPARE(f.aircraft.clearOwnGroundEquipmentCalls, 1);
}

void PlaceGroundEquipmentStateTest::closesTheDoorsOnceTheUnknownServiceStateResolvesToIdle()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.gsxService.serviceUnderway = std::nullopt;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.serviceUnderway = false;

    const auto transition = state.Evaluate(f.ctx);
    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QVERIFY(state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
    QVERIFY(f.ctx.data.doorsClosed);
}

void PlaceGroundEquipmentStateTest::leavesTheDoorsAloneOnceTheUnknownServiceStateResolvesToUnderway()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.gsxService.serviceUnderway = std::nullopt;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.serviceUnderway = true;

    QVERIFY(state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 0);
    QVERIFY(f.ctx.data.doorsClosed);
}

void PlaceGroundEquipmentStateTest::closesTheDoorsWhenTheServiceStateNeverArrives()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;
    f.gsxService.serviceUnderway = std::nullopt;

    f.ctx.data.stateTickCount = 239;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.aircraft.closeAllDoorsCalls, 0);

    f.ctx.data.stateTickCount = 240;

    const auto transition = state.Evaluate(f.ctx);
    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QVERIFY(state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.closeAllDoorsCalls, 1);
    QVERIFY(f.ctx.data.doorsClosed);
}

void PlaceGroundEquipmentStateTest::clearsTheGroundEquipmentTheAircraftPlacedItself()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.aircraft.clearOwnGroundEquipmentCalls, 1);
    QVERIFY(f.ctx.data.ownGroundEquipmentCleared);
}

void PlaceGroundEquipmentStateTest::clearsTheAircraftGroundEquipmentOnlyOnce()
{
    TurnaroundStateFixture f;
    PlaceGroundEquipmentState state;

    f.settings.callGpu = true;
    f.gsxService.gpuStatus = GroundPowerStatus::Unknown;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.clearOwnGroundEquipmentCalls, 1);
}

QTEST_APPLESS_MAIN(PlaceGroundEquipmentStateTest)

#include "tst_place_ground_equipment_state.moc"
