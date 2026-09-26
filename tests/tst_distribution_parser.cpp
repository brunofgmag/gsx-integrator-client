#include <QtTest/QTest>

#include "../src/infrastructure/update/DistributionParser.h"

class DistributionParserTest final : public QObject
{
    Q_OBJECT

private slots:
    static void flightsimToChannelCarriesThePageUrl();
    static void flightsimToWithoutPageUrlFallsBackToTheSiteHome();
    static void missingFileMeansGithub();
    static void malformedJsonMeansGithub();
    static void otherChannelMeansGithub();
    static void flightsimToBuildIsFlightsimToWhateverTheFileSays();
};

void DistributionParserTest::flightsimToChannelCarriesThePageUrl()
{
    const Distribution distribution = ParseDistribution(
        "{\"channel\": \"flightsim.to\", \"pageUrl\": \"https://flightsim.to/file/1/gsx-integrator\"}");

    QVERIFY(distribution.flightsimTo);
    QCOMPARE(distribution.pageUrl, QStringLiteral("https://flightsim.to/file/1/gsx-integrator"));
}

void DistributionParserTest::flightsimToWithoutPageUrlFallsBackToTheSiteHome()
{
    const Distribution missing = ParseDistribution("{\"channel\": \"flightsim.to\"}");
    const Distribution empty = ParseDistribution("{\"channel\": \"flightsim.to\", \"pageUrl\": \"  \"}");

    QVERIFY(missing.flightsimTo);
    QCOMPARE(missing.pageUrl, QStringLiteral("https://flightsim.to/"));
    QVERIFY(empty.flightsimTo);
    QCOMPARE(empty.pageUrl, QStringLiteral("https://flightsim.to/"));
}

void DistributionParserTest::missingFileMeansGithub()
{
    const Distribution distribution = ParseDistribution({});

    QVERIFY(!distribution.flightsimTo);
    QVERIFY(distribution.pageUrl.isEmpty());
}

void DistributionParserTest::malformedJsonMeansGithub()
{
    QVERIFY(!ParseDistribution("{\"channel\": \"flightsim.to\"").flightsimTo);
    QVERIFY(!ParseDistribution("[\"flightsim.to\"]").flightsimTo);
}

void DistributionParserTest::otherChannelMeansGithub()
{
    QVERIFY(!ParseDistribution("{\"channel\": \"github\", \"pageUrl\": \"https://flightsim.to/x\"}").flightsimTo);
    QVERIFY(!ParseDistribution("{\"pageUrl\": \"https://flightsim.to/x\"}").flightsimTo);
    QVERIFY(!ParseDistribution("{\"channel\": 3}").flightsimTo);
}

void DistributionParserTest::flightsimToBuildIsFlightsimToWhateverTheFileSays()
{
    const Distribution withPage = ParseFlightsimToDistribution(
        "{\"channel\": \"github\", \"pageUrl\": \"https://flightsim.to/file/1/gsx-integrator\"}");
    const Distribution withoutFile = ParseFlightsimToDistribution({});
    const Distribution malformed = ParseFlightsimToDistribution("{\"pageUrl\"");

    QVERIFY(withPage.flightsimTo);
    QCOMPARE(withPage.pageUrl, QStringLiteral("https://flightsim.to/file/1/gsx-integrator"));
    QVERIFY(withoutFile.flightsimTo);
    QCOMPARE(withoutFile.pageUrl, QStringLiteral("https://flightsim.to/"));
    QVERIFY(malformed.flightsimTo);
    QCOMPARE(malformed.pageUrl, QStringLiteral("https://flightsim.to/"));
}

QTEST_GUILESS_MAIN(DistributionParserTest)

#include "tst_distribution_parser.moc"
