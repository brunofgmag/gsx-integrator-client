#include <QtTest/QTest>

#include "../src/application/sim/TickMode.h"

Q_DECLARE_METATYPE(TickMode)

class TickModeTest final : public QObject
{
    Q_OBJECT

private slots:
    static void resolvesTheTruthTable_data();
    static void resolvesTheTruthTable();
};

void TickModeTest::resolvesTheTruthTable_data()
{
    QTest::addColumn<bool>("automationEnabled");
    QTest::addColumn<bool>("gsxAvailable");
    QTest::addColumn<bool>("actsOnTheSim");
    QTest::addColumn<TickMode>("expected");

    QTest::newRow("automation with the gsx up drives") << true << true << false << TickMode::Driving;
    QTest::newRow("automation with the gsx up drives under the probe env") << true << true << true
        << TickMode::Driving;
    QTest::newRow("automation with the gsx down only observes") << true << false << false << TickMode::ObserveOnly;
    QTest::newRow("automation with the gsx down only observes under the probe env") << true << false << true
        << TickMode::ObserveOnly;
    QTest::newRow("the probe env alone observes with the gsx up") << false << true << true << TickMode::ObserveOnly;
    QTest::newRow("the probe env alone observes with the gsx down") << false << false << true
        << TickMode::ObserveOnly;
    QTest::newRow("without automation or the probe env the gsx up stays idle") << false << true << false
        << TickMode::Idle;
    QTest::newRow("without automation or the probe env the gsx down stays idle") << false << false << false
        << TickMode::Idle;
}

void TickModeTest::resolvesTheTruthTable()
{
    QFETCH(bool, automationEnabled);
    QFETCH(bool, gsxAvailable);
    QFETCH(bool, actsOnTheSim);
    QFETCH(TickMode, expected);

    QCOMPARE(TickModeResolution::Resolve(automationEnabled, gsxAvailable, actsOnTheSim), expected);
}

QTEST_APPLESS_MAIN(TickModeTest)

#include "tst_tick_mode.moc"
