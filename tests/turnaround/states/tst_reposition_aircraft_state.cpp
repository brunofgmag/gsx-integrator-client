#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RepositionAircraftState.h"

class RepositionAircraftStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhenRepositioningFails();
    static void advancesWhenRepositioningCompletes();
    static void advancesAfterGiveUpWhenRepositioningNeverHappens();
    static void skipsRepositioningWhenSettingEnabled();
};

void RepositionAircraftStateTest::holdsWhenRepositioningFails()
{
    TurnaroundStateFixture f;
    RepositionAircraftState state;

    f.gsxService.repositioning = false;
    f.ctx.data.repositionRequested = false;
    f.ctx.data.repositionCompleted = false;

    for (int tick = 0; tick < 45; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.repositionCalls, 5);
}

void RepositionAircraftStateTest::advancesWhenRepositioningCompletes()
{
    TurnaroundStateFixture f;
    RepositionAircraftState state;

    f.gsxService.repositioning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.repositionCalls, 1);
    QVERIFY(f.ctx.data.repositionRequested);
    QVERIFY(f.ctx.data.repositionCompleted);
}

void RepositionAircraftStateTest::advancesAfterGiveUpWhenRepositioningNeverHappens()
{
    TurnaroundStateFixture f;
    RepositionAircraftState state;

    f.gsxService.repositioning = false;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 61 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QVERIFY(f.ctx.data.repositionCompleted);
}

void RepositionAircraftStateTest::skipsRepositioningWhenSettingEnabled()
{
    TurnaroundStateFixture f;
    RepositionAircraftState state;

    f.settings.skipReposition = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::CallServices);
    QCOMPARE(f.menuGateway.repositionCalls, 0);
}

QTEST_APPLESS_MAIN(RepositionAircraftStateTest)

#include "tst_reposition_aircraft_state.moc"
