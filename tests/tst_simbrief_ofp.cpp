#include <QtTest/QTest>

#include "../src/infrastructure/simbrief/SimbriefOfpParser.h"

class SimbriefOfpTest final : public QObject
{
    Q_OBJECT

private slots:
    static void parsesKilograms();
    static void convertsPounds();
    static void rejectsIncompletePayload();
    static void rejectsEmptyPayload();
    static void rejectsZeroZfw();
    static void rejectsNegativeFuel();
    static void rejectsZeroFuel();
    static void rejectsMalformedFuel();
    static void rejectsNegativePax();
    static void defaultsPassengersToZeroWhenMissing();
    static void defaultsToKilogramsWhenUnitsMissing();
    static void ignoresUnknownUnitsAsKilograms();
};

void SimbriefOfpTest::parsesKilograms()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>12000</plan_ramp>"
        "<est_zfw>180000</est_zfw><pax_count>210</pax_count>";

    const auto flightPlan = ParseSimbriefOfp(payload);

    QVERIFY(flightPlan.has_value());
    QCOMPARE(flightPlan->fuelKg, 12000.0);
    QCOMPARE(flightPlan->zfwKg, 180000.0);
    QCOMPARE(flightPlan->passengers, 210);
    QCOMPARE(flightPlan->unit, WeightUnit::Kg);
}

void SimbriefOfpTest::convertsPounds()
{
    constexpr char payload[] =
        "<units>lbs</units><plan_ramp>2204.62262185</plan_ramp>"
        "<est_zfw>4409.2452437</est_zfw>";

    const auto flightPlan = ParseSimbriefOfp(payload);

    QVERIFY(flightPlan.has_value());
    QVERIFY(qAbs(flightPlan->fuelKg - 1000.0) < 0.01);
    QVERIFY(qAbs(flightPlan->zfwKg - 2000.0) < 0.01);
    QCOMPARE(flightPlan->unit, WeightUnit::Lb);
}

void SimbriefOfpTest::rejectsIncompletePayload()
{
    constexpr char payload[] = "<plan_ramp>12000</plan_ramp>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::rejectsEmptyPayload()
{
    QVERIFY(!ParseSimbriefOfp("").has_value());
}

void SimbriefOfpTest::rejectsZeroZfw()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>12000</plan_ramp><est_zfw>0</est_zfw>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::rejectsNegativeFuel()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>-1</plan_ramp><est_zfw>180000</est_zfw>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::rejectsZeroFuel()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>0</plan_ramp><est_zfw>180000</est_zfw>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::rejectsMalformedFuel()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>abc</plan_ramp><est_zfw>180000</est_zfw>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::rejectsNegativePax()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>12000</plan_ramp>"
        "<est_zfw>180000</est_zfw><pax_count>-5</pax_count>";

    QVERIFY(!ParseSimbriefOfp(payload).has_value());
}

void SimbriefOfpTest::defaultsPassengersToZeroWhenMissing()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>12000</plan_ramp><est_zfw>180000</est_zfw>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QCOMPARE(plan->passengers, 0);
}

void SimbriefOfpTest::defaultsToKilogramsWhenUnitsMissing()
{
    constexpr char payload[] =
        "<plan_ramp>12000</plan_ramp><est_zfw>180000</est_zfw>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QCOMPARE(plan->fuelKg, 12000.0);
    QCOMPARE(plan->zfwKg, 180000.0);
    QCOMPARE(plan->unit, WeightUnit::Kg);
}

void SimbriefOfpTest::ignoresUnknownUnitsAsKilograms()
{
    constexpr char payload[] =
        "<units>tonnes</units><plan_ramp>12000</plan_ramp><est_zfw>180000</est_zfw>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QCOMPARE(plan->fuelKg, 12000.0);
    QCOMPARE(plan->unit, WeightUnit::Kg);
}

QTEST_APPLESS_MAIN(SimbriefOfpTest)

#include "tst_simbrief_ofp.moc"
