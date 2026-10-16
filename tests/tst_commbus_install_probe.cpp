#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SimulatorCandidates.h"
#include "../src/infrastructure/update/CommbusInstallProbe.h"

class CommbusInstallProbeTest final : public QObject
{
    Q_OBJECT

private slots:
    static void parsesInstalledPackagesPath();
    static void rejectsUserCfgWithoutPackagesPath();
    static void parsesManifestVersion();
    static void rejectsInvalidManifest();
    static void detectsVersionFromOverrideDir();
    static void detectionReturnsEmptyWithoutManifest();
    static void listsEveryCommunitySiblingUnderThePackagesPath();
    static void listsNothingWhenThePackagesPathHasNoCommunityFolder();
    static void parsesBackslashPathsOfAnyDrive();
    static void candidatesAreTheFourSimulatorLocationsEachWithTheCommunityFallback();
    static void candidatesAddTheSiblingsTheUserCfgPointsTo();
    static void candidatesFollowAPrefixDriveLetterAndIgnoreCase();
    static void candidatesOfNoHomeAreEmpty();
};

void CommbusInstallProbeTest::parsesInstalledPackagesPath()
{
    const QByteArray userCfg =
        "Version 72\n"
        "InstalledPackagesPath \"C:\\MSFS Packages\"\n"
        "{Graphics\n}\n";

    QCOMPARE(ParseInstalledPackagesPath(userCfg), QStringLiteral("C:/MSFS Packages"));
}

void CommbusInstallProbeTest::rejectsUserCfgWithoutPackagesPath()
{
    QVERIFY(ParseInstalledPackagesPath("Version 72\n{Graphics\n}\n").isEmpty());
    QVERIFY(ParseInstalledPackagesPath({}).isEmpty());
    QVERIFY(ParseInstalledPackagesPath("InstalledPackagesPath\n").isEmpty());
}

void CommbusInstallProbeTest::parsesManifestVersion()
{
    QCOMPARE(ParseManifestVersion("{\"package_version\": \"0.2.1\"}"), QStringLiteral("0.2.1"));
    QCOMPARE(ParseManifestVersion("{\"package_version\": \" 1.0.0 \"}"), QStringLiteral("1.0.0"));
}

void CommbusInstallProbeTest::rejectsInvalidManifest()
{
    QVERIFY(ParseManifestVersion({}).isEmpty());
    QVERIFY(ParseManifestVersion("{not json").isEmpty());
    QVERIFY(ParseManifestVersion("{\"title\": \"GSX Integrator CommBus\"}").isEmpty());
    QVERIFY(ParseManifestVersion("{\"package_version\": \"abc\"}").isEmpty());
}

void CommbusInstallProbeTest::detectsVersionFromOverrideDir()
{
    const QTemporaryDir community;

    QVERIFY(community.isValid());

    QDir(community.path()).mkpath(QStringLiteral("gsx-integrator-commbus"));

    QFile manifest(community.path() + QStringLiteral("/gsx-integrator-commbus/manifest.json"));

    QVERIFY(manifest.open(QIODevice::WriteOnly));

    manifest.write("{\"package_version\": \"0.2.1\"}");
    manifest.close();

    QCOMPARE(DetectInstalledCommbusVersion(community.path()), QStringLiteral("0.2.1"));
}

void CommbusInstallProbeTest::detectionReturnsEmptyWithoutManifest()
{
    const QTemporaryDir community;

    QVERIFY(community.isValid());

    QVERIFY(DetectInstalledCommbusVersion(community.path()).isEmpty());
}

void CommbusInstallProbeTest::listsEveryCommunitySiblingUnderThePackagesPath()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());

    const QDir base(root.path());
    QVERIFY(base.mkdir(QStringLiteral("Community")));
    QVERIFY(base.mkdir(QStringLiteral("Community2024")));
    QVERIFY(base.mkdir(QStringLiteral("CommunityBackup")));
    QVERIFY(base.mkdir(QStringLiteral("Official")));

    const QStringList dirs = CommunityDirsUnder(root.path());

    QCOMPARE(dirs.size(), 3);
    QVERIFY(dirs.contains(base.filePath(QStringLiteral("Community"))));
    QVERIFY(dirs.contains(base.filePath(QStringLiteral("Community2024"))));
    QVERIFY(dirs.contains(base.filePath(QStringLiteral("CommunityBackup"))));
}

void CommbusInstallProbeTest::listsNothingWhenThePackagesPathHasNoCommunityFolder()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(QDir(root.path()).mkdir(QStringLiteral("Official")));

    QVERIFY(CommunityDirsUnder(root.path()).isEmpty());
    QVERIFY(CommunityDirsUnder(QString()).isEmpty());
}

void CommbusInstallProbeTest::parsesBackslashPathsOfAnyDrive()
{
    QCOMPARE(ParseInstalledPackagesPath("InstalledPackagesPath \"S:\\MSFS 2024\\Packages\"\n"),
             QStringLiteral("S:/MSFS 2024/Packages"));
}

void CommbusInstallProbeTest::candidatesAreTheFourSimulatorLocationsEachWithTheCommunityFallback()
{
    const QTemporaryDir home;

    QStringList expected;
    for (const SimulatorCandidate& candidate : kSimulatorCandidates)
    {
        expected.append(home.filePath(QLatin1String(candidate.baseSubPath)) + QStringLiteral("/Packages/Community"));
    }

    QCOMPARE(expected.size(), 4);
    QCOMPARE(CandidateCommunityDirsIn({home.path()}), expected);
}

void CommbusInstallProbeTest::candidatesAddTheSiblingsTheUserCfgPointsTo()
{
    const QTemporaryDir home;
    const QString packages = home.filePath(QStringLiteral("packages2024"));
    QDir().mkpath(packages + QStringLiteral("/Community"));
    QDir().mkpath(packages + QStringLiteral("/Community2"));
    FakeProtonTree::WriteFile(
        home.filePath(QStringLiteral("AppData/Roaming/Microsoft Flight Simulator 2024/UserCfg.opt")),
        "InstalledPackagesPath \"" + packages.toUtf8() + "\"\n");

    const QStringList dirs = CandidateCommunityDirsIn({home.path()});

    QVERIFY(dirs.contains(packages + QStringLiteral("/Community")));
    QVERIFY(dirs.contains(packages + QStringLiteral("/Community2")));
    QVERIFY(dirs.contains(home.filePath(QStringLiteral("AppData/Roaming/Microsoft Flight Simulator 2024/Packages/Community"))));
    QCOMPARE(dirs.size(), 6);
}

void CommbusInstallProbeTest::candidatesFollowAPrefixDriveLetterAndIgnoreCase()
{
#ifdef Q_OS_WIN
    QSKIP("Proton prefixes exist only on Linux");
#endif
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());
    QDir().mkpath(tree.secondLibrary + QStringLiteral("/Packages2024/community"));
    FakeProtonTree::WriteFile(
        tree.userHome2024 + QStringLiteral("/AppData/Roaming/Microsoft Flight Simulator 2024/usercfg.OPT"),
        "InstalledPackagesPath \"S:\\packages2024\"\n");

    const QStringList dirs = CandidateCommunityDirsIn({tree.userHome2024});

    QVERIFY(dirs.contains(QFileInfo(tree.secondLibrary).canonicalFilePath() + QStringLiteral("/Packages2024/community")));
}

void CommbusInstallProbeTest::candidatesOfNoHomeAreEmpty()
{
    QVERIFY(CandidateCommunityDirsIn({}).isEmpty());
}

QTEST_APPLESS_MAIN(CommbusInstallProbeTest)

#include "tst_commbus_install_probe.moc"
