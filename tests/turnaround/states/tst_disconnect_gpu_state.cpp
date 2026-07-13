#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/DisconnectGpuState.h"

class DisconnectGpuStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void skipsWhenGpuManagementDisabled();
    static void skipsWhenGpuNotConnected();
    static void dismissesConnectedGpuThenAdvances();
    static void advancesAfterMaxAttempts();
};

void DisconnectGpuStateTest::skipsWhenGpuManagementDisabled()
{
    TurnaroundStateFixture f;
    DisconnectGpuState state;

    f.settings.callGpu = false;
    f.gsxService.gpuConnected = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void DisconnectGpuStateTest::skipsWhenGpuNotConnected()
{
    TurnaroundStateFixture f;
    DisconnectGpuState state;

    f.settings.callGpu = true;
    f.gsxService.gpuConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 0);
}

void DisconnectGpuStateTest::dismissesConnectedGpuThenAdvances()
{
    TurnaroundStateFixture f;
    DisconnectGpuState state;

    f.settings.callGpu = true;
    f.gsxService.gpuConnected = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
    QVERIFY(f.ctx.data.gpuDismissRequested);

    f.gsxService.gpuConnected = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 1);
}

void DisconnectGpuStateTest::advancesAfterMaxAttempts()
{
    TurnaroundStateFixture f;
    DisconnectGpuState state;

    f.settings.callGpu = true;
    f.gsxService.gpuConnected = true;

    std::optional<TurnaroundTransition> transition;
    for (int tick = 0; tick < 240 && !transition; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        transition = state.Evaluate(f.ctx);
    }

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestPushback);
    QCOMPARE(f.menuGateway.toggleGpuCalls, 3);
}

QTEST_APPLESS_MAIN(DisconnectGpuStateTest)

#include "tst_disconnect_gpu_state.moc"
