#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CabinServicesState.h"

class CabinServicesStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void advancesImmediatelyWhenAllDisabled();
    static void requestsServicesInLavatoryWaterCleaningOrder();
    static void onlyRequestsEnabledServices();
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

void CabinServicesStateTest::requestsServicesInLavatoryWaterCleaningOrder()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callLavatory = true;
    f.settings.callWater = true;
    f.settings.callCleaning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 1);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestWaterCalls, 1);
    QCOMPARE(f.menuGateway.requestCleaningCalls, 0);

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCleaningCalls, 1);

    const auto transition = state.Evaluate(f.ctx);
    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
}

void CabinServicesStateTest::onlyRequestsEnabledServices()
{
    TurnaroundStateFixture f;
    CabinServicesState state;

    f.settings.callCleaning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestLavatoryCalls, 0);
    QCOMPARE(f.menuGateway.requestWaterCalls, 0);
    QCOMPARE(f.menuGateway.requestCleaningCalls, 1);

    const auto transition = state.Evaluate(f.ctx);
    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingNewFlight);
}

QTEST_APPLESS_MAIN(CabinServicesStateTest)

#include "tst_cabin_services_state.moc"
