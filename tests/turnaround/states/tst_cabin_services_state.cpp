#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CabinServicesState.h"

class CabinServicesStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void advancesImmediatelyWhenAllDisabled();
    static void skipsCabinServicesWhenSettingsAreNull();
    static void dispatchesAllThreeThenWaitsForCompletion();
    static void retriesFailedTriggerWithoutSkippingOthers();
    static void doesNotCompleteBeforeServiceSeenInProgress();
    static void advancesAfterTimeoutWhenServiceStuck();
    static void forcesProgressAfterMaxFailedTriggerAttempts();
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

void CabinServicesStateTest::dispatchesAllThreeThenWaitsForCompletion()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.settings.callWater = true;
    f.settings.callCleaning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestWaterCalls, 1);
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCleaningCalls, 1);

    f.gsxService.lavatoryInProgress = true;
    f.gsxService.waterInProgress = true;
    f.gsxService.cleaningInProgress = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.lavatoryInProgress = false;
    f.gsxService.waterInProgress = false;
    f.gsxService.cleaningInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
    QCOMPARE(transition->delayTicks, 60);
}

void CabinServicesStateTest::retriesFailedTriggerWithoutSkippingOthers()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.settings.callWater = true;
    f.menuGateway.requestLavatoryResult = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 2);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);

    f.menuGateway.requestLavatoryResult = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 3);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestWaterCalls, 1);
}

void CabinServicesStateTest::doesNotCompleteBeforeServiceSeenInProgress()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void CabinServicesStateTest::advancesAfterTimeoutWhenServiceStuck()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.gsxService.lavatoryInProgress = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 400 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
}

void CabinServicesStateTest::forcesProgressAfterMaxFailedTriggerAttempts()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.settings.callWater = true;
    f.menuGateway.requestLavatoryResult = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 2);
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 3);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 3);
    QCOMPARE(f.menuGateway.requestWaterCalls, 1);
}

QTEST_APPLESS_MAIN(CabinServicesStateTest)

#include "tst_cabin_services_state.moc"
