#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitCateringState.h"

class WaitCateringStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenCateringDisabled();
    static void skipsWhenCateringNotInProgress();
    static void waitsWhileCateringInProgressThenAdvances();
    static void advancesAfterTimeout();
};

void WaitCateringStateTest::skipsWhenCateringDisabled()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.settings.callCatering = false;
    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::DisconnectGpu);
}

void WaitCateringStateTest::skipsWhenCateringNotInProgress()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.settings.callCatering = true;
    f.gsxService.cateringInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::DisconnectGpu);
}

void WaitCateringStateTest::waitsWhileCateringInProgressThenAdvances()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.settings.callCatering = true;
    f.gsxService.cateringInProgress = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.cateringInProgress = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::DisconnectGpu);
}

void WaitCateringStateTest::advancesAfterTimeout()
{
    TurnaroundStateFixture f;
    WaitCateringState state;

    f.settings.callCatering = true;
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
    QCOMPARE(transition->next, TurnaroundPhase::DisconnectGpu);
}

QTEST_APPLESS_MAIN(WaitCateringStateTest)

#include "tst_wait_catering_state.moc"
