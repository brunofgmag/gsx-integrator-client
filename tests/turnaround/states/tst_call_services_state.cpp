#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CallServicesState.h"

class CallServicesStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void advancesWhenStairsAreAvailable();
    static void advancesWhenJetwayIsAvailable();
    static void prefersJetwayWhenBothAreAvailable();
    static void callStairsWhenJetwayFailsToComplete();
    static void skipsWhenAircraftDoesNotSupportStairsOrJetways();
    static void advancesImmediatelyWhenJetwayAlreadyInPlace();
    static void givesUpAfterTwoAttempts();
    static void requestsGpuAndCateringWhenEnabled();
    static void skipsCateringForCargoVariant();
    static void retriesCateringUntilConfirmed();
    static void advancesWhenCateringNeverStarts();
    static void doesNotRequestGroundServicesWhenDisabled();
    static void skipsGpuToggleWhenAlreadyConnected();
    static void skipsGroundServicesWhenSettingsAreNull();
};

void CallServicesStateTest::advancesWhenStairsAreAvailable()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.stairsAvailable = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.stairsInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallServicesStateTest::advancesWhenJetwayIsAvailable()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.jetwayAvailable = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.jetwayInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallServicesStateTest::prefersJetwayWhenBothAreAvailable()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;
    f.menuGateway.callStairsResult = true;
    f.menuGateway.callJetwayResult = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.jetwayInPlace = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QVERIFY(f.ctx.data.jetwayOrStairsRequested);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallServicesStateTest::callStairsWhenJetwayFailsToComplete()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;
    f.menuGateway.callStairsResult = true;
    f.menuGateway.callJetwayResult = true;

    for (int tick = 0; tick < 121; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
}

void CallServicesStateTest::skipsWhenAircraftDoesNotSupportStairsOrJetways()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.aircraft.supportsStairsOrJetways = false;
    f.gsxService.jetwayAvailable = true;
    f.gsxService.stairsAvailable = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 0);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
}

void CallServicesStateTest::advancesImmediatelyWhenJetwayAlreadyInPlace()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.jetwayInPlace = true;
    f.gsxService.stairsAvailable = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 0);
    QCOMPARE(f.menuGateway.callStairsCalls, 0);
    QVERIFY(f.ctx.data.jetwayOrStairsCompleted);
}

void CallServicesStateTest::givesUpAfterTwoAttempts()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.gsxService.jetwayAvailable = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 240 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.callJetwayCalls, 1);
    QCOMPARE(f.menuGateway.callStairsCalls, 1);
    QCOMPARE(f.ctx.data.jetwayOrStairsAttempts, 2);
}

void CallServicesStateTest::requestsGpuAndCateringWhenEnabled()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.settings.callGpu = true;
    f.settings.callCatering = true;
    f.aircraft.supportsStairsOrJetways = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuRequested);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
    QVERIFY(!f.ctx.data.cateringRequested);

    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QVERIFY(f.ctx.data.cateringRequested);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
}

void CallServicesStateTest::skipsCateringForCargoVariant()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.settings.callCatering = true;
    f.aircraft.cargo = true;
    f.aircraft.supportsStairsOrJetways = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
    QVERIFY(!f.ctx.data.cateringRequested);
}

void CallServicesStateTest::retriesCateringUntilConfirmed()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.settings.callCatering = true;
    f.aircraft.supportsStairsOrJetways = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
    QVERIFY(!f.ctx.data.cateringRequested);

    for (int tick = 0; tick < 10; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    QVERIFY(f.menuGateway.requestCateringCalls >= 2);
    QVERIFY(!f.ctx.data.cateringRequested);

    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QVERIFY(f.ctx.data.cateringRequested);
}

void CallServicesStateTest::advancesWhenCateringNeverStarts()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.settings.callCatering = true;
    f.aircraft.supportsStairsOrJetways = false;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 200 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QVERIFY(f.ctx.data.cateringRequested);
}

void CallServicesStateTest::doesNotRequestGroundServicesWhenDisabled()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.aircraft.supportsStairsOrJetways = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
}

void CallServicesStateTest::skipsGpuToggleWhenAlreadyConnected()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.settings.callGpu = true;
    f.gsxService.gpuConnected = true;
    f.aircraft.supportsStairsOrJetways = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void CallServicesStateTest::skipsGroundServicesWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    CallServicesState state;

    f.ctx.settings = nullptr;
    f.aircraft.supportsStairsOrJetways = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
}

QTEST_APPLESS_MAIN(CallServicesStateTest)

#include "tst_call_services_state.moc"
