#include <initializer_list>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/RunningProcess.h"

namespace
{
    QByteArray Cmdline(const std::initializer_list<const char*> arguments)
    {
        QByteArray cmdline;
        for (const char* argument : arguments)
        {
            cmdline.append(argument).append('\0');
        }

        return cmdline;
    }

    void AddProcess(const QString& procRoot, const QString& pid, const QByteArray& cmdline)
    {
        FakeProtonTree::WriteFile(procRoot + u'/' + pid + QStringLiteral("/cmdline"), cmdline);
    }
}

class RunningProcessTest final : public QObject
{
    Q_OBJECT

private slots:
    static void findsAWineProcessByTheExeInItsCommandLine();
    static void matchesTheBaseNameIgnoringCase();
    static void matchesAnyArgumentNotOnlyTheFirst();
    static void aLongerNameWithTheSameSuffixIsAnotherProgram();
    static void theTwoSimulatorsAreToldApart();
    static void ignoresEntriesThatAreNotProcesses();
    static void anEmptyOrMissingProcRootFindsNothing();
    static void seesTheTestProcessItselfInTheRealProcessList();
};

void RunningProcessTest::findsAWineProcessByTheExeInItsCommandLine()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("101"), Cmdline({"/usr/bin/steam", "-silent"}));
    AddProcess(proc.path(), QStringLiteral("202"),
               Cmdline({"S:\\Jogos\\SteamLibrary\\steamapps\\common\\MicrosoftFlightSimulator2024\\FlightSimulator2024.exe"}));

    QVERIFY(IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator2024.exe")));
}

void RunningProcessTest::matchesTheBaseNameIgnoringCase()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("7"), Cmdline({"C:/Games/flightsimulator2024.EXE"}));

    QVERIFY(IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator2024.exe")));
}

void RunningProcessTest::matchesAnyArgumentNotOnlyTheFirst()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("9"),
               Cmdline({"python3", "proton", "waitforexitandrun", "S:\\x\\FlightSimulator.exe"}));

    QVERIFY(IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator.exe")));
}

void RunningProcessTest::aLongerNameWithTheSameSuffixIsAnotherProgram()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("11"), Cmdline({"C:\\x\\NotFlightSimulator.exe"}));
    AddProcess(proc.path(), QStringLiteral("12"), Cmdline({"C:\\x\\FlightSimulator.exe.bak"}));

    QVERIFY(!IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator.exe")));
}

void RunningProcessTest::theTwoSimulatorsAreToldApart()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("30"), Cmdline({"S:\\x\\FlightSimulator2024.exe"}));

    QVERIFY(IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator2024.exe")));
    QVERIFY(!IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator.exe")));
}

void RunningProcessTest::ignoresEntriesThatAreNotProcesses()
{
    const QTemporaryDir proc;
    AddProcess(proc.path(), QStringLiteral("self"), Cmdline({"S:\\x\\FlightSimulator.exe"}));
    AddProcess(proc.path(), QStringLiteral("12ab"), Cmdline({"S:\\x\\FlightSimulator.exe"}));
    FakeProtonTree::WriteFile(proc.filePath(QStringLiteral("cmdline")), "S:\\x\\FlightSimulator.exe");

    QVERIFY(!IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator.exe")));
}

void RunningProcessTest::anEmptyOrMissingProcRootFindsNothing()
{
    const QTemporaryDir proc;
    QDir().mkpath(proc.filePath(QStringLiteral("55")));

    QVERIFY(!IsProcessRunningUnder(proc.path(), QStringLiteral("FlightSimulator.exe")));
    QVERIFY(!IsProcessRunningUnder(proc.filePath(QStringLiteral("missing")), QStringLiteral("FlightSimulator.exe")));
}

void RunningProcessTest::seesTheTestProcessItselfInTheRealProcessList()
{
    const QString self = QFileInfo(QCoreApplication::applicationFilePath()).fileName();

    QVERIFY(IsProcessRunning(self));
    QVERIFY(!IsProcessRunning(QStringLiteral("no-such-program-gsxi.exe")));
}

QTEST_GUILESS_MAIN(RunningProcessTest)

#include "tst_running_process.moc"
