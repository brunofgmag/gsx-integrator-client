#include <iterator>
#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SimulatorCandidates.h"

class SimulatorCandidatesTest final : public QObject
{
    Q_OBJECT

private slots:
    static void listsTheStoreAndSteamEditionsOfBothSimulators();
    static void onlyTheSteamEditionsHaveAnAppId();
    static void userCfgAndFallbackSitInTheBaseDir();
    static void aFolderOfAnotherCaseIsFoundOnCaseSensitiveFileSystems();
    static void aMissingFolderKeepsTheNameItWasGiven();
};

void SimulatorCandidatesTest::listsTheStoreAndSteamEditionsOfBothSimulators()
{
    QCOMPARE(std::size(kSimulatorCandidates), 4u);
    QCOMPARE(QLatin1String(kSimulatorCandidates[0].label), QLatin1String("MSFS 2020 (Steam)"));
    QCOMPARE(QLatin1String(kSimulatorCandidates[1].label), QLatin1String("MSFS 2020 (Microsoft Store)"));
    QCOMPARE(QLatin1String(kSimulatorCandidates[2].label), QLatin1String("MSFS 2024 (Steam)"));
    QCOMPARE(QLatin1String(kSimulatorCandidates[3].label), QLatin1String("MSFS 2024 (Microsoft Store)"));
    QCOMPARE(QLatin1String(kSimulatorCandidates[0].processName), QLatin1String("FlightSimulator.exe"));
    QCOMPARE(QLatin1String(kSimulatorCandidates[3].processName), QLatin1String("FlightSimulator2024.exe"));
}

void SimulatorCandidatesTest::onlyTheSteamEditionsHaveAnAppId()
{
    QCOMPARE(kSimulatorCandidates[0].steamAppId, 1250410u);
    QCOMPARE(kSimulatorCandidates[1].steamAppId, 0u);
    QCOMPARE(kSimulatorCandidates[2].steamAppId, 2537590u);
    QCOMPARE(kSimulatorCandidates[3].steamAppId, 0u);
}

void SimulatorCandidatesTest::userCfgAndFallbackSitInTheBaseDir()
{
    const QTemporaryDir home;
    const SimulatorCandidate& steam2024 = kSimulatorCandidates[2];
    const QString base = home.filePath(QStringLiteral("AppData/Roaming/Microsoft Flight Simulator 2024"));

    QCOMPARE(SimulatorBaseDir(home.path(), steam2024), base);
    QCOMPARE(SimulatorUserCfgPath(home.path(), steam2024), base + QStringLiteral("/UserCfg.opt"));
    QCOMPARE(SimulatorFallbackCommunityDir(home.path(), steam2024), base + QStringLiteral("/Packages/Community"));
}

void SimulatorCandidatesTest::aFolderOfAnotherCaseIsFoundOnCaseSensitiveFileSystems()
{
#ifdef Q_OS_WIN
    QSKIP("the file system already ignores case on Windows");
#endif
    const QTemporaryDir home;
    const QString base = home.filePath(QStringLiteral("appdata/Roaming/microsoft flight simulator 2024"));
    FakeProtonTree::WriteFile(base + QStringLiteral("/usercfg.OPT"), "x");
    QDir().mkpath(base + QStringLiteral("/PACKAGES/community"));

    QCOMPARE(SimulatorBaseDir(home.path(), kSimulatorCandidates[2]), base);
    QCOMPARE(SimulatorUserCfgPath(home.path(), kSimulatorCandidates[2]), base + QStringLiteral("/usercfg.OPT"));
    QCOMPARE(SimulatorFallbackCommunityDir(home.path(), kSimulatorCandidates[2]),
             base + QStringLiteral("/PACKAGES/community"));
}

void SimulatorCandidatesTest::aMissingFolderKeepsTheNameItWasGiven()
{
    const QTemporaryDir home;

    QCOMPARE(SimulatorUserCfgPath(home.path(), kSimulatorCandidates[1]),
             home.filePath(QStringLiteral("AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/UserCfg.opt")));
}

QTEST_APPLESS_MAIN(SimulatorCandidatesTest)

#include "tst_simulator_candidates.moc"
