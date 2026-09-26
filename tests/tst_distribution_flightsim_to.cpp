#include <QtTest/QTest>

#include "../src/infrastructure/update/DistributionParser.h"

class DistributionFlightsimToTest final : public QObject
{
    Q_OBJECT

private slots:
    static void flightsimToBuildIsFlightsimToWhateverTheFileSays();
    static void flightsimToBuildWithoutFileFallsBackToTheSiteHome();
};

void DistributionFlightsimToTest::flightsimToBuildIsFlightsimToWhateverTheFileSays()
{
    const Distribution distribution = DistributionOfThisBuild(
        "{\"channel\": \"github\", \"pageUrl\": \"https://flightsim.to/file/1/gsx-integrator\"}");

    QVERIFY(distribution.flightsimTo);
    QCOMPARE(distribution.pageUrl, QStringLiteral("https://flightsim.to/file/1/gsx-integrator"));
}

void DistributionFlightsimToTest::flightsimToBuildWithoutFileFallsBackToTheSiteHome()
{
    const Distribution distribution = DistributionOfThisBuild({});

    QVERIFY(distribution.flightsimTo);
    QCOMPARE(distribution.pageUrl, QStringLiteral("https://flightsim.to/"));
}

QTEST_GUILESS_MAIN(DistributionFlightsimToTest)

#include "tst_distribution_flightsim_to.moc"
