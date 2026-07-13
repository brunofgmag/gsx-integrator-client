#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingReadyToPushState.h"

class WaitingReadyToPushStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilAircraftReady();
    static void advancesWhenReady();
};

void WaitingReadyToPushStateTest::holdsUntilAircraftReady()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingReadyToPushStateTest::advancesWhenReady()
{
    TurnaroundStateFixture f;
    WaitingReadyToPushState state;

    f.aircraft.readyToPush = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::DisconnectGpu);
}

QTEST_APPLESS_MAIN(WaitingReadyToPushStateTest)

#include "tst_waiting_ready_to_push_state.moc"
