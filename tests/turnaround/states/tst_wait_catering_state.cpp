#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitCateringState.h"

class WaitCateringStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenCateringNeverRequested();
    static void skipsWhenCateringNotInProgress();
    static void waitsWhileCateringInProgressThenAdvances();
    static void advancesAfterTimeout();
};

void WaitCateringStateTest::skipsWhenCateringNeverRequested()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RemoveGroundEquipment);
}

void WaitCateringStateTest::skipsWhenCateringNotInProgress()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.ctx.data.cateringRequested = true;
    f.gsxService.cateringInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RemoveGroundEquipment);
}

void WaitCateringStateTest::waitsWhileCateringInProgressThenAdvances()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.ctx.data.cateringRequested = true;
    f.gsxService.cateringInProgress = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.settings.callCatering = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.cateringInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RemoveGroundEquipment);
}

void WaitCateringStateTest::advancesAfterTimeout()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.ctx.data.cateringRequested = true;
    f.gsxService.cateringInProgress = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 149 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    ++f.ctx.data.stateTickCount;
    transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RemoveGroundEquipment);
}

QTEST_APPLESS_MAIN(WaitCateringStateTest)

#include "tst_wait_catering_state.moc"
