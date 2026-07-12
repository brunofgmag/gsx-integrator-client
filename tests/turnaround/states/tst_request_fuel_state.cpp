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
    f.ctx.smartSwitchPressed = true;
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
    f.ctx.smartSwitchPressed = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.smartSwitchPressed);
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

QTEST_APPLESS_MAIN(RequestFuelStateTest)

#include "tst_request_fuel_state.moc"
