#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/CallCateringState.h"

class CallCateringStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void requestsCateringWhenEnabled();
    static void advancesWhenCateringDisabled();
    static void skipsCateringForCargoVariant();
    static void retriesCateringUntilConfirmed();
    static void advancesWhenCateringNeverStarts();
    static void holdsWhileMenuUnsettled();
    static void skipsCateringWhenSettingsAreNull();
};

void CallCateringStateTest::requestsCateringWhenEnabled()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.settings.callCatering = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
    QVERIFY(!f.ctx.data.cateringRequested);

    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QVERIFY(f.ctx.data.cateringRequested);
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
}

void CallCateringStateTest::advancesWhenCateringDisabled()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
    QVERIFY(!f.ctx.data.cateringRequested);
}

void CallCateringStateTest::skipsCateringForCargoVariant()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.settings.callCatering = true;
    f.aircraft.cargo = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
    QVERIFY(!f.ctx.data.cateringRequested);
}

void CallCateringStateTest::retriesCateringUntilConfirmed()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.settings.callCatering = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.requestCateringCalls, 1);
    QVERIFY(!f.ctx.data.cateringRequested);

    for (int tick = 0; tick < 10; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    QVERIFY(f.menuGateway.requestCateringCalls >= 2);
    QVERIFY(!f.ctx.data.cateringRequested);

    f.gsxService.cateringInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QVERIFY(f.ctx.data.cateringRequested);
}

void CallCateringStateTest::advancesWhenCateringNeverStarts()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.settings.callCatering = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 200 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QVERIFY(f.ctx.data.cateringRequested);
}

void CallCateringStateTest::holdsWhileMenuUnsettled()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.settings.callCatering = true;
    f.menuGateway.menuSettled = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
}

void CallCateringStateTest::skipsCateringWhenSettingsAreNull()
{
    TurnaroundStateFixture f;
    CallCateringState state;

    f.ctx.settings = nullptr;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
    QCOMPARE(f.menuGateway.requestCateringCalls, 0);
}

QTEST_APPLESS_MAIN(CallCateringStateTest)

#include "tst_call_catering_state.moc"
