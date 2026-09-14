#include <QtTest/QTest>

#include <string>
#include "../src/infrastructure/simbrief/SimbriefOfpParser.h"

class SimbriefOfpTest final : public QObject
{
    Q_OBJECT

private slots:
    static void parsesKilograms();
    static void readsTheAirportPairAndTheGenerationTime();
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
    static void readsTheOperatingEmptyWeightAndThePayloadLine();
    static void convertsTheOperatingEmptyWeightAndThePayloadLineFromPounds();
    static void keepsAPlanWithoutAPayloadLineAndLeavesTheLineAbsent();
    static void leavesAnUnreadablePayloadLineAbsent();
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

void SimbriefOfpTest::readsTheAirportPairAndTheGenerationTime()
{
    constexpr char payload[] =
        "<params><time_generated>1786922025</time_generated><units>kgs</units></params>"
        "<plan_ramp>7444</plan_ramp><est_zfw>57770</est_zfw>"
        "<origin><icao_code>SBFZ</icao_code><iata_code>FOR</iata_code></origin>"
        "<destination><icao_code>SBTE</icao_code><iata_code>THE</iata_code></destination>"
        "<alternate><icao_code>SBSL</icao_code></alternate>";

    const auto flightPlan = ParseSimbriefOfp(payload);

    QVERIFY(flightPlan.has_value());
    QCOMPARE(flightPlan->origin, std::string("SBFZ"));
    QCOMPARE(flightPlan->destination, std::string("SBTE"));
    QCOMPARE(flightPlan->generatedEpoch, 1786922025LL);
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

void SimbriefOfpTest::readsTheOperatingEmptyWeightAndThePayloadLine()
{
    constexpr char payload[] =
        "<params><units>kgs</units></params><fuel><plan_ramp>10127</plan_ramp></fuel>"
        "<weights><oew>42306</oew><pax_count>12</pax_count><freight_added>2000</freight_added>"
        "<cargo>2000</cargo><payload>3252</payload><est_zfw>45558</est_zfw>"
        "<max_zfw>61689</max_zfw></weights>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QCOMPARE(plan->operatingEmptyKg, 42306.0);
    QVERIFY(plan->payloadKg.has_value());
    QCOMPARE(*plan->payloadKg, 3252.0);
}

void SimbriefOfpTest::convertsTheOperatingEmptyWeightAndThePayloadLineFromPounds()
{
    constexpr char payload[] =
        "<units>lbs</units><plan_ramp>22046.2262185</plan_ramp><oew>93268.7646399</oew>"
        "<payload>36376.2732605</payload><est_zfw>129645.0379004</est_zfw>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QVERIFY(qAbs(plan->operatingEmptyKg - 42306.0) < 0.01);
    QVERIFY(plan->payloadKg.has_value());
    QVERIFY(qAbs(*plan->payloadKg - 16500.0) < 0.01);
}

void SimbriefOfpTest::keepsAPlanWithoutAPayloadLineAndLeavesTheLineAbsent()
{
    constexpr char payload[] =
        "<units>kgs</units><plan_ramp>12000</plan_ramp><est_zfw>180000</est_zfw>";

    const auto plan = ParseSimbriefOfp(payload);

    QVERIFY(plan.has_value());
    QCOMPARE(plan->operatingEmptyKg, 0.0);
    QVERIFY(!plan->payloadKg.has_value());
}

void SimbriefOfpTest::leavesAnUnreadablePayloadLineAbsent()
{
    for (const char* line : {"<payload>abc</payload>", "<payload>-1</payload>"})
    {
        const std::string payload =
            std::string("<units>kgs</units><plan_ramp>12000</plan_ramp><est_zfw>180000</est_zfw>")
            + "<oew>90000</oew>" + line;

        const auto plan = ParseSimbriefOfp(payload);

        QVERIFY2(plan.has_value(), line);
        QVERIFY2(!plan->payloadKg.has_value(), line);
    }
}

QTEST_APPLESS_MAIN(SimbriefOfpTest)

#include "tst_simbrief_ofp.moc"
