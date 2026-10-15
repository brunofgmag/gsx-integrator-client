#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SteamLibraries.h"

namespace
{
    QByteArray Fixture(const char* name)
    {
        return FakeProtonTree::ReadFile(QStringLiteral(GSX_FIXTURES_DIR "/") + QLatin1String(name));
    }
}

class SteamLibrariesTest final : public QObject
{
    Q_OBJECT

private slots:
    static void readsEveryLibraryPathOfARealFile();
    static void ignoresPathKeysBelowTheLibraryEntry();
    static void unescapesBackslashesAndQuotes();
    static void skipsLineComments();
    static void emptyOrBrokenInputYieldsNothing();
    static void defaultDirsAreTheTwoSteamLocationsUnderHome();
    static void listsTheStartingDirsAndEveryLibraryOnce();
    static void dropsLibrariesThatDoNotExist();
};

void SteamLibrariesTest::readsEveryLibraryPathOfARealFile()
{
    const QStringList paths = ParseSteamLibraryPaths(Fixture("libraryfolders.vdf"));

    QCOMPARE(paths, (QStringList{QStringLiteral("/home/bruno/.local/share/Steam"),
                                 QStringLiteral("/home/bruno/Jogos/SteamLibrary")}));
}

void SteamLibrariesTest::ignoresPathKeysBelowTheLibraryEntry()
{
    const QByteArray vdf =
        "\"libraryfolders\"\n{\n\t\"0\"\n\t{\n\t\t\"path\"\t\t\"/a\"\n"
        "\t\t\"apps\"\n\t\t{\n\t\t\t\"path\"\t\t\"/not-a-library\"\n\t\t}\n\t}\n}\n";

    QCOMPARE(ParseSteamLibraryPaths(vdf), QStringList{QStringLiteral("/a")});
}

void SteamLibrariesTest::unescapesBackslashesAndQuotes()
{
    const QByteArray vdf = "\"libraryfolders\"\n{\n\t\"0\"\n\t{\n\t\t\"PATH\"\t\t\"D:\\\\Games\\\\\\\"Steam\\\"\"\n\t}\n}\n";

    QCOMPARE(ParseSteamLibraryPaths(vdf), QStringList{QStringLiteral("D:\\Games\\\"Steam\"")});
}

void SteamLibrariesTest::skipsLineComments()
{
    const QByteArray vdf =
        "// header\n\"libraryfolders\"\n{\n\t\"0\"\n\t{\n\t\t// \"path\" \"/commented\"\n"
        "\t\t\"path\"\t\t\"/real\"\n\t}\n}\n";

    QCOMPARE(ParseSteamLibraryPaths(vdf), QStringList{QStringLiteral("/real")});
}

void SteamLibrariesTest::emptyOrBrokenInputYieldsNothing()
{
    QVERIFY(ParseSteamLibraryPaths({}).isEmpty());
    QVERIFY(ParseSteamLibraryPaths("\"libraryfolders\" {").isEmpty());
    QVERIFY(ParseSteamLibraryPaths("\"path\" \"/top-level\"").isEmpty());
}

void SteamLibrariesTest::defaultDirsAreTheTwoSteamLocationsUnderHome()
{
    QCOMPARE(DefaultSteamappsDirs(QStringLiteral("/home/u")),
             (QStringList{QStringLiteral("/home/u/.local/share/Steam/steamapps"),
                          QStringLiteral("/home/u/.steam/steam/steamapps")}));
}

void SteamLibrariesTest::listsTheStartingDirsAndEveryLibraryOnce()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    const QStringList libraries = SteamLibrarySteamappsDirs(DefaultSteamappsDirs(tree.home));

    QCOMPARE(libraries, (QStringList{QFileInfo(tree.defaultSteamapps).canonicalFilePath(),
                                     QFileInfo(tree.secondLibrary + QStringLiteral("/steamapps")).canonicalFilePath()}));
}

void SteamLibrariesTest::dropsLibrariesThatDoNotExist()
{
    const QTemporaryDir home;
    FakeProtonTree::WriteFile(home.path() + QStringLiteral("/steamapps/libraryfolders.vdf"),
                              "\"libraryfolders\"\n{\n\"0\"\n{\n\"path\"\t\"/nowhere/at/all\"\n}\n}\n");

    const QStringList libraries = SteamLibrarySteamappsDirs(
        {home.path() + QStringLiteral("/steamapps"), home.path() + QStringLiteral("/missing/steamapps")});

    QCOMPARE(libraries, QStringList{QFileInfo(home.path() + QStringLiteral("/steamapps")).canonicalFilePath()});
}

QTEST_APPLESS_MAIN(SteamLibrariesTest)

#include "tst_steam_libraries.moc"
