#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingSupportedAircraftState.h"

class WaitingSupportedAircraftStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWithoutAircraft();
    static void advancesWhenAircraftAttached();
};

void WaitingSupportedAircraftStateTest::holdsWithoutAircraft()
{
    TurnaroundStateFixture f;
    f.ctx.aircraft = nullptr;

    WaitingSupportedAircraftState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingSupportedAircraftStateTest::advancesWhenAircraftAttached()
{
    TurnaroundStateFixture f;
    WaitingSupportedAircraftState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingAircraftReady);
}

QTEST_APPLESS_MAIN(WaitingSupportedAircraftStateTest)

#include "tst_waiting_supported_aircraft_state.moc"
