#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SimulatorUserFolders.h"
#include "../src/infrastructure/simulator/SteamLibraries.h"

namespace
{
    class ScopedEnvironment
    {
    public:
        ScopedEnvironment(const char* name, const QByteArray& value)
            : name_(name),
              previous_(qgetenv(name))
        {
            qputenv(name_, value);
        }

        ScopedEnvironment(const ScopedEnvironment&) = delete;
        ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

        ~ScopedEnvironment()
        {
            qputenv(name_, previous_);
        }

    private:
        const char* name_;
        QByteArray previous_;
    };
}

class SimulatorUserFoldersTest final : public QObject
{
    Q_OBJECT

private slots:
    static void theEnvironmentVariableWinsWhenSet();
    static void anEmptyEnvironmentVariableIsAsUnset();
    static void theFilesystemPathKeepsNonAsciiNames();
    static void theActiveSimulatorsRoamingFolderIsUsedOnLinux();
    static void theFirstFoundPrefixIsUsedWhenNothingRuns();
    static void noSteamMeansNoRoamingFolder();
    static void userHomesAreOnePerFoundPrefix();
    static void aPathInsideAPrefixHomeGoesThroughTheDriveLetters();
    static void aPathOutsideAPrefixIsLeftAlone();
};

void SimulatorUserFoldersTest::theEnvironmentVariableWinsWhenSet()
{
    const QTemporaryDir appData;
    const ScopedEnvironment scoped("APPDATA", appData.path().toUtf8());

    QCOMPARE(SimulatorRoamingAppDataDir(), appData.path());
    QVERIFY(SimulatorRoamingAppData().has_value());
    QCOMPARE(QString::fromStdString(SimulatorRoamingAppData()->generic_string()), appData.path());
}

void SimulatorUserFoldersTest::anEmptyEnvironmentVariableIsAsUnset()
{
    const QTemporaryDir home;
    const ScopedEnvironment appData("APPDATA", "");
    const ScopedEnvironment homeFolder("HOME", home.path().toUtf8());

    QVERIFY(SimulatorRoamingAppDataDir().isEmpty());
    QVERIFY(!SimulatorRoamingAppData().has_value());
}

void SimulatorUserFoldersTest::theFilesystemPathKeepsNonAsciiNames()
{
    const QTemporaryDir parent;
    const QString appData = parent.filePath(QStringLiteral("Dados do Usu\u00e1rio"));
    const ScopedEnvironment scoped("APPDATA", appData.toUtf8());

    QCOMPARE(QString::fromUtf8(SimulatorRoamingAppData()->u8string().c_str()), appData);
}

void SimulatorUserFoldersTest::theActiveSimulatorsRoamingFolderIsUsedOnLinux()
{
#ifdef Q_OS_WIN
    QSKIP("Proton prefixes exist only on Linux");
#endif
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    const QString roaming = ActiveSimulatorRoamingAppData(tree.home, [](const QString& processName)
    {
        return processName == QStringLiteral("FlightSimulator.exe");
    });

    QCOMPARE(roaming, tree.userHome2020 + QStringLiteral("/AppData/Roaming"));
}

void SimulatorUserFoldersTest::theFirstFoundPrefixIsUsedWhenNothingRuns()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    const QString roaming = ActiveSimulatorRoamingAppData(tree.home, [](const QString&) { return false; });

    QCOMPARE(roaming, tree.userHome2024 + QStringLiteral("/AppData/Roaming"));
}

void SimulatorUserFoldersTest::noSteamMeansNoRoamingFolder()
{
    const QTemporaryDir home;

    QVERIFY(ActiveSimulatorRoamingAppData(home.path(), [](const QString&) { return true; }).isEmpty());
}

void SimulatorUserFoldersTest::userHomesAreOnePerFoundPrefix()
{
#ifdef Q_OS_WIN
    QSKIP("Proton prefixes exist only on Linux");
#endif
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());

    QCOMPARE(SimulatorUserHomes(tree.home), (QStringList{tree.userHome2024, tree.userHome2020}));
    QVERIFY(SimulatorUserHomes(home.path() + QStringLiteral("/nowhere")).isEmpty());
}

void SimulatorUserFoldersTest::aPathInsideAPrefixHomeGoesThroughTheDriveLetters()
{
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    QDir().mkpath(tree.secondLibrary + QStringLiteral("/Pkgs"));

    QCOMPARE(QFileInfo(SimulatorLocalPath(tree.userHome2024, QStringLiteral("S:/Pkgs"))).canonicalFilePath(),
             QFileInfo(tree.secondLibrary + QStringLiteral("/Pkgs")).canonicalFilePath());
}

void SimulatorUserFoldersTest::aPathOutsideAPrefixIsLeftAlone()
{
    QCOMPARE(SimulatorLocalPath(QStringLiteral("C:/Users/bruno"), QStringLiteral("E:/MSFS Packages")),
             QStringLiteral("E:/MSFS Packages"));
}

QTEST_GUILESS_MAIN(SimulatorUserFoldersTest)

#include "tst_simulator_user_folders.moc"
