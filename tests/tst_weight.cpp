#include <QtTest/QTest>

#include "../src/domain/support/Weight.h"

class WeightTest final : public QObject
{
    Q_OBJECT

private slots:
    static void convertsPoundsToKilograms();
    static void convertsKilogramsToPounds();
    static void roundTripPreservesValue();
    static void handlesZeroAndNegative();
    static void reverseRoundTripFromPounds();
    static void autoUnitPrefersSimbriefWhenReady();
    static void autoUnitFallsBackToAircraftWhenSimbriefUnavailable();
    static void autoUnitDefaultsToKilograms();
};

void WeightTest::convertsPoundsToKilograms()
{
    QVERIFY(qAbs(weight::LbToKg(2204.62262185) - 1000.0) < 1e-6);
    QVERIFY(qAbs(weight::LbToKg(1.0) - 0.45359237) < 1e-9);
}

void WeightTest::convertsKilogramsToPounds()
{
    QVERIFY(qAbs(weight::KgToLb(1000.0) - 2204.62262185) < 1e-4);
    QVERIFY(qAbs(weight::KgToLb(0.45359237) - 1.0) < 1e-9);
}

void WeightTest::roundTripPreservesValue()
{
    constexpr double kg = 73456.0;

    QVERIFY(qAbs(weight::LbToKg(weight::KgToLb(kg)) - kg) < 1e-6);
}

void WeightTest::handlesZeroAndNegative()
{
    QCOMPARE(weight::LbToKg(0.0), 0.0);
    QCOMPARE(weight::KgToLb(0.0), 0.0);
    QVERIFY(weight::LbToKg(-100.0) < 0.0);
}

void WeightTest::reverseRoundTripFromPounds()
{
    constexpr double lb = 162345.678;

    QVERIFY(qAbs(weight::KgToLb(weight::LbToKg(lb)) - lb) < 1e-6);
}

void WeightTest::autoUnitPrefersSimbriefWhenReady()
{
    QCOMPARE(weight::ResolveAutoWeightUnit(true, true, WeightUnit::Lb, WeightUnit::Kg), WeightUnit::Lb);
    QCOMPARE(weight::ResolveAutoWeightUnit(true, true, WeightUnit::Kg, WeightUnit::Lb), WeightUnit::Kg);
}

void WeightTest::autoUnitFallsBackToAircraftWhenSimbriefUnavailable()
{
    QCOMPARE(weight::ResolveAutoWeightUnit(false, false, WeightUnit::Kg, WeightUnit::Lb), WeightUnit::Lb);
    QCOMPARE(weight::ResolveAutoWeightUnit(true, false, WeightUnit::Lb, WeightUnit::Kg), WeightUnit::Kg);
}

void WeightTest::autoUnitDefaultsToKilograms()
{
    QCOMPARE(weight::ResolveAutoWeightUnit(false, false, WeightUnit::Lb, std::nullopt), WeightUnit::Kg);
    QCOMPARE(weight::ResolveAutoWeightUnit(true, false, WeightUnit::Lb, std::nullopt), WeightUnit::Kg);
}

QTEST_APPLESS_MAIN(WeightTest)

#include "tst_weight.moc"
