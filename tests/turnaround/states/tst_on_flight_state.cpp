#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/OnFlightState.h"

class OnFlightStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhileAirborne();
    static void advancesWhenBackOnGround();
};

void OnFlightStateTest::holdsWhileAirborne()
{
    TurnaroundStateFixture f;
    OnFlightState state;

    f.gsxService.onGround = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void OnFlightStateTest::advancesWhenBackOnGround()
{
    TurnaroundStateFixture f;
    OnFlightState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingEngineShutdown);
}

QTEST_APPLESS_MAIN(OnFlightStateTest)

#include "tst_on_flight_state.moc"
