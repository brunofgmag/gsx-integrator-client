#include <functional>
#include <signal.h>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/simulator/SimulatorWatcherFiles.h"
#include "../src/infrastructure/simulator/SystemdSimulatorLauncher.h"

namespace
{
    constexpr auto kScriptPath = "/home/u/.local/share/gsx-integrator-client/gsx-integrator-watcher.sh";

    bool WriteTestFile(const QString& path, const QByteArray& content)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        {
            return false;
        }

        QFile file(path);

        return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(content) == content.size();
    }

    QByteArray ReadTestFile(const QString& path)
    {
        QFile file(path);

        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    QByteArray Cmdline(const QList<QByteArray>& arguments)
    {
        QByteArray cmdline;
        for (const QByteArray& argument : arguments)
        {
            cmdline += argument;
            cmdline.append('\0');
        }

        return cmdline;
    }

    bool WaitFor(const std::function<bool()>& condition, const int timeoutMs = 8000)
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < timeoutMs)
        {
            if (condition())
            {
                return true;
            }
            QTest::qWait(25);
        }

        return condition();
    }

    int CountLines(const QString& path, const QString& needle)
    {
        int count = 0;
        for (const QString& line : QString::fromUtf8(ReadTestFile(path)).split(u'\n', Qt::SkipEmptyParts))
        {
            if (line.startsWith(needle))
            {
                ++count;
            }
        }

        return count;
    }

    QStringList BothSimulators()
    {
        return {QStringLiteral("FlightSimulator.exe"), QStringLiteral("FlightSimulator2024.exe")};
    }

    constexpr int kLongCommandLineArguments = 200000;

    QChar ProcessState(const QString& pid)
    {
        const QByteArray stat = ReadTestFile(QStringLiteral("/proc/%1/stat").arg(pid));
        const qsizetype end = stat.lastIndexOf(')');

        return end >= 0 && end + 2 < stat.size() ? QChar::fromLatin1(stat.at(end + 2)) : QChar();
    }

    bool IsAlive(const QString& pid)
    {
        const QChar state = ProcessState(pid);

        return !state.isNull() && state != u'Z';
    }

    bool IsReadingInsideOf(const qint64 pid, const QString& file, const qint64 size)
    {
        if (QFile::symLinkTarget(QStringLiteral("/proc/%1/fd/0").arg(pid)) != file)
        {
            return false;
        }

        static const QRegularExpression positionLine(QStringLiteral("^pos:\\s+(\\d+)$"),
                                                     QRegularExpression::MultilineOption);
        const QRegularExpressionMatch match = positionLine.match(
            QString::fromLatin1(ReadTestFile(QStringLiteral("/proc/%1/fdinfo/0").arg(pid))));
        const qint64 position = match.hasMatch() ? match.captured(1).toLongLong() : 0;

        return position > 0 && position < size;
    }

    bool FreezeWhileReadingInsideOf(const qint64 pid, const QString& file, const qint64 size)
    {
        if (!IsReadingInsideOf(pid, file, size))
        {
            return false;
        }

        ::kill(static_cast<pid_t>(pid), SIGSTOP);
        const QString watcher = QString::number(pid);
        const bool frozen = WaitFor([&] { return ProcessState(watcher) == u'T'; }, 2000);
        if (frozen && IsReadingInsideOf(pid, file, size))
        {
            return true;
        }
        ::kill(static_cast<pid_t>(pid), SIGCONT);

        return false;
    }

    bool CatchReadingInsideOf(const qint64 pid, const QString& file, const qint64 size)
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 15000)
        {
            if (FreezeWhileReadingInsideOf(pid, file, size))
            {
                return true;
            }
        }

        return false;
    }

    struct FakeSystemctl
    {
        QStringList calls;
        QStringList failing;
        bool answers = true;

        SystemctlRunner Runner()
        {
            return [this](const QStringList& arguments)
            {
                calls.append(arguments.join(u' '));
                if (!answers)
                {
                    return SystemctlOutcome{};
                }

                return SystemctlOutcome{.started = true, .exitCode = failing.contains(arguments.value(0)) ? 1 : 0};
            };
        }
    };

    struct Sandbox
    {
        QTemporaryDir dir;
        QString script = dir.filePath(QStringLiteral("data/gsx-integrator-client/gsx-integrator-watcher.sh"));
        QString installed = dir.filePath(QStringLiteral("data/gsx-integrator-client/gsx-integrator-client.AppImage"));
        QString unitDir = dir.filePath(QStringLiteral("config/systemd/user"));
        QString unit = unitDir + QStringLiteral("/gsx-integrator-client.service");
        FakeSystemctl systemctl;

        [[nodiscard]] SystemdSimulatorLauncher Launcher(const QString& client = {})
        {
            return SystemdSimulatorLauncher(
                {.unitDir = unitDir,
                 .scriptPath = script,
                 .clientPath = client.isEmpty() ? installed : client,
                 .installedClientPath = installed,
                 .processNames = BothSimulators()},
                systemctl.Runner());
        }
    };

    struct ScriptRun
    {
        QTemporaryDir dir;
        QString proc = dir.filePath(QStringLiteral("proc"));
        QString log = dir.filePath(QStringLiteral("client.log"));
        QString clientPid = dir.filePath(QStringLiteral("client.pid"));
        QProcess process;

        bool Start(const QString& simulators)
        {
            const QString scriptPath = dir.filePath(QStringLiteral("watcher.sh"));
            const QString clientPath = dir.filePath(QStringLiteral("fake-client.sh"));
            const SimulatorWatcherFiles files = WatcherFilesFor(scriptPath, {QStringLiteral("FlightSimulator2024.exe")});
            const QString client = QStringLiteral(
                "#!/bin/sh\n"
                "echo $$ > \"%2\"\n"
                "echo \"started $*\" >> \"%1\"\n"
                "trap 'echo stopped >> \"%1\"; exit 0' TERM\n"
                "while [ -d \"%3\" ]; do sleep 0.05; done\n").arg(log, clientPid, dir.path());

            const auto executable = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
            if (!QDir().mkpath(proc) || !WriteTestFile(scriptPath, files.script.toUtf8())
                || !WriteTestFile(clientPath, client.toUtf8())
                || !QFile::setPermissions(scriptPath, executable) || !QFile::setPermissions(clientPath, executable))
            {
                return false;
            }

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert(QStringLiteral("GSXI_PROC_ROOT"), proc);
            env.insert(QStringLiteral("GSXI_POLL_SECONDS"), QStringLiteral("0.05"));
            env.insert(QStringLiteral("GSXI_SIMULATORS"), simulators);
            env.insert(QStringLiteral("GSXI_CLIENT"), clientPath);
            process.setProcessEnvironment(env);
            process.start(scriptPath, {});

            return process.waitForStarted();
        }

        bool AddProcess(const QString& pid, const QByteArray& cmdline) const
        {
            return WriteTestFile(proc + u'/' + pid + QStringLiteral("/cmdline"), cmdline);
        }

        bool RemoveProcess(const QString& pid) const
        {
            return QDir(proc + u'/' + pid).removeRecursively();
        }

        bool LinkProcess(const QString& pid, const QString& cmdline) const
        {
            return QDir().mkpath(proc + u'/' + pid) && QFile::link(cmdline, proc + u'/' + pid + QStringLiteral("/cmdline"));
        }

        ~ScriptRun()
        {
            process.kill();
            process.waitForFinished(3000);
        }
    };
}

class SimulatorWatcherTest final : public QObject
{
    Q_OBJECT

private slots:
    static void unitTextIsExactlyWhatTheInstallerWrites();
    static void unitEscapesPercentDollarAndQuotesInTheScriptPath();
    static void unitKeepsSimulatorNamesOutOfTheScriptArguments();
    static void aStandaloneAppImageIsPassedToTheScriptByEnvironment();
    static void scriptTextNamesTheClientAndTheTrayFlag();
    static void enableWritesExecutableScriptAndUnitThenEnablesNow();
    static void enableTwiceWithTheSameFilesRestartsNothing();
    static void enablingAChangedUnitRestartsTheService();
    static void enableWithoutSystemdWritesNothingAndSaysUnavailable();
    static void enableFailureIsReported();
    static void disableStopsRemovesBothFilesAndReloads();
    static void disableFailureKeepsTheFilesForARetry();
    static void disableRemovesTheFilesEvenWhenSystemctlIsGone();
    static void enabledMeansTheUnitExistsAndSystemdSaysSo();
    static void defaultLocationsFollowTheXdgRule();
    static void scriptStartsTheClientOncePerSimulatorRunAndStopsItWithTheSimulator();
    static void scriptStartsTheClientWhenAnotherProcessOnlyMentionsItsName();
    static void scriptDoesNotCountItsOwnProcessAsTheSimulator();
    static void scriptRefusesToRunWithoutSimulators();
    static void scriptKeepsWatchingWhenAProcessDiesWhileItsCommandLineIsRead();
    static void theFakeClientDiesWithItsRun();
};

void SimulatorWatcherTest::unitTextIsExactlyWhatTheInstallerWrites()
{
    const SimulatorWatcherFiles files = WatcherFilesFor(QLatin1String(kScriptPath), BothSimulators());

    QCOMPARE(files.unit,
             QStringLiteral("[Unit]\n"
                            "Description=Start GSX Integrator with the simulator\n"
                            "\n"
                            "[Service]\n"
                            "Type=simple\n"
                            "Environment=\"GSXI_SIMULATORS=FlightSimulator.exe FlightSimulator2024.exe\"\n"
                            "ExecStart=\"/home/u/.local/share/gsx-integrator-client/gsx-integrator-watcher.sh\"\n"
                            "Restart=on-failure\n"
                            "RestartSec=5\n"
                            "\n"
                            "[Install]\n"
                            "WantedBy=default.target\n"));
}

void SimulatorWatcherTest::unitEscapesPercentDollarAndQuotesInTheScriptPath()
{
    const SimulatorWatcherFiles files = WatcherFilesFor(QStringLiteral("/home/100%/we\"ird/$x/w.sh"),
                                                        {QStringLiteral("FlightSimulator.exe")});

    QVERIFY(files.unit.contains(QStringLiteral("ExecStart=\"/home/100%%/we\\\"ird/$$x/w.sh\"\n")));
}

void SimulatorWatcherTest::unitKeepsSimulatorNamesOutOfTheScriptArguments()
{
    const SimulatorWatcherFiles files = WatcherFilesFor(QStringLiteral("/x/w.sh"), BothSimulators());

    QStringList execLines;
    for (const QString& line : files.unit.split(u'\n'))
    {
        if (line.startsWith(QStringLiteral("ExecStart=")))
        {
            execLines.append(line);
        }
    }

    QCOMPARE(execLines.size(), 1);
    QVERIFY(!execLines[0].contains(QStringLiteral("FlightSimulator")));
}

void SimulatorWatcherTest::aStandaloneAppImageIsPassedToTheScriptByEnvironment()
{
    const SimulatorWatcherFiles files = WatcherFilesFor(QLatin1String(kScriptPath), BothSimulators(),
                                                        QStringLiteral("/home/u/Downloads/gsx 100%.AppImage"));
    const SimulatorWatcherFiles plain = WatcherFilesFor(QLatin1String(kScriptPath), BothSimulators());

    QVERIFY(files.unit.contains(QStringLiteral(
        "Environment=\"GSXI_CLIENT=/home/u/Downloads/gsx 100%%.AppImage\"\nExecStart=")));
    QCOMPARE(QString(files.unit).remove(QRegularExpression(QStringLiteral("Environment=\"GSXI_CLIENT=[^\\n]*\\n"))),
             plain.unit);
    QCOMPARE(files.script, plain.script);
}

void SimulatorWatcherTest::scriptTextNamesTheClientAndTheTrayFlag()
{
    const SimulatorWatcherFiles files = WatcherFilesFor(QStringLiteral("/x/w.sh"), BothSimulators());

    QVERIFY(files.script.startsWith(QStringLiteral("#!/usr/bin/env bash\n")));
    QVERIFY(files.script.contains(QStringLiteral("client=\"${GSXI_CLIENT:-$here/gsx-integrator-client.AppImage}\"")));
    QVERIFY(files.script.contains(QStringLiteral("client_names=(gsx-integrator-client gsx-integrator-client.AppImage)")));
    QVERIFY(files.script.contains(QStringLiteral("setsid \"$client\" --tray")));
    QVERIFY(files.script.contains(QStringLiteral("read -r -a simulators <<<\"${GSXI_SIMULATORS:-}\"")));
}

void SimulatorWatcherTest::enableWritesExecutableScriptAndUnitThenEnablesNow()
{
    Sandbox box;
    SystemdSimulatorLauncher launcher = box.Launcher();

    const LaunchWithSimulatorResult result = launcher.Set(true);

    QVERIFY(!result.unavailable);
    QVERIFY(!result.serviceFailed);
    QVERIFY(QFileInfo(box.script).isExecutable());
    QVERIFY(!QFileInfo(box.unit).isExecutable());
    const SimulatorWatcherFiles expected = WatcherFilesFor(box.script, BothSimulators());
    QCOMPARE(QString::fromUtf8(ReadTestFile(box.unit)), expected.unit);
    QCOMPARE(QString::fromUtf8(ReadTestFile(box.script)), expected.script);

    const qsizetype show = box.systemctl.calls.indexOf(QStringLiteral("show-environment"));
    const qsizetype reload = box.systemctl.calls.indexOf(QStringLiteral("daemon-reload"));
    const qsizetype enable = box.systemctl.calls.indexOf(QStringLiteral("enable --now gsx-integrator-client.service"));
    QVERIFY(show >= 0);
    QVERIFY(reload > show);
    QVERIFY(enable > reload);
    QVERIFY(!box.systemctl.calls.contains(QStringLiteral("restart gsx-integrator-client.service")));
}

void SimulatorWatcherTest::enableTwiceWithTheSameFilesRestartsNothing()
{
    Sandbox box;
    SystemdSimulatorLauncher launcher = box.Launcher();

    QVERIFY(!launcher.Set(true).serviceFailed);
    QVERIFY(!launcher.Set(true).serviceFailed);

    QVERIFY(!box.systemctl.calls.contains(QStringLiteral("restart gsx-integrator-client.service")));
}

void SimulatorWatcherTest::enablingAChangedUnitRestartsTheService()
{
    Sandbox box;
    QVERIFY(!box.Launcher().Set(true).serviceFailed);

    QVERIFY(!box.Launcher(QStringLiteral("/home/u/Downloads/other.AppImage")).Set(true).serviceFailed);

    QVERIFY(QString::fromUtf8(ReadTestFile(box.unit)).contains(QStringLiteral("GSXI_CLIENT=/home/u/Downloads/other.AppImage")));
    QVERIFY(box.systemctl.calls.contains(QStringLiteral("restart gsx-integrator-client.service")));
}

void SimulatorWatcherTest::enableWithoutSystemdWritesNothingAndSaysUnavailable()
{
    Sandbox box;
    box.systemctl.answers = false;
    SystemdSimulatorLauncher launcher = box.Launcher();

    QVERIFY(!launcher.IsAvailable());
    const LaunchWithSimulatorResult result = launcher.Set(true);

    QVERIFY(result.unavailable);
    QVERIFY(!QFile::exists(box.unit));
    QVERIFY(!QFile::exists(box.script));
}

void SimulatorWatcherTest::enableFailureIsReported()
{
    Sandbox box;
    box.systemctl.failing = {QStringLiteral("enable")};

    QVERIFY(box.Launcher().Set(true).serviceFailed);
}

void SimulatorWatcherTest::disableStopsRemovesBothFilesAndReloads()
{
    Sandbox box;
    SystemdSimulatorLauncher launcher = box.Launcher();
    QVERIFY(!launcher.Set(true).serviceFailed);
    box.systemctl.calls.clear();

    QVERIFY(!launcher.Set(false).serviceFailed);

    QVERIFY(!QFile::exists(box.unit));
    QVERIFY(!QFile::exists(box.script));
    const qsizetype disable = box.systemctl.calls.indexOf(QStringLiteral("disable --now gsx-integrator-client.service"));
    QVERIFY(disable >= 0);
    QVERIFY(box.systemctl.calls.indexOf(QStringLiteral("daemon-reload"), disable) > disable);
}

void SimulatorWatcherTest::disableFailureKeepsTheFilesForARetry()
{
    Sandbox box;
    QVERIFY(!box.Launcher().Set(true).serviceFailed);
    box.systemctl.failing = {QStringLiteral("disable")};

    QVERIFY(box.Launcher().Set(false).serviceFailed);

    QVERIFY(QFile::exists(box.unit));
    QVERIFY(QFile::exists(box.script));
}

void SimulatorWatcherTest::disableRemovesTheFilesEvenWhenSystemctlIsGone()
{
    Sandbox box;
    QVERIFY(!box.Launcher().Set(true).serviceFailed);
    box.systemctl.answers = false;

    QVERIFY(!box.Launcher().Set(false).serviceFailed);

    QVERIFY(!QFile::exists(box.unit));
    QVERIFY(!QFile::exists(box.script));
}

void SimulatorWatcherTest::enabledMeansTheUnitExistsAndSystemdSaysSo()
{
    Sandbox box;
    SystemdSimulatorLauncher launcher = box.Launcher();
    QVERIFY(!launcher.IsEnabled());

    QVERIFY(!launcher.Set(true).serviceFailed);
    QVERIFY(launcher.IsEnabled());

    box.systemctl.failing = {QStringLiteral("is-enabled")};
    QVERIFY(!launcher.IsEnabled());
}

void SimulatorWatcherTest::defaultLocationsFollowTheXdgRule()
{
    qputenv("XDG_DATA_HOME", "/data/home");
    qputenv("XDG_CONFIG_HOME", "relative/config");
    qputenv("HOME", "/home/pilot");
    qunsetenv("APPIMAGE");

    const SimulatorWatcherLocations locations = DefaultWatcherLocations();

    QCOMPARE(locations.scriptPath, QStringLiteral("/data/home/gsx-integrator-client/gsx-integrator-watcher.sh"));
    QCOMPARE(locations.installedClientPath, QStringLiteral("/data/home/gsx-integrator-client/gsx-integrator-client.AppImage"));
    QCOMPARE(locations.unitDir, QStringLiteral("/home/pilot/.config/systemd/user"));
    QCOMPARE(locations.processNames, BothSimulators());

    qputenv("APPIMAGE", "/data/home/gsx-integrator-client/gsx-integrator-client.AppImage");
    QCOMPARE(DefaultWatcherLocations().clientPath, DefaultWatcherLocations().installedClientPath);
    qunsetenv("APPIMAGE");
}

void SimulatorWatcherTest::scriptStartsTheClientOncePerSimulatorRunAndStopsItWithTheSimulator()
{
    ScriptRun run;
    QVERIFY(run.Start(QStringLiteral("FlightSimulator2024.exe")));

    QTest::qWait(300);
    QCOMPARE(CountLines(run.log, QStringLiteral("started")), 0);

    QVERIFY(run.AddProcess(QStringLiteral("100"), Cmdline({"S:\\Steam\\MSFS2024\\FlightSimulator2024.exe"})));
    QVERIFY(WaitFor([&] { return CountLines(run.log, QStringLiteral("started")) == 1; }));
    QVERIFY(ReadTestFile(run.log).contains("started --tray"));

    QVERIFY(run.AddProcess(QStringLiteral("101"), Cmdline({"proton", "waitforexitandrun", "/x/FlightSimulator2024.exe"})));
    QTest::qWait(500);
    QCOMPARE(CountLines(run.log, QStringLiteral("started")), 1);
    QCOMPARE(CountLines(run.log, QStringLiteral("stopped")), 0);

    QVERIFY(run.RemoveProcess(QStringLiteral("100")));
    QVERIFY(run.RemoveProcess(QStringLiteral("101")));
    QVERIFY(WaitFor([&] { return CountLines(run.log, QStringLiteral("stopped")) == 1; }));

    QVERIFY(run.AddProcess(QStringLiteral("200"), Cmdline({"S:\\x\\FlightSimulator2024.exe"})));
    QVERIFY(WaitFor([&] { return CountLines(run.log, QStringLiteral("started")) == 2; }));
}

void SimulatorWatcherTest::scriptStartsTheClientWhenAnotherProcessOnlyMentionsItsName()
{
    ScriptRun run;
    QVERIFY(run.Start(QStringLiteral("FlightSimulator2024.exe")));
    QVERIFY(run.AddProcess(QStringLiteral("320"), Cmdline({"ninja", "gsx-integrator-client"})));
    QVERIFY(run.AddProcess(QStringLiteral("321"), Cmdline({"/usr/bin/tail", "-f", "/x/gsx-integrator-client.AppImage"})));

    QVERIFY(run.AddProcess(QStringLiteral("100"), Cmdline({"S:\\x\\FlightSimulator2024.exe"})));

    QVERIFY(WaitFor([&] { return CountLines(run.log, QStringLiteral("started")) == 1; }));
}

void SimulatorWatcherTest::scriptDoesNotCountItsOwnProcessAsTheSimulator()
{
    ScriptRun run;
    QVERIFY(run.Start(QStringLiteral("FlightSimulator2024.exe")));

    QVERIFY(run.AddProcess(QString::number(run.process.processId()),
                           Cmdline({"bash", "/x/watcher.sh", "FlightSimulator2024.exe"})));
    QTest::qWait(700);

    QCOMPARE(CountLines(run.log, QStringLiteral("started")), 0);
}

void SimulatorWatcherTest::scriptRefusesToRunWithoutSimulators()
{
    ScriptRun run;
    QVERIFY(run.Start(QString()));

    QVERIFY(run.process.waitForFinished(5000));
    QCOMPARE(run.process.exitCode(), 1);
}

void SimulatorWatcherTest::scriptKeepsWatchingWhenAProcessDiesWhileItsCommandLineIsRead()
{
    QStringList arguments{QStringLiteral("-c"), QStringLiteral("read -r _"), QStringLiteral("bash")};
    for (int i = 0; i < kLongCommandLineArguments; ++i)
    {
        arguments.append(QStringLiteral("x"));
    }
    QProcess dying;
    dying.start(QStringLiteral("bash"), arguments);
    QVERIFY(dying.waitForStarted());
    const QString dyingCmdline = QStringLiteral("/proc/%1/cmdline").arg(dying.processId());
    QVERIFY(WaitFor([&] { return ReadTestFile(dyingCmdline).count('x') == kLongCommandLineArguments; }));
    const qint64 size = ReadTestFile(dyingCmdline).size();

    ScriptRun run;
    QVERIFY(run.LinkProcess(QStringLiteral("150"), dyingCmdline));
    QVERIFY(run.Start(QStringLiteral("FlightSimulator2024.exe")));
    const qint64 watcher = run.process.processId();
    QVERIFY(CatchReadingInsideOf(watcher, dyingCmdline, size));

    dying.kill();
    const bool reaped = dying.waitForFinished(5000);
    ::kill(static_cast<pid_t>(watcher), SIGCONT);
    QVERIFY(reaped);

    QVERIFY(run.AddProcess(QStringLiteral("200"), Cmdline({"S:\\x\\FlightSimulator2024.exe"})));
    QVERIFY(WaitFor([&] { return CountLines(run.log, QStringLiteral("started")) == 1; }));
}

void SimulatorWatcherTest::theFakeClientDiesWithItsRun()
{
    QString clientPid;
    {
        ScriptRun run;
        QVERIFY(run.Start(QStringLiteral("FlightSimulator2024.exe")));
        QVERIFY(run.AddProcess(QStringLiteral("100"), Cmdline({"S:\\x\\FlightSimulator2024.exe"})));
        QVERIFY(WaitFor([&] { return !ReadTestFile(run.clientPid).trimmed().isEmpty(); }));
        clientPid = QString::fromLatin1(ReadTestFile(run.clientPid).trimmed());
        QVERIFY(IsAlive(clientPid));
    }

    QVERIFY(WaitFor([&] { return !IsAlive(clientPid); }));
}

QTEST_GUILESS_MAIN(SimulatorWatcherTest)

#include "tst_simulator_watcher.moc"
