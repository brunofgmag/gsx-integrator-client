#include <QtTest/QTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/states/WaitingDepartureState.h"

class WaitingDepartureStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsOnGround();
    static void advancesWhenAirborne();
};

void WaitingDepartureStateTest::holdsOnGround()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingDepartureStateTest::advancesWhenAirborne()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    f.gsxService.onGround = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::OnFlight);
}

QTEST_APPLESS_MAIN(WaitingDepartureStateTest)

#include "tst_waiting_departure_state.moc"
