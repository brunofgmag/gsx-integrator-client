#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RequestFuelState.h"

class RequestFuelStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void doesNotRequestFuelWhenServiceIsUnavailable();
    static void advancesWhenRequestFuelServiceIsActive();
    static void requestsFuelOnlyOnce();
    static void shouldRetryWhenRequestFuelFails();
    static void advancesWhenRefuelingAlreadyCompleted();
    static void holdsRequestWhenAutoStartLoadingDisabled();
    static void requestsFuelAfterLoadingConfirmed();
    static void advancesFromExternalRefuelingWhileHolding();
    static void requestsFuelWhenSmartSwitchIsPressed();
    static void smartSwitchActsAsStartLoadingButton();
    static void doesNotNotifyAircraftWhileRequesting();
    static void flagsTheRequestGsxTookAndNeverServed();
    static void keepsQuietWhileGsxStillOffersTheService();
    static void flagsAPlanThatExceedsTheAirframeCapacity();
    static void staysQuietWhenThePlanFitsTheTanks();
    static void staysQuietWhenTheAircraftDoesNotKnowItsCapacity();
};

void RequestFuelStateTest::doesNotRequestFuelWhenServiceIsUnavailable()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Unavailable;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.refuelingCalls, 0);
}

void RequestFuelStateTest::advancesWhenRequestFuelServiceIsActive()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.hoseConnected = true;
    f.gsxService.refuelingState = GsxStateStatus::Active;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Refueling);
    QCOMPARE(f.menuGateway.refuelingCalls, 1);
    QCOMPARE(f.gsxService.takeOverCalls, 1);
    QVERIFY(f.ctx.data.refuelingRequested);
}

void RequestFuelStateTest::requestsFuelOnlyOnce()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.refuelingCalls, 1);
}

void RequestFuelStateTest::shouldRetryWhenRequestFuelFails()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 65; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.refuelingCalls, 2);
}

void RequestFuelStateTest::advancesWhenRefuelingAlreadyCompleted()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Unavailable;
    f.gsxService.refuelingCompleted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Refueling);
    QCOMPARE(f.menuGateway.refuelingCalls, 0);
}

void RequestFuelStateTest::holdsRequestWhenAutoStartLoadingDisabled()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.settings.autoStartLoading = false;
    f.gsxService.refuelingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 65; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.refuelingCalls, 0);
    QCOMPARE(f.gsxService.takeOverCalls, 0);
    QVERIFY(!f.ctx.data.refuelingRequested);
}

void RequestFuelStateTest::requestsFuelAfterLoadingConfirmed()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.settings.autoStartLoading = false;
    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.refuelingCalls, 0);

    f.ctx.data.loadingConfirmed = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.refuelingCalls, 1);
    QCOMPARE(f.gsxService.takeOverCalls, 1);
    QVERIFY(f.ctx.data.refuelingRequested);
}

void RequestFuelStateTest::advancesFromExternalRefuelingWhileHolding()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.settings.autoStartLoading = false;
    f.gsxService.refuelingState = GsxStateStatus::Active;
    f.gsxService.hoseConnected = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Refueling);
    QCOMPARE(f.menuGateway.refuelingCalls, 0);
}

void RequestFuelStateTest::requestsFuelWhenSmartSwitchIsPressed()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.settings.autoStartLoading = false;
    f.ctx.pilotTouched = true;
    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(f.gsxService.takeOverCalls);
    QVERIFY(f.menuGateway.refuelingCalls);
}

void RequestFuelStateTest::smartSwitchActsAsStartLoadingButton()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.settings.autoStartLoading = false;
    f.gsxService.refuelingState = GsxStateStatus::Unavailable;
    f.ctx.pilotTouched = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.pilotTouched);
    QVERIFY(f.ctx.data.loadingConfirmed);
    QCOMPARE(f.menuGateway.refuelingCalls, 0);

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.refuelingCalls, 1);
    QVERIFY(f.ctx.data.refuelingRequested);
}

void RequestFuelStateTest::doesNotNotifyAircraftWhileRequesting()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 65; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.aircraft.onLoadingStartedCalls, 0);
}

void RequestFuelStateTest::flagsTheRequestGsxTookAndNeverServed()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.refuelingCalls, 1);
    QVERIFY(!f.ctx.data.fuelRequestStalled);

    f.gsxService.refuelingState = GsxStateStatus::Requested;

    for (int tick = 0; tick < 599; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QVERIFY(!f.ctx.data.fuelRequestStalled);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(f.ctx.data.fuelRequestStalled);
}

void RequestFuelStateTest::keepsQuietWhileGsxStillOffersTheService()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 800; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QVERIFY(!f.ctx.data.fuelRequestStalled);
}

void RequestFuelStateTest::flagsAPlanThatExceedsTheAirframeCapacity()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.ctx.data.plannedFuelKg = 10360.0;
    f.aircraft.fuelCapacityKg = 9418.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(f.ctx.data.fuelPlanOverCapacity);
}

void RequestFuelStateTest::staysQuietWhenThePlanFitsTheTanks()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.ctx.data.plannedFuelKg = 5776.0;
    f.aircraft.fuelCapacityKg = 9418.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(!f.ctx.data.fuelPlanOverCapacity);
}

void RequestFuelStateTest::staysQuietWhenTheAircraftDoesNotKnowItsCapacity()
{
    TurnaroundStateFixture f;
    RequestFuelState state;

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.ctx.data.plannedFuelKg = 10360.0;
    f.aircraft.fuelCapacityKg = 0.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(!f.ctx.data.fuelPlanOverCapacity);
}

QTEST_APPLESS_MAIN(RequestFuelStateTest)

#include "tst_request_fuel_state.moc"
