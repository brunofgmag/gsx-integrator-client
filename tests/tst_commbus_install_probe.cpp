#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

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

QTEST_APPLESS_MAIN(CommbusInstallProbeTest)

#include "tst_commbus_install_probe.moc"
