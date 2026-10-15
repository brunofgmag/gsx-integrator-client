#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/CaseInsensitivePath.h"
#include "../src/infrastructure/simulator/ProtonPrefix.h"
#include "../src/infrastructure/simulator/SteamLibraries.h"

namespace
{
    QString Canonical(const QString& path)
    {
        return QFileInfo(path).canonicalFilePath();
    }
}

class ProtonPrefixTest final : public QObject
{
    Q_OBJECT

private slots:
    static void findsBothPrefixesNewestFirstAcrossLibraries();
    static void findsNothingWithoutSteam();
    static void aSimulatorInstalledOnlyOnTheSecondLibraryIsFound();
    static void knownFoldersAreTheSteamUserOnes();
    static void mapsAWindowsPathThroughTheDriveLetterOfThatPrefix();
    static void theSameLetterPointsToADifferentLibraryPerPrefix();
    static void resolvesEveryComponentIgnoringCase();
    static void aDriveWithoutLinkHasNoLinuxPath();
    static void aPathWithoutDriveLetterHasNoLinuxPath();
    static void recognisesTheUserHomeOfAPrefix();
    static void activePrefixIsTheOneWhoseSimulatorRuns();
    static void activePrefixFallsBackToTheFirstFound();
    static void noPrefixHasNoActiveOne();
    static void caseInsensitiveResolutionPrefersTheExactName();
    static void caseInsensitiveResolutionFindsDifferentCase();
    static void caseInsensitiveResolutionKeepsTheMissingTailLiteral();
};

void ProtonPrefixTest::findsBothPrefixesNewestFirstAcrossLibraries()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(tree.home));

    QCOMPARE(prefixes.size(), 2u);
    QCOMPARE(QLatin1String(prefixes[0].simulator->label), QLatin1String("MSFS 2024 (Steam)"));
    QCOMPARE(Canonical(prefixes[0].root), Canonical(tree.prefix2024));
    QCOMPARE(QLatin1String(prefixes[1].simulator->label), QLatin1String("MSFS 2020 (Steam)"));
    QCOMPARE(Canonical(prefixes[1].root), Canonical(tree.prefix2020));
}

void ProtonPrefixTest::findsNothingWithoutSteam()
{
    const QTemporaryDir home;

    QVERIFY(FindProtonPrefixes(DefaultSteamappsDirs(home.path())).empty());
}

void ProtonPrefixTest::aSimulatorInstalledOnlyOnTheSecondLibraryIsFound()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    QDir(tree.prefix2020).removeRecursively();

    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(tree.home));

    QCOMPARE(prefixes.size(), 1u);
    QCOMPARE(prefixes[0].simulator->steamAppId, 2537590u);
}

void ProtonPrefixTest::knownFoldersAreTheSteamUserOnes()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    const ProtonKnownFolders folders = ProtonKnownFoldersOf(tree.prefix2024);

    QCOMPARE(folders.home, tree.userHome2024);
    QCOMPARE(folders.roaming, tree.userHome2024 + QStringLiteral("/AppData/Roaming"));
    QCOMPARE(folders.local, tree.userHome2024 + QStringLiteral("/AppData/Local"));
}

void ProtonPrefixTest::mapsAWindowsPathThroughTheDriveLetterOfThatPrefix()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    QDir().mkpath(tree.secondLibrary + QStringLiteral("/Packages2024/Community"));

    QCOMPARE(Canonical(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("S:\\Packages2024\\Community"))),
             Canonical(tree.secondLibrary + QStringLiteral("/Packages2024/Community")));
    QCOMPARE(Canonical(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("c:/users/steamuser"))),
             Canonical(tree.userHome2024));
    QCOMPARE(Canonical(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("S:"))), Canonical(tree.secondLibrary));
}

void ProtonPrefixTest::theSameLetterPointsToADifferentLibraryPerPrefix()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    QDir().mkpath(tree.secondLibrary + QStringLiteral("/Packages"));
    QDir().mkpath(tree.defaultSteamapps + QStringLiteral("/Packages"));

    QCOMPARE(Canonical(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("S:\\Packages"))),
             Canonical(tree.secondLibrary + QStringLiteral("/Packages")));
    QCOMPARE(Canonical(ProtonPathFromWindows(tree.prefix2020, QStringLiteral("S:\\Packages"))),
             Canonical(tree.defaultSteamapps + QStringLiteral("/Packages")));
}

void ProtonPrefixTest::resolvesEveryComponentIgnoringCase()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    FakeProtonTree::WriteFile(tree.secondLibrary + QStringLiteral("/Packages2024/community/readme.txt"), "x");

    const QString resolved = ProtonPathFromWindows(tree.prefix2024, QStringLiteral("S:\\PACKAGES2024\\Community\\README.TXT"));

    QCOMPARE(Canonical(resolved), Canonical(tree.secondLibrary + QStringLiteral("/Packages2024/community/readme.txt")));
    QVERIFY(QFileInfo::exists(resolved));
}

void ProtonPrefixTest::aDriveWithoutLinkHasNoLinuxPath()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    QVERIFY(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("E:\\Flight Simulator 2024")).isEmpty());
}

void ProtonPrefixTest::aPathWithoutDriveLetterHasNoLinuxPath()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    QVERIFY(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("/home/u/packages")).isEmpty());
    QVERIFY(ProtonPathFromWindows(tree.prefix2024, QStringLiteral("packages")).isEmpty());
    QVERIFY(ProtonPathFromWindows(tree.prefix2024, {}).isEmpty());
}

void ProtonPrefixTest::recognisesTheUserHomeOfAPrefix()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    QCOMPARE(ProtonPrefixRootOfUserHome(tree.userHome2024), tree.prefix2024);
    QVERIFY(ProtonPrefixRootOfUserHome(tree.home).isEmpty());
    QVERIFY(ProtonPrefixRootOfUserHome(QStringLiteral("C:/Users/bruno")).isEmpty());
}

void ProtonPrefixTest::activePrefixIsTheOneWhoseSimulatorRuns()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(tree.home));

    const ProtonPrefix* active = ActiveProtonPrefix(prefixes, [](const QString& processName)
    {
        return processName == QStringLiteral("FlightSimulator.exe");
    });

    QVERIFY(active != nullptr);
    QCOMPARE(active->simulator->steamAppId, 1250410u);
}

void ProtonPrefixTest::activePrefixFallsBackToTheFirstFound()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(tree.home));

    const ProtonPrefix* active = ActiveProtonPrefix(prefixes, [](const QString&) { return false; });

    QVERIFY(active != nullptr);
    QCOMPARE(active->simulator->steamAppId, 2537590u);
}

void ProtonPrefixTest::noPrefixHasNoActiveOne()
{
    QVERIFY(ActiveProtonPrefix({}, [](const QString&) { return true; }) == nullptr);
}

void ProtonPrefixTest::caseInsensitiveResolutionPrefersTheExactName()
{
    const QTemporaryDir dir;
    FakeProtonTree::WriteFile(dir.filePath(QStringLiteral("EXE.xml")), "upper");
    FakeProtonTree::WriteFile(dir.filePath(QStringLiteral("exe.xml")), "lower");

    QCOMPARE(ResolveCaseInsensitive(dir.path(), QStringLiteral("exe.xml")), dir.filePath(QStringLiteral("exe.xml")));
    QCOMPARE(ResolveCaseInsensitive(dir.path(), QStringLiteral("EXE.xml")), dir.filePath(QStringLiteral("EXE.xml")));
}

void ProtonPrefixTest::caseInsensitiveResolutionFindsDifferentCase()
{
    const QTemporaryDir dir;
    FakeProtonTree::WriteFile(dir.filePath(QStringLiteral("Virtuali/Couatladdons.ini")), "x");

    QCOMPARE(ResolveCaseInsensitive(dir.path(), QStringLiteral("virtuali/CouatlAddons.ini")),
             dir.filePath(QStringLiteral("Virtuali/Couatladdons.ini")));
}

void ProtonPrefixTest::caseInsensitiveResolutionKeepsTheMissingTailLiteral()
{
    const QTemporaryDir dir;
    QDir().mkpath(dir.filePath(QStringLiteral("Roaming")));

    QCOMPARE(ResolveCaseInsensitive(dir.path(), QStringLiteral("roaming/New Folder/File.ini")),
             dir.filePath(QStringLiteral("Roaming/New Folder/File.ini")));
    QCOMPARE(ResolveCaseInsensitive(dir.path(), {}), dir.path());
}

QTEST_APPLESS_MAIN(ProtonPrefixTest)

#include "tst_proton_prefix.moc"
