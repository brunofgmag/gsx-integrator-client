#include <QtTest/QTest>

#include "doubles/FakeVariableGateway.h"
#include "../src/infrastructure/aircraft/SmartSwitch.h"

namespace
{
    constexpr auto kLVar = "SW";
    constexpr auto kCapt = "SW_CAPT";
    constexpr auto kFo = "SW_FO";

    SmartSwitch::Predicate PeaksTo(const double threshold)
    {
        return [threshold](double, const double max) { return max >= threshold; };
    }

    SmartSwitch::Predicate DipsTo(const double threshold)
    {
        return [threshold](const double min, double) { return min <= threshold; };
    }
}

class SmartSwitchTest final : public QObject
{
    Q_OBJECT

private slots:
    static void quietUntilSubscribed();
    static void firesOncePerPressAndRearms();
    static void catchesDownwardActivation();
    static void catchesTransientBetweenPolls();
    static void firesFromEitherLVar();
    static void writesResetValueWhenConfigured();
    static void subscribesRawLVarName();
};

void SmartSwitchTest::quietUntilSubscribed()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, PeaksTo(100.0));

    gateway.lvars[kLVar] = 100.0;

    QVERIFY(!smartSwitch.Consume());

    smartSwitch.Subscribe();

    QVERIFY(smartSwitch.Consume());
}

void SmartSwitchTest::firesOncePerPressAndRearms()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, PeaksTo(100.0));
    smartSwitch.Subscribe();

    gateway.lvars[kLVar] = 100.0;
    QVERIFY(smartSwitch.Consume());
    QVERIFY(!smartSwitch.Consume());

    gateway.lvars[kLVar] = 0.0;
    QVERIFY(!smartSwitch.Consume());

    gateway.lvars[kLVar] = 100.0;
    QVERIFY(smartSwitch.Consume());
}

void SmartSwitchTest::catchesDownwardActivation()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, DipsTo(0.0));
    smartSwitch.Subscribe();

    gateway.lvars[kLVar] = 1.0;
    QVERIFY(!smartSwitch.Consume());

    gateway.lvars[kLVar] = 0.0;
    QVERIFY(smartSwitch.Consume());
}

void SmartSwitchTest::catchesTransientBetweenPolls()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, PeaksTo(100.0));
    smartSwitch.Subscribe();

    gateway.lvars[kLVar] = 50.0;
    gateway.lvarSpans[kLVar] = LVarSpan{50.0, 100.0, true};

    QVERIFY(smartSwitch.Consume());
    QVERIFY(!smartSwitch.Consume());
}

void SmartSwitchTest::firesFromEitherLVar()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kCapt, kFo}, PeaksTo(100.0));
    smartSwitch.Subscribe();

    gateway.lvars[kFo] = 100.0;
    QVERIFY(smartSwitch.Consume());
    QVERIFY(!smartSwitch.Consume());
}

void SmartSwitchTest::writesResetValueWhenConfigured()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, DipsTo(0.0), 1.0);
    smartSwitch.Subscribe();

    gateway.lvars[kLVar] = 0.0;
    QVERIFY(smartSwitch.Consume());
    QCOMPARE(gateway.Written(kLVar), 1.0);
}

void SmartSwitchTest::subscribesRawLVarName()
{
    FakeVariableGateway gateway;
    SmartSwitch smartSwitch(gateway, {kLVar}, PeaksTo(100.0));
    smartSwitch.Subscribe();

    QCOMPARE(gateway.fastRefreshNames.size(), static_cast<std::size_t>(1));
    QVERIFY(gateway.fastRefreshNames.front() == kLVar);
}

QTEST_APPLESS_MAIN(SmartSwitchTest)

#include "tst_smart_switch.moc"
