#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/update/CommbusBundleInstaller.h"

namespace
{
    constexpr auto kSimProcess = "FlightSimulator2024.exe";

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

    QByteArray Manifest(const QString& version)
    {
        return QStringLiteral("{\"package_version\": \"%1\"}").arg(version).toUtf8();
    }

    void MakeBundle(const QString& bundleDir, const QString& version)
    {
        WriteFile(bundleDir + QStringLiteral("/manifest.json"), Manifest(version));
        WriteFile(bundleDir + QStringLiteral("/layout.json"), "{\"content\": []}");
        WriteFile(bundleDir + QStringLiteral("/SimObjects/bridge.wasm"), "wasm " + version.toUtf8());
    }

    QString PackageDir(const QString& communityPath)
    {
        return communityPath + QStringLiteral("/gsx-integrator-commbus");
    }

    std::vector<CommbusInstallTarget> OneTarget(const QString& communityPath)
    {
        return {{QStringLiteral("MSFS 2024 (Steam)"), communityPath, QLatin1String(kSimProcess)}};
    }

    bool NothingRunning(const QString&)
    {
        return false;
    }
}

class CommbusBundleInstallerTest final : public QObject
{
    Q_OBJECT

private slots:
    static void freshInstallCopiesTheBundleAndWritesTheMarker();
    static void olderInstallIsReplacedAndMarked();
    static void equalOrNewerInstallIsLeftUntouched();
    static void runningSimulatorSkipsTheTarget();
    static void upToDateTargetIsNotBlockedByTheRunningSimulator();
    static void junctionIsUnlinkedWithoutTouchingItsTarget();
    static void bundleWithoutManifestFailsEveryTarget();
    static void installedVersionPrefersTheMarkerOverTheManifest();
    static void detectsTargetsWhoseCommunityFolderExists();
    static void overrideDirIsTheOnlyTargetAndHasNoProcess();
};

void CommbusBundleInstallerTest::freshInstallCopiesTheBundleAndWritesTheMarker()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/Community");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    QDir().mkpath(community);

    const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community), NothingRunning);

    QCOMPARE(result.bundledVersion, QStringLiteral("0.4.0"));
    QCOMPARE(result.targets.size(), 1u);
    QCOMPARE(result.targets[0].label, QStringLiteral("MSFS 2024 (Steam)"));
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Installed);
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/SimObjects/bridge.wasm")), QByteArray("wasm 0.4.0"));
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/.gsxi-version")), QByteArray("0.4.0"));
}

void CommbusBundleInstallerTest::olderInstallIsReplacedAndMarked()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/Community");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    MakeBundle(PackageDir(community), QStringLiteral("0.3.2"));
    WriteFile(PackageDir(community) + QStringLiteral("/stale.txt"), "old");

    const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community), NothingRunning);

    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Installed);
    QVERIFY(!QFile::exists(PackageDir(community) + QStringLiteral("/stale.txt")));
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/manifest.json")), Manifest(QStringLiteral("0.4.0")));
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/.gsxi-version")), QByteArray("0.4.0"));
}

void CommbusBundleInstallerTest::equalOrNewerInstallIsLeftUntouched()
{
    for (const QString& installed : {QStringLiteral("0.4.0"), QStringLiteral("0.5.1")})
    {
        const QTemporaryDir root;
        const QString bundle = root.path() + QStringLiteral("/bundle");
        const QString community = root.path() + QStringLiteral("/Community");
        MakeBundle(bundle, QStringLiteral("0.4.0"));
        MakeBundle(PackageDir(community), installed);

        const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community), NothingRunning);

        QCOMPARE(result.targets[0].status, CommbusBundleStatus::UpToDate);
        QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/SimObjects/bridge.wasm")),
                 "wasm " + installed.toUtf8());
        QVERIFY(!QFile::exists(PackageDir(community) + QStringLiteral("/.gsxi-version")));
    }
}

void CommbusBundleInstallerTest::runningSimulatorSkipsTheTarget()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/Community");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    MakeBundle(PackageDir(community), QStringLiteral("0.3.2"));

    QStringList asked;
    const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community),
                                                            [&asked](const QString& processName)
                                                            {
                                                                asked.append(processName);

                                                                return true;
                                                            });

    QCOMPARE(asked, QStringList{QLatin1String(kSimProcess)});
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::SimRunning);
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/manifest.json")), Manifest(QStringLiteral("0.3.2")));
}

void CommbusBundleInstallerTest::upToDateTargetIsNotBlockedByTheRunningSimulator()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/Community");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    MakeBundle(PackageDir(community), QStringLiteral("0.4.0"));

    const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community),
                                                            [](const QString&) { return true; });

    QCOMPARE(result.targets[0].status, CommbusBundleStatus::UpToDate);
}

void CommbusBundleInstallerTest::junctionIsUnlinkedWithoutTouchingItsTarget()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/Community");
    const QString devCheckout = root.path() + QStringLiteral("/dev-checkout");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    MakeBundle(devCheckout, QStringLiteral("0.3.2"));
    QDir().mkpath(community);

    const int mklink = QProcess::execute(QStringLiteral("cmd.exe"), {
        QStringLiteral("/c"), QStringLiteral("mklink"), QStringLiteral("/J"),
        QDir::toNativeSeparators(PackageDir(community)), QDir::toNativeSeparators(devCheckout)
    });

    QCOMPARE(mklink, 0);
    QVERIFY(QFileInfo(PackageDir(community)).isJunction());

    const CommbusBundleResult result = InstallCommbusBundle(bundle, OneTarget(community), NothingRunning);

    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Installed);
    QVERIFY(!QFileInfo(PackageDir(community)).isJunction());
    QCOMPARE(ReadFile(devCheckout + QStringLiteral("/manifest.json")), Manifest(QStringLiteral("0.3.2")));
    QVERIFY(QFile::exists(devCheckout + QStringLiteral("/SimObjects/bridge.wasm")));
    QCOMPARE(ReadFile(PackageDir(community) + QStringLiteral("/manifest.json")), Manifest(QStringLiteral("0.4.0")));
}

void CommbusBundleInstallerTest::bundleWithoutManifestFailsEveryTarget()
{
    const QTemporaryDir root;
    const QString community = root.path() + QStringLiteral("/Community");
    QDir().mkpath(community);

    const CommbusBundleResult result =
        InstallCommbusBundle(root.path() + QStringLiteral("/missing"), OneTarget(community), NothingRunning);

    QVERIFY(result.bundledVersion.isEmpty());
    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Failed);
    QVERIFY(!QDir(PackageDir(community)).exists());
}

void CommbusBundleInstallerTest::installedVersionPrefersTheMarkerOverTheManifest()
{
    const QTemporaryDir root;
    const QString community = root.path() + QStringLiteral("/Community");
    MakeBundle(PackageDir(community), QStringLiteral("0.3.2"));

    QCOMPARE(InstalledCommbusPackageVersion(community), QStringLiteral("0.3.2"));

    WriteFile(PackageDir(community) + QStringLiteral("/.gsxi-version"), "0.3.5\n");

    QCOMPARE(InstalledCommbusPackageVersion(community), QStringLiteral("0.3.5"));
}

void CommbusBundleInstallerTest::detectsTargetsWhoseCommunityFolderExists()
{
    const QTemporaryDir home;
    const QString packages2024 = home.path() + QStringLiteral("/packages2024");
    const QString packages2020 = home.path() + QStringLiteral("/packages2020");
    QDir().mkpath(packages2024 + QStringLiteral("/Community"));
    QDir().mkpath(packages2020);

    WriteFile(home.path() + QStringLiteral("/AppData/Local/Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache/UserCfg.opt"),
              "InstalledPackagesPath \"" + QDir::toNativeSeparators(packages2024).toUtf8() + "\"\n");
    WriteFile(home.path() + QStringLiteral("/AppData/Roaming/Microsoft Flight Simulator/UserCfg.opt"),
              "InstalledPackagesPath \"" + QDir::toNativeSeparators(packages2020).toUtf8() + "\"\n");

    const std::vector<CommbusInstallTarget> targets = DetectCommbusInstallTargets(home.path());

    QCOMPARE(targets.size(), 1u);
    QCOMPARE(targets[0].label, QStringLiteral("MSFS 2024 (Microsoft Store)"));
    QCOMPARE(targets[0].communityPath, packages2024 + QStringLiteral("/Community"));
    QCOMPARE(targets[0].processName, QStringLiteral("FlightSimulator2024.exe"));
}

void CommbusBundleInstallerTest::overrideDirIsTheOnlyTargetAndHasNoProcess()
{
    const QTemporaryDir root;
    const QString bundle = root.path() + QStringLiteral("/bundle");
    const QString community = root.path() + QStringLiteral("/qa-community");
    MakeBundle(bundle, QStringLiteral("0.4.0"));
    QDir().mkpath(community);

    const std::vector<CommbusInstallTarget> targets =
        ResolveCommbusInstallTargets(QDir::toNativeSeparators(community), root.path());

    QCOMPARE(targets.size(), 1u);
    QCOMPARE(targets[0].communityPath, community);
    QVERIFY(targets[0].processName.isEmpty());

    const CommbusBundleResult result = InstallCommbusBundle(bundle, targets,
                                                            [](const QString&) { return true; });

    QCOMPARE(result.targets[0].status, CommbusBundleStatus::Installed);
}

QTEST_GUILESS_MAIN(CommbusBundleInstallerTest)

#include "tst_commbus_bundle_installer.moc"
