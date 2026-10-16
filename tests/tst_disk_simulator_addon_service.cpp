#include <memory>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/simulator/DiskSimulatorAddonService.h"

namespace
{
    constexpr auto kSteam2024UserCfg = "AppData/Roaming/Microsoft Flight Simulator 2024/UserCfg.opt";
    constexpr auto kStore2020UserCfg = "AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/UserCfg.opt";
    constexpr auto kClientExePath = "C:\\Apps\\GSX Integrator\\gsx-integrator-client.exe";

    void WriteFile(const QString& path, const QByteArray& content)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(content);
    }

    QByteArray ReadFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return file.readAll();
    }

    void MakeBundle(const QString& bundleDir, const QString& version)
    {
        WriteFile(bundleDir + QStringLiteral("/manifest.json"),
                  QStringLiteral("{\"package_version\": \"%1\"}").arg(version).toUtf8());
        WriteFile(bundleDir + QStringLiteral("/SimObjects/bridge.wasm"), "wasm " + version.toUtf8());
    }

    QString AddSimulator(const QString& home, const char* userCfgSubPath, const QString& packagesName)
    {
        const QString packages = home + u'/' + packagesName;
        const QString community = packages + QStringLiteral("/Community");
        QDir().mkpath(community);
        WriteFile(home + u'/' + QLatin1String(userCfgSubPath),
                  "InstalledPackagesPath \"" + QDir::toNativeSeparators(packages).toUtf8() + "\"\n");

        return community;
    }

    QString PackageDir(const QString& community)
    {
        return community + QStringLiteral("/gsx-integrator-commbus");
    }

    QString ExeXmlNextTo(const QString& home, const char* userCfgSubPath)
    {
        return QFileInfo(home + u'/' + QLatin1String(userCfgSubPath)).absolutePath() + QStringLiteral("/EXE.xml");
    }

    DiskSimulatorAddonService MakeService(const QString& root, const ProcessRunningCheck& isRunning,
                                          const QString& overrideDir = {})
    {
        return {
            {
                root + QStringLiteral("/bundle"), overrideDir, root + QStringLiteral("/home"),
                QLatin1String(kClientExePath)
            },
            isRunning
        };
    }

    class RecordingLauncher final : public SimulatorLauncher
    {
    public:
        [[nodiscard]] bool IsAvailable() const override { return available; }
        [[nodiscard]] bool IsEnabled() const override { return enabled; }

        [[nodiscard]] LaunchWithSimulatorResult Set(const bool value) override
        {
            requests.append(value);
            enabled = value;

            return {};
        }

        bool available = true;
        bool enabled = false;
        QList<bool> requests;
    };

    bool NothingRunning(const QString&)
    {
        return false;
    }

    bool Running2024(const QString& processName)
    {
        return processName == QStringLiteral("FlightSimulator2024.exe");
    }
}

class DiskSimulatorAddonServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void launchWithSimulatorWritesAndRemovesEveryCandidate();
    static void launchWithSimulatorKeepsOtherAddons();
    static void launchWithSimulatorWithoutSimulatorWritesNothing();
    static void removeIsRefusedWhileTheSimulatorRuns();
    static void removeRefusalTouchesNoOtherTarget();
    static void removeDeletesThePackage();
    static void removeUnlinksAJunctionWithoutTouchingItsTarget();
    static void removeReportsTargetsWithoutThePackageAsAbsent();
    static void removeFromTheOverrideDirIgnoresTheProcessCheck();
    static void enableIsRefusedWhileTheSimulatorRunsAndTheInstallIsOutdated();
    static void reEnableInstallsThePackageAgain();
    static void commbusTargetsSpanEveryUserHome();
    static void aTargetFoundThroughTwoHomesIsInstalledOnce();
    static void aLauncherReplacesTheExeXmlForTheStartWithTheSimulator();
    static void withoutALauncherLaunchingIsAlwaysAvailable();
};

void DiskSimulatorAddonServiceTest::launchWithSimulatorWritesAndRemovesEveryCandidate()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    AddSimulator(home, kStore2020UserCfg, QStringLiteral("packages2020"));
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    QVERIFY(!service.IsLaunchedWithSimulator());

    const LaunchWithSimulatorResult enabled = service.SetLaunchedWithSimulator(true);

    QVERIFY(!enabled.noSimulator);
    QVERIFY(enabled.failedTargets.isEmpty());
    QVERIFY(service.IsLaunchedWithSimulator());
    for (const char* userCfg : {kSteam2024UserCfg, kStore2020UserCfg})
    {
        const QString content = QString::fromUtf8(ReadFile(ExeXmlNextTo(home, userCfg)));
        QVERIFY(content.contains(QStringLiteral("<Path>C:\\Apps\\GSX Integrator\\gsx-integrator-client.exe</Path>")));
        QVERIFY(content.contains(QStringLiteral("<Name>GSX Integrator</Name>")));
        QVERIFY(content.contains(QStringLiteral("<CommandLine>--tray</CommandLine>")));
    }

    const LaunchWithSimulatorResult disabled = service.SetLaunchedWithSimulator(false);

    QVERIFY(disabled.failedTargets.isEmpty());
    QVERIFY(!service.IsLaunchedWithSimulator());
    for (const char* userCfg : {kSteam2024UserCfg, kStore2020UserCfg})
    {
        QVERIFY(!ReadFile(ExeXmlNextTo(home, userCfg)).contains("gsx-integrator-client.exe"));
    }
}

void DiskSimulatorAddonServiceTest::launchWithSimulatorKeepsOtherAddons()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    const QString exeXml = ExeXmlNextTo(home, kSteam2024UserCfg);
    WriteFile(exeXml,
              "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
              "<SimBase.Document Type=\"Launch\" version=\"1,0\">\n"
              "  <Launch.Addon>\n"
              "    <Name>Other Tool</Name>\n"
              "    <Path>C:\\Other\\other-tool.exe</Path>\n"
              "  </Launch.Addon>\n"
              "</SimBase.Document>\n");
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    QVERIFY(service.SetLaunchedWithSimulator(true).failedTargets.isEmpty());
    QVERIFY(service.SetLaunchedWithSimulator(false).failedTargets.isEmpty());

    const QByteArray content = ReadFile(exeXml);

    QVERIFY(content.contains("other-tool.exe"));
    QVERIFY(!content.contains("gsx-integrator-client.exe"));
}

void DiskSimulatorAddonServiceTest::launchWithSimulatorWithoutSimulatorWritesNothing()
{
    const QTemporaryDir root;
    QDir().mkpath(root.path() + QStringLiteral("/home"));
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    const LaunchWithSimulatorResult result = service.SetLaunchedWithSimulator(true);

    QVERIFY(result.noSimulator);
    QVERIFY(!service.IsLaunchedWithSimulator());
    QCOMPARE(QDir(root.path() + QStringLiteral("/home")).entryList(QDir::NoDotAndDotDot | QDir::AllEntries),
             QStringList{});
}

void DiskSimulatorAddonServiceTest::removeIsRefusedWhileTheSimulatorRuns()
{
    const QTemporaryDir root;
    const QString community = AddSimulator(root.path() + QStringLiteral("/home"), kSteam2024UserCfg,
                                           QStringLiteral("packages2024"));
    MakeBundle(PackageDir(community), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service = MakeService(root.path(), Running2024);

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].label, QStringLiteral("MSFS 2024 (Steam)"));
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::SimRunning);
    QVERIFY(QFile::exists(PackageDir(community) + QStringLiteral("/SimObjects/bridge.wasm")));
}

void DiskSimulatorAddonServiceTest::removeRefusalTouchesNoOtherTarget()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    const QString community2024 = AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    const QString community2020 = AddSimulator(home, kStore2020UserCfg, QStringLiteral("packages2020"));
    MakeBundle(PackageDir(community2024), QStringLiteral("0.4.0"));
    MakeBundle(PackageDir(community2020), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service = MakeService(root.path(), Running2024);

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::SimRunning);
    QVERIFY(QDir(PackageDir(community2020)).exists());
    QVERIFY(QDir(PackageDir(community2024)).exists());
}

void DiskSimulatorAddonServiceTest::removeDeletesThePackage()
{
    const QTemporaryDir root;
    const QString community = AddSimulator(root.path() + QStringLiteral("/home"), kSteam2024UserCfg,
                                           QStringLiteral("packages2024"));
    MakeBundle(PackageDir(community), QStringLiteral("0.4.0"));
    WriteFile(community + QStringLiteral("/other-addon/manifest.json"), "{}");
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Removed);
    QVERIFY(!QFileInfo::exists(PackageDir(community)));
    QVERIFY(QFile::exists(community + QStringLiteral("/other-addon/manifest.json")));
}

void DiskSimulatorAddonServiceTest::removeUnlinksAJunctionWithoutTouchingItsTarget()
{
#ifndef Q_OS_WIN
    QSKIP("mklink /J exists only on Windows");
#endif
    const QTemporaryDir root;
    const QString community = AddSimulator(root.path() + QStringLiteral("/home"), kSteam2024UserCfg,
                                           QStringLiteral("packages2024"));
    const QString devCheckout = root.path() + QStringLiteral("/dev-checkout");
    MakeBundle(devCheckout, QStringLiteral("0.3.2"));

    const int mklink = QProcess::execute(QStringLiteral("cmd.exe"), {
        QStringLiteral("/c"), QStringLiteral("mklink"), QStringLiteral("/J"),
        QDir::toNativeSeparators(PackageDir(community)), QDir::toNativeSeparators(devCheckout)
    });

    QCOMPARE(mklink, 0);

    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Removed);
    QVERIFY(!QFileInfo(PackageDir(community)).isJunction());
    QVERIFY(!QFileInfo::exists(PackageDir(community)));
    QVERIFY(QFile::exists(devCheckout + QStringLiteral("/SimObjects/bridge.wasm")));
}

void DiskSimulatorAddonServiceTest::removeReportsTargetsWithoutThePackageAsAbsent()
{
    const QTemporaryDir root;
    AddSimulator(root.path() + QStringLiteral("/home"), kSteam2024UserCfg, QStringLiteral("packages2024"));
    DiskSimulatorAddonService service = MakeService(root.path(), Running2024);

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Absent);
}

void DiskSimulatorAddonServiceTest::removeFromTheOverrideDirIgnoresTheProcessCheck()
{
    const QTemporaryDir root;
    const QString community = root.path() + QStringLiteral("/qa-community");
    MakeBundle(PackageDir(community), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service = MakeService(root.path(), [](const QString&) { return true; },
                                                    QDir::toNativeSeparators(community));

    const CommbusBundleResult result = service.RemoveCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Removed);
    QVERIFY(!QFileInfo::exists(PackageDir(community)));
}

void DiskSimulatorAddonServiceTest::enableIsRefusedWhileTheSimulatorRunsAndTheInstallIsOutdated()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    const QString community2024 = AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    const QString community2020 = AddSimulator(home, kStore2020UserCfg, QStringLiteral("packages2020"));
    MakeBundle(root.path() + QStringLiteral("/bundle"), QStringLiteral("0.4.0"));
    MakeBundle(PackageDir(community2024), QStringLiteral("0.3.2"));
    DiskSimulatorAddonService service = MakeService(root.path(), Running2024);

    const CommbusBundleResult result = service.EnableCommbus();

    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].label, QStringLiteral("MSFS 2024 (Steam)"));
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::SimRunning);
    QVERIFY(!QDir(PackageDir(community2020)).exists());
    QCOMPARE(ReadFile(PackageDir(community2024) + QStringLiteral("/SimObjects/bridge.wasm")),
             QByteArray("wasm 0.3.2"));
}

void DiskSimulatorAddonServiceTest::reEnableInstallsThePackageAgain()
{
    const QTemporaryDir root;
    const QString community = AddSimulator(root.path() + QStringLiteral("/home"), kSteam2024UserCfg,
                                           QStringLiteral("packages2024"));
    MakeBundle(root.path() + QStringLiteral("/bundle"), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    QCOMPARE(service.InstallCommbus().targets[0].status, CommbusBundleStatus::Installed);
    QCOMPARE(service.RemoveCommbus().targets[0].status, CommbusBundleStatus::Removed);
    QVERIFY(!QFileInfo::exists(PackageDir(community)));

    const CommbusBundleResult result = service.EnableCommbus();

    QCOMPARE(result.bundledVersion, QStringLiteral("0.4.0"));
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Installed);
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/SimObjects/bridge.wasm")), QByteArray("wasm 0.4.0"));
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/.gsxi-version")), QByteArray("0.4.0"));
}

void DiskSimulatorAddonServiceTest::commbusTargetsSpanEveryUserHome()
{
    const QTemporaryDir root;
    const QString homeA = root.path() + QStringLiteral("/prefix2024/home");
    const QString homeB = root.path() + QStringLiteral("/prefix2020/home");
    const QString communityA = AddSimulator(homeA, kSteam2024UserCfg, QStringLiteral("packages2024"));
    const QString communityB = AddSimulator(homeB, kStore2020UserCfg, QStringLiteral("packages2020"));
    MakeBundle(root.path() + QStringLiteral("/bundle"), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service({root.path() + QStringLiteral("/bundle"), {}, root.path(),
                                       QLatin1String(kClientExePath), {homeA, homeB}},
                                      NothingRunning);

    const CommbusBundleResult result = service.InstallCommbus();

    QCOMPARE(result.targets.size(), std::size_t{2});
    QVERIFY(QFileInfo::exists(PackageDir(communityA) + QStringLiteral("/manifest.json")));
    QVERIFY(QFileInfo::exists(PackageDir(communityB) + QStringLiteral("/manifest.json")));
}

void DiskSimulatorAddonServiceTest::aTargetFoundThroughTwoHomesIsInstalledOnce()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    MakeBundle(root.path() + QStringLiteral("/bundle"), QStringLiteral("0.4.0"));
    DiskSimulatorAddonService service({root.path() + QStringLiteral("/bundle"), {}, root.path(),
                                       QLatin1String(kClientExePath), {home, home}},
                                      NothingRunning);

    QCOMPARE(service.InstallCommbus().targets.size(), std::size_t{1});
}

void DiskSimulatorAddonServiceTest::aLauncherReplacesTheExeXmlForTheStartWithTheSimulator()
{
    const QTemporaryDir root;
    const QString home = root.path() + QStringLiteral("/home");
    AddSimulator(home, kSteam2024UserCfg, QStringLiteral("packages2024"));
    auto launcher = std::make_unique<RecordingLauncher>();
    RecordingLauncher* recorded = launcher.get();
    DiskSimulatorAddonService service({root.path() + QStringLiteral("/bundle"), {}, home, QLatin1String(kClientExePath)},
                                      NothingRunning, std::move(launcher));

    QVERIFY(!service.IsLaunchedWithSimulator());
    const LaunchWithSimulatorResult result = service.SetLaunchedWithSimulator(true);

    QVERIFY(!result.noSimulator);
    QVERIFY(service.IsLaunchedWithSimulator());
    QCOMPARE(recorded->requests, QList<bool>{true});
    QVERIFY(!QFileInfo::exists(ExeXmlNextTo(home, kSteam2024UserCfg)));

    recorded->available = false;

    QVERIFY(!service.CanLaunchWithSimulator());
}

void DiskSimulatorAddonServiceTest::withoutALauncherLaunchingIsAlwaysAvailable()
{
    const QTemporaryDir root;
    DiskSimulatorAddonService service = MakeService(root.path(), NothingRunning);

    QVERIFY(service.CanLaunchWithSimulator());
}

QTEST_GUILESS_MAIN(DiskSimulatorAddonServiceTest)

#include "tst_disk_simulator_addon_service.moc"
