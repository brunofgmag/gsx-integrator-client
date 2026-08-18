#include <QtTest/QTest>

#include <QtCore/QString>
#include <QtCore/QTemporaryDir>
#include <filesystem>
#include <fstream>
#include <string>
#include "../src/infrastructure/ifly/IFlyPlanFile.h"
#include "../src/infrastructure/simbrief/SimbriefOfpParser.h"

namespace
{
    constexpr auto kPlanFile = "activeflightplan.xml";
    constexpr long long kPlanEpoch = 1786922025;

    int gPlanWarnings = 0;

    void CountPlanWarnings(QtMsgType, const QMessageLogContext&, const QString& message)
    {
        if (message.contains(QStringLiteral("no readable flight plan")))
        {
            ++gPlanWarnings;
        }
    }

    std::filesystem::path PathOf(const QTemporaryDir& dir)
    {
        return std::filesystem::path(dir.path().toStdWString());
    }

    std::filesystem::path FltplanUnder(const std::filesystem::path& appData)
    {
        return appData / "Microsoft Flight Simulator 2024" / "WASM" / "MSFS2020"
            / "ifly-aircraft-737max8" / "work" / "navdata" / "FLTPLAN";
    }

    std::string OfpWithEpoch(const long long epoch, const std::size_t padBytes = 0)
    {
        return "<?xml version=\"1.0\"?><OFP><params><time_generated>"
            + std::to_string(epoch)
            + "</time_generated><units>kgs</units></params>"
            + "<plan_ramp>7444</plan_ramp><est_zfw>57770</est_zfw>"
            + std::string(padBytes, ' ')
            + "<destination><icao_code>SBTE</icao_code></destination></OFP>";
    }

    void WritePlan(const std::filesystem::path& directory, const std::string& xml)
    {
        std::filesystem::create_directories(directory);

        std::ofstream stream(directory / kPlanFile, std::ios::binary);
        stream << xml;
    }
}

class IFlyPlanFileTest final : public QObject
{
    Q_OBJECT

private slots:
    static void directorySitsUnderTheIflyWorkFolder();
    static void missingFolderIsNotGuessed();
    static void readsTheEpochFromTheFileHead();
    static void ignoresAnEpochPastTheFirstFourKilobytes();
    static void epochMatchesWhatTheParserReads();
    static void missingFileHasNoEpoch();
    static void matchingEpochIsSeenAndAnotherPlanIsNot();
    static void latchSurvivesTheFileVanishing();
    static void rearmsWhenThePlanChanges();
    static void blindUntilTheFirstLook();
    static void unresolvedFolderReleasesTheFlowAndSaysSoOnce();
    static void unreadableFileAlsoReleasesTheFlow();
};

void IFlyPlanFileTest::directorySitsUnderTheIflyWorkFolder()
{
    const QTemporaryDir appData;
    std::filesystem::create_directories(FltplanUnder(PathOf(appData)));

    const auto directory = IFlyPlanFile::DirectoryUnder(PathOf(appData));

    QVERIFY(directory.has_value());
    QCOMPARE(*directory, FltplanUnder(PathOf(appData)));
    QCOMPARE(directory->filename().string(), std::string("FLTPLAN"));
    QCOMPARE(directory->parent_path().parent_path().filename().string(), std::string("work"));
}

void IFlyPlanFileTest::missingFolderIsNotGuessed()
{
    const QTemporaryDir appData;

    QCOMPARE(IFlyPlanFile::DirectoryUnder(PathOf(appData)), std::nullopt);
}

void IFlyPlanFileTest::readsTheEpochFromTheFileHead()
{
    const QTemporaryDir whole;
    const QTemporaryDir truncated;
    const std::string xml = OfpWithEpoch(kPlanEpoch, 200000);

    WritePlan(PathOf(whole), xml);
    WritePlan(PathOf(truncated), xml.substr(0, 4096));

    QCOMPARE(IFlyPlanFile::PlanEpochIn(PathOf(whole)), kPlanEpoch);
    QCOMPARE(IFlyPlanFile::PlanEpochIn(PathOf(truncated)), IFlyPlanFile::PlanEpochIn(PathOf(whole)));
}

void IFlyPlanFileTest::ignoresAnEpochPastTheFirstFourKilobytes()
{
    const QTemporaryDir dir;
    WritePlan(PathOf(dir), std::string(5000, ' ') + OfpWithEpoch(kPlanEpoch));

    QCOMPARE(IFlyPlanFile::PlanEpochIn(PathOf(dir)), 0LL);
}

void IFlyPlanFileTest::epochMatchesWhatTheParserReads()
{
    const QTemporaryDir dir;
    const std::string xml = OfpWithEpoch(kPlanEpoch);
    WritePlan(PathOf(dir), xml);

    const auto plan = ParseSimbriefOfp(xml);

    QVERIFY(plan.has_value());
    QCOMPARE(IFlyPlanFile::PlanEpochIn(PathOf(dir)), plan->generatedEpoch);
}

void IFlyPlanFileTest::missingFileHasNoEpoch()
{
    const QTemporaryDir dir;

    QCOMPARE(IFlyPlanFile::PlanEpochIn(PathOf(dir)), 0LL);
}

void IFlyPlanFileTest::matchingEpochIsSeenAndAnotherPlanIsNot()
{
    const QTemporaryDir dir;
    WritePlan(PathOf(dir), OfpWithEpoch(kPlanEpoch));

    IFlyPlanImport matching;
    matching.Observe(PathOf(dir), kPlanEpoch);

    QVERIFY(matching.Seen());
    QVERIFY(!matching.Blind());

    IFlyPlanImport another;
    another.Observe(PathOf(dir), kPlanEpoch - 3600);

    QVERIFY(!another.Seen());
    QVERIFY(!another.Blind());
}

void IFlyPlanFileTest::latchSurvivesTheFileVanishing()
{
    const QTemporaryDir dir;
    WritePlan(PathOf(dir), OfpWithEpoch(kPlanEpoch));

    IFlyPlanImport watch;
    watch.Observe(PathOf(dir), kPlanEpoch);

    QVERIFY(watch.Seen());

    std::filesystem::remove(PathOf(dir) / kPlanFile);

    for (int look = 0; look < 10; ++look)
    {
        QVERIFY(watch.Seen());
    }

    watch.Observe(PathOf(dir), kPlanEpoch);

    QVERIFY(watch.Seen());
    QVERIFY(!watch.Blind());
}

void IFlyPlanFileTest::rearmsWhenThePlanChanges()
{
    const QTemporaryDir dir;
    WritePlan(PathOf(dir), OfpWithEpoch(kPlanEpoch));

    IFlyPlanImport watch;
    watch.Observe(PathOf(dir), kPlanEpoch);

    QVERIFY(watch.Seen());

    watch.Observe(PathOf(dir), kPlanEpoch + 7200);

    QVERIFY(!watch.Seen());
    QVERIFY(!watch.Blind());

    WritePlan(PathOf(dir), OfpWithEpoch(kPlanEpoch + 7200));
    watch.Observe(PathOf(dir), kPlanEpoch + 7200);

    QVERIFY(watch.Seen());
}

void IFlyPlanFileTest::blindUntilTheFirstLook()
{
    const IFlyPlanImport watch;

    QVERIFY(watch.Blind());
    QVERIFY(!watch.Seen());
}

void IFlyPlanFileTest::unresolvedFolderReleasesTheFlowAndSaysSoOnce()
{
    gPlanWarnings = 0;
    const QtMessageHandler previous = qInstallMessageHandler(CountPlanWarnings);

    IFlyPlanImport watch;
    watch.Observe(std::nullopt, kPlanEpoch);
    watch.Observe(std::nullopt, kPlanEpoch);
    watch.Observe(std::nullopt, kPlanEpoch + 7200);

    qInstallMessageHandler(previous);

    QCOMPARE(gPlanWarnings, 1);
    QVERIFY(watch.Blind());
    QVERIFY(!watch.Seen());
}

void IFlyPlanFileTest::unreadableFileAlsoReleasesTheFlow()
{
    const QTemporaryDir dir;
    WritePlan(PathOf(dir), "<?xml version=\"1.0\"?><OFP><params></params></OFP>");

    IFlyPlanImport watch;
    watch.Observe(PathOf(dir), kPlanEpoch);

    QVERIFY(watch.Blind());
    QVERIFY(!watch.Seen());
}

QTEST_APPLESS_MAIN(IFlyPlanFileTest)

#include "tst_ifly_plan_file.moc"
