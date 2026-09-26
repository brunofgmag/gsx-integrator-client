#include <QtTest/QTest>

#include "../src/infrastructure/update/DistributionParser.h"

class DistributionParserTest final : public QObject
{
    Q_OBJECT

private slots:
    static void flightsimToFileCarriesThePageUrl();
    static void flightsimToWithoutPageUrlFallsBackToTheSiteHome();
    static void flightsimToBuildIsFlightsimToWhateverTheFileSays();
    static void githubBuildIgnoresAFileSayingFlightsimTo();
};

void DistributionParserTest::flightsimToFileCarriesThePageUrl()
{
    const Distribution distribution = ParseFlightsimToDistribution(
        "{\"channel\": \"flightsim.to\", \"pageUrl\": \"https://flightsim.to/file/1/gsx-integrator\"}");

    QVERIFY(distribution.flightsimTo);
    QCOMPARE(distribution.pageUrl, QStringLiteral("https://flightsim.to/file/1/gsx-integrator"));
}

void DistributionParserTest::flightsimToWithoutPageUrlFallsBackToTheSiteHome()
{
    const Distribution missing = ParseFlightsimToDistribution("{\"channel\": \"flightsim.to\"}");
    const Distribution empty = ParseFlightsimToDistribution("{\"channel\": \"flightsim.to\", \"pageUrl\": \"  \"}");

    QCOMPARE(missing.pageUrl, QStringLiteral("https://flightsim.to/"));
    QCOMPARE(empty.pageUrl, QStringLiteral("https://flightsim.to/"));
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

void DistributionParserTest::githubBuildIgnoresAFileSayingFlightsimTo()
{
    const Distribution distribution = DistributionOfThisBuild(
        "{\"channel\": \"flightsim.to\", \"pageUrl\": \"https://flightsim.to/file/1/gsx-integrator\"}");

    QVERIFY(!distribution.flightsimTo);
    QVERIFY(distribution.pageUrl.isEmpty());
}

QTEST_GUILESS_MAIN(DistributionParserTest)

#include "tst_distribution_parser.moc"
