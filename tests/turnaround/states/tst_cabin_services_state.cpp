#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CabinServicesState.h"

class CabinServicesStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void advancesImmediatelyWhenAllDisabled();
    static void skipsCabinServicesWhenSettingsAreNull();
    static void confirmsEachServiceThenWaitsForCompletion();
    static void waitsForActiveServiceWhenDisabledMidRun();
    static void retriesTriggerUntilServiceInProgress();
    static void givesUpWhenServiceNeverStarts();
};

void CabinServicesStateTest::advancesImmediatelyWhenAllDisabled()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
    QCOMPARE(transition->delayTicks, 60);
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 0);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);
    QCOMPARE(f.menuGateway.requestCleaningCalls, 0);
}

void CabinServicesStateTest::skipsCabinServicesWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.ctx.settings = nullptr;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
    QCOMPARE(transition->delayTicks, 60);
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 0);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);
    QCOMPARE(f.menuGateway.requestCleaningCalls, 0);
}

void CabinServicesStateTest::confirmsEachServiceThenWaitsForCompletion()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.settings.callWater = true;
    f.settings.callCleaning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);

    f.gsxService.lavatoryInProgress = true;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.lavatoryRequested);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestWaterCalls, 1);
    QCOMPARE(f.menuGateway.requestCleaningCalls, 0);

    f.gsxService.waterInProgress = true;
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCleaningCalls, 1);

    f.gsxService.cleaningInProgress = true;
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.lavatoryInProgress = false;
    f.gsxService.waterInProgress = false;
    f.gsxService.cleaningInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
    QCOMPARE(transition->delayTicks, 60);
}

void CabinServicesStateTest::waitsForActiveServiceWhenDisabledMidRun()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);

    f.gsxService.lavatoryInProgress = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.lavatoryActiveSeen);

    f.settings.callLavatory = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.lavatoryInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
}

void CabinServicesStateTest::retriesTriggerUntilServiceInProgress()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QVERIFY(!f.ctx.data.lavatoryRequested);

    for (int tick = 0; tick < 10; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    QVERIFY(f.menuGateway.requestLavatoryCalls >= 2);
    QVERIFY(!f.ctx.data.lavatoryRequested);

    f.gsxService.lavatoryInProgress = true;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.lavatoryRequested);
}

void CabinServicesStateTest::givesUpWhenServiceNeverStarts()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 200 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
}

QTEST_APPLESS_MAIN(CabinServicesStateTest)

#include "tst_cabin_services_state.moc"
