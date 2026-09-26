#include <map>
#include <utility>
#include <vector>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/update/GithubReleaseParser.h"
#include "../src/infrastructure/update/GithubUpdateService.h"

namespace
{
    struct CannedResponse
    {
        int status = 200;
        QByteArray body;
    };

    class MiniHttpServer final : public QObject
    {
    public:
        MiniHttpServer()
        {
            server_.listen(QHostAddress::LocalHost, 0);
            connect(&server_, &QTcpServer::newConnection, this, &MiniHttpServer::HandleConnection);
        }

        [[nodiscard]] QString UrlFor(const QString& path) const
        {
            return QStringLiteral("http://127.0.0.1:%1%2").arg(server_.serverPort()).arg(path);
        }

        void SetResponse(const QString& path, const int status, QByteArray body)
        {
            responses_[path] = {status, std::move(body)};
        }

    private:
        void HandleConnection()
        {
            QTcpSocket* socket = server_.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]
            {
                const QString requestLine = QString::fromUtf8(socket->readAll()).section("\r\n", 0, 0);
                const QString path = requestLine.section(' ', 1, 1);

                CannedResponse response{404, {}};
                if (const auto it = responses_.find(path); it != responses_.end())
                {
                    response = it->second;
                }

                QByteArray payload = "HTTP/1.1 " + QByteArray::number(response.status)
                    + (response.status == 200 ? " OK" : " Not Found") + "\r\n"
                    + "Content-Length: " + QByteArray::number(response.body.size()) + "\r\n"
                    + "Connection: close\r\n\r\n"
                    + response.body;
                socket->write(payload);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }

        QTcpServer server_;
        std::map<QString, CannedResponse> responses_;
    };

    struct CheckCall
    {
        bool ok = false;
        bool available = false;
        UpdateInfo info;
        QString error;
    };

    struct CommbusCall
    {
        bool ok = false;
        QString installed;
        QString latest;
    };

    struct StageCall
    {
        bool ok = false;
        QString error;
    };

    class RecordingObserver final : public UpdateServiceObserver
    {
    public:
        std::vector<CheckCall> checks;
        std::vector<CommbusCall> commbusChecks;
        std::vector<StageCall> stages;
        int progressCalls = 0;

        void OnCheckFinished(const bool ok, const bool updateAvailable,
                             const UpdateInfo& info, const QString& error) override
        {
            checks.push_back({ok, updateAvailable, info, error});
        }

        void OnCommbusCheckFinished(const bool ok, const QString& installedVersion,
                                    const QString& latestVersion) override
        {
            commbusChecks.push_back({ok, installedVersion, latestVersion});
        }

        void OnDownloadProgress(qint64, qint64) override
        {
            ++progressCalls;
        }

        void OnStageFinished(const bool ok, const QString& error) override
        {
            stages.push_back({ok, error});
        }
    };

    QByteArray ReleaseFeed(const QString& tag, const QString& zipUrl, const QString& shaUrl)
    {
        QJsonArray assets;
        if (!zipUrl.isEmpty())
        {
            assets.append(QJsonObject{
                {"name", "gsx-integrator-client-x.zip"},
                {"browser_download_url", zipUrl}
            });
        }
        if (!shaUrl.isEmpty())
        {
            assets.append(QJsonObject{
                {"name", "gsx-integrator-client-x.zip.sha256"},
                {"browser_download_url", shaUrl}
            });
        }

        const QJsonObject release{
            {"tag_name", tag},
            {"html_url", "https://example.test/release"},
            {"assets", assets}
        };

        return QJsonDocument(release).toJson(QJsonDocument::Compact);
    }

    constexpr auto kTestUninstallRoot = R"(HKEY_CURRENT_USER\Software\gsxi-test-uninstall)";
    constexpr auto kTestUninstallProviderRoot = R"(HKCU:\Software\gsxi-test-uninstall)";
    constexpr auto kTestExeName = "gsx-integrator-client.exe";
    constexpr auto kStaleDisplayVersion = "1.29.7";

    void DeleteTestUninstallRoot()
    {
        QProcess reg;
        reg.start(QStringLiteral("reg.exe"),
                  {QStringLiteral("delete"), QString::fromLatin1(kTestUninstallRoot), QStringLiteral("/f")});
        reg.waitForFinished();
    }

    qint64 ExitedProcessId()
    {
        QProcess process;
        process.start(QStringLiteral("cmd.exe"), {QStringLiteral("/c"), QStringLiteral("exit 0")});
        process.waitForStarted();
        const qint64 pid = process.processId();
        process.waitForFinished();

        return pid;
    }

    bool WriteFile(const QString& path, const QByteArray& content)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        file.write(content);
        file.close();

        return true;
    }
}

class GithubUpdateServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void initTestCase();

    static void clientCheckSuccessReportsUpdateAvailable();
    static void clientCheckOlderVersionReportsNotAvailable();
    static void clientCheckHttpErrorReportsFailure();
    static void clientCheckBadFeedReportsFormatError();
    static void commbusCheckReportsInstalledAndLatest();
    static void downloadWithoutAssetsFailsImmediately();
    static void downloadWithInvalidShaFileFails();
    static void downloadWithChecksumMismatchDiscardsZip();
    static void applyArgumentsCarryTheNormalisedReleaseVersion_data();
    static void applyArgumentsCarryTheNormalisedReleaseVersion();
    static void applyScriptUpdatesDisplayVersionOnlyForThisInstall_data();
    static void applyScriptUpdatesDisplayVersionOnlyForThisInstall();

    static void cleanupTestCase();
};

void GithubUpdateServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("GsxIntegratorTests"));
    QCoreApplication::setApplicationName(QStringLiteral("GithubUpdateServiceTest"));
    QStandardPaths::setTestModeEnabled(true);
    DeleteTestUninstallRoot();
}

void GithubUpdateServiceTest::cleanupTestCase()
{
    DeleteTestUninstallRoot();
}

void GithubUpdateServiceTest::clientCheckSuccessReportsUpdateAvailable()
{
    MiniHttpServer http;
    http.SetResponse("/client.json",
                     200,
                     ReleaseFeed("v9.9.9", "https://example.test/z.zip", "https://example.test/z.zip.sha256"));

    GithubUpdateService service(http.UrlFor("/client.json"), {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.CheckForUpdates();

    QTRY_COMPARE(observer.checks.size(), std::size_t{1});
    QCOMPARE(observer.checks[0].ok, true);
    QCOMPARE(observer.checks[0].available, true);
    QCOMPARE(observer.checks[0].info.version, QStringLiteral("9.9.9"));
    QCOMPARE(observer.checks[0].info.zipName, QStringLiteral("gsx-integrator-client-x.zip"));
    QVERIFY(observer.checks[0].error.isEmpty());
}

void GithubUpdateServiceTest::clientCheckOlderVersionReportsNotAvailable()
{
    MiniHttpServer http;
    http.SetResponse("/client.json", 200, ReleaseFeed("v0.0.1", {}, {}));

    GithubUpdateService service(http.UrlFor("/client.json"), {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.CheckForUpdates();

    QTRY_COMPARE(observer.checks.size(), std::size_t{1});
    QCOMPARE(observer.checks[0].ok, true);
    QCOMPARE(observer.checks[0].available, false);
}

void GithubUpdateServiceTest::clientCheckHttpErrorReportsFailure()
{
    const MiniHttpServer http;

    GithubUpdateService service(http.UrlFor("/missing.json"), {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.CheckForUpdates();

    QTRY_COMPARE(observer.checks.size(), std::size_t{1});
    QCOMPARE(observer.checks[0].ok, false);
    QCOMPARE(observer.checks[0].error, QStringLiteral("HTTP 404"));
}

void GithubUpdateServiceTest::clientCheckBadFeedReportsFormatError()
{
    MiniHttpServer http;
    http.SetResponse("/client.json", 200, "this is not a release feed");

    GithubUpdateService service(http.UrlFor("/client.json"), {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.CheckForUpdates();

    QTRY_COMPARE(observer.checks.size(), std::size_t{1});
    QCOMPARE(observer.checks[0].ok, false);
    QCOMPARE(observer.checks[0].error, QStringLiteral("Unexpected release feed format."));
}

void GithubUpdateServiceTest::commbusCheckReportsInstalledAndLatest()
{
    const QTemporaryDir communityDir;

    QVERIFY(communityDir.isValid());

    const QString packageDir = communityDir.path() + QStringLiteral("/gsx-integrator-commbus");

    QVERIFY(QDir().mkpath(packageDir));

    QFile manifest(packageDir + QStringLiteral("/manifest.json"));

    QVERIFY(manifest.open(QIODevice::WriteOnly));

    manifest.write(R"({"package_version":"1.2.3"})");
    manifest.close();
    qputenv("GSXI_COMMBUS_COMMUNITY_DIR", communityDir.path().toUtf8());

    MiniHttpServer http;
    http.SetResponse("/commbus.json", 200, ReleaseFeed("v2.0.0", {}, {}));

    GithubUpdateService service({}, http.UrlFor("/commbus.json"), QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.CheckForUpdates();

    QTRY_COMPARE(observer.commbusChecks.size(), std::size_t{1});

    qunsetenv("GSXI_COMMBUS_COMMUNITY_DIR");

    QCOMPARE(observer.commbusChecks[0].ok, true);
    QCOMPARE(observer.commbusChecks[0].installed, QStringLiteral("1.2.3"));
    QCOMPARE(observer.commbusChecks[0].latest, QStringLiteral("2.0.0"));
}

void GithubUpdateServiceTest::downloadWithoutAssetsFailsImmediately()
{
    GithubUpdateService service({}, {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    service.DownloadAndStage(UpdateInfo{});

    QCOMPARE(observer.stages.size(), std::size_t{1});
    QCOMPARE(observer.stages[0].ok, false);
    QCOMPARE(observer.stages[0].error, QStringLiteral("The release has no download assets."));
    QVERIFY(!service.HasStagedUpdate());
}

void GithubUpdateServiceTest::downloadWithInvalidShaFileFails()
{
    MiniHttpServer http;
    http.SetResponse("/z.zip.sha256", 200, "not-a-checksum");

    GithubUpdateService service({}, {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    UpdateInfo info;
    info.version = QStringLiteral("9.9.9");
    info.zipName = QStringLiteral("gsx-integrator-client-x.zip");
    info.zipUrl = http.UrlFor("/z.zip");
    info.shaUrl = http.UrlFor("/z.zip.sha256");
    service.DownloadAndStage(info);

    QTRY_COMPARE(observer.stages.size(), std::size_t{1});
    QCOMPARE(observer.stages[0].ok, false);
    QCOMPARE(observer.stages[0].error, QStringLiteral("Invalid checksum file."));
    QVERIFY(!service.HasStagedUpdate());
}

void GithubUpdateServiceTest::downloadWithChecksumMismatchDiscardsZip()
{
    MiniHttpServer http;
    const QByteArray wrongSha(64, 'a');
    http.SetResponse("/z.zip.sha256", 200, wrongSha + " gsx-integrator-client-x.zip");
    http.SetResponse("/z.zip", 200, "zip-bytes-that-do-not-match");

    GithubUpdateService service({}, {}, QStringLiteral("1.0.0"));
    RecordingObserver observer;
    service.AddObserver(&observer);

    UpdateInfo info;
    info.version = QStringLiteral("9.9.9");
    info.zipName = QStringLiteral("gsx-integrator-client-x.zip");
    info.zipUrl = http.UrlFor("/z.zip");
    info.shaUrl = http.UrlFor("/z.zip.sha256");
    service.DownloadAndStage(info);

    QTRY_COMPARE(observer.stages.size(), std::size_t{1});
    QCOMPARE(observer.stages[0].ok, false);
    QCOMPARE(observer.stages[0].error, QStringLiteral("Checksum mismatch. Download discarded."));
    QVERIFY(observer.progressCalls > 0);
    QVERIFY(!service.HasStagedUpdate());

    const QString zipPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/updates/download/gsx-integrator-client-x.zip");
    QVERIFY(!QFile::exists(zipPath));
}

void GithubUpdateServiceTest::applyArgumentsCarryTheNormalisedReleaseVersion_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain tag") << QStringLiteral("v1.40.0") << QStringLiteral("1.40.0");
    QTest::newRow("upper-case prefix") << QStringLiteral("V1.34.12") << QStringLiteral("1.34.12");
    QTest::newRow("pre-release suffix") << QStringLiteral("v1.41.0-rc.1") << QStringLiteral("1.41.0");
}

void GithubUpdateServiceTest::applyArgumentsCarryTheNormalisedReleaseVersion()
{
    QFETCH(QString, tag);
    QFETCH(QString, expected);

    const auto info = ParseLatestRelease(ReleaseFeed(tag, {}, {}));

    QVERIFY(info.has_value());

    const QStringList arguments = GithubUpdateService::BuildApplyArguments(
        QStringLiteral("C:/updates/apply.ps1"), 42, QStringLiteral("C:/updates/staged"),
        QStringLiteral("C:/app"), QString::fromLatin1(kTestExeName), info->version, false);
    const qsizetype versionIndex = arguments.indexOf(QStringLiteral("-Version"));

    QVERIFY(versionIndex >= 0);
    QVERIFY(versionIndex + 1 < arguments.size());
    QCOMPARE(arguments.at(versionIndex + 1), expected);
}

void GithubUpdateServiceTest::applyScriptUpdatesDisplayVersionOnlyForThisInstall_data()
{
    QTest::addColumn<bool>("keyExists");
    QTest::addColumn<QString>("installLocationSuffix");
    QTest::addColumn<bool>("otherLocation");
    QTest::addColumn<QString>("destSuffix");
    QTest::addColumn<QString>("expectedDisplayVersion");

    QTest::newRow("same location") << true << QString() << false << QString() << QStringLiteral("1.40.0");
    QTest::newRow("install location with trailing backslash")
        << true << QStringLiteral("\\") << false << QString() << QStringLiteral("1.40.0");
    QTest::newRow("install location with trailing slash")
        << true << QStringLiteral("/") << false << QString() << QStringLiteral("1.40.0");
    QTest::newRow("dest with trailing separator")
        << true << QString() << false << QStringLiteral("/") << QStringLiteral("1.40.0");
    QTest::newRow("other location stays untouched")
        << true << QString() << true << QString() << QString::fromLatin1(kStaleDisplayVersion);
    QTest::newRow("missing key is not created") << false << QString() << false << QString() << QString();
}

void GithubUpdateServiceTest::applyScriptUpdatesDisplayVersionOnlyForThisInstall()
{
    QFETCH(bool, keyExists);
    QFETCH(QString, installLocationSuffix);
    QFETCH(bool, otherLocation);
    QFETCH(QString, destSuffix);
    QFETCH(QString, expectedDisplayVersion);

    const QTemporaryDir root;

    QVERIFY(root.isValid());

    const QString source = root.filePath(QStringLiteral("source"));
    const QString dest = root.filePath(QStringLiteral("install"));
    const QString other = root.filePath(QStringLiteral("elsewhere"));
    const QString scriptDir = root.filePath(QStringLiteral("updates"));

    QVERIFY(QDir().mkpath(source));
    QVERIFY(QDir().mkpath(dest));
    QVERIFY(QDir().mkpath(other));
    QVERIFY(QDir().mkpath(scriptDir));
    QVERIFY(WriteFile(source + QStringLiteral("/") + QString::fromLatin1(kTestExeName), "new build"));
    QVERIFY(WriteFile(dest + QStringLiteral("/") + QString::fromLatin1(kTestExeName), "old"));

    const QString scriptPath = scriptDir + QStringLiteral("/apply.ps1");

    QVERIFY(WriteFile(scriptPath, GithubUpdateService::ApplyScript()));

    const QString keyName = QString::fromLatin1(QTest::currentDataTag()).replace(u' ', u'-');
    const QString installLocation =
        QDir::toNativeSeparators(otherLocation ? other : dest) + installLocationSuffix;

    DeleteTestUninstallRoot();
    if (keyExists)
    {
        QSettings registry(QString::fromLatin1(kTestUninstallRoot), QSettings::NativeFormat);
        registry.beginGroup(keyName);
        registry.setValue(QStringLiteral("InstallLocation"), installLocation);
        registry.setValue(QStringLiteral("DisplayVersion"), QString::fromLatin1(kStaleDisplayVersion));
        registry.endGroup();
        registry.sync();

        QCOMPARE(registry.status(), QSettings::NoError);
    }

    QStringList arguments = GithubUpdateService::BuildApplyArguments(
        scriptPath, ExitedProcessId(), source, dest + destSuffix, QString::fromLatin1(kTestExeName),
        QStringLiteral("1.40.0"), false);
    arguments << QStringLiteral("-UninstallKey")
        << QString::fromLatin1(kTestUninstallProviderRoot) + QStringLiteral("\\") + keyName;

    QProcess apply;
    apply.start(QStringLiteral("powershell.exe"), arguments);

    QVERIFY(apply.waitForFinished(60000));
    QCOMPARE(apply.exitCode(), 0);

    QFile copied(dest + QStringLiteral("/") + QString::fromLatin1(kTestExeName));

    QVERIFY(copied.open(QIODevice::ReadOnly));
    QCOMPARE(copied.readAll(), QByteArray("new build"));

    const QSettings registry(QString::fromLatin1(kTestUninstallRoot), QSettings::NativeFormat);
    const bool keyPresent = registry.childGroups().contains(keyName);

    QCOMPARE(keyPresent, keyExists);
    if (keyExists)
    {
        QCOMPARE(registry.value(keyName + QStringLiteral("/DisplayVersion")).toString(), expectedDisplayVersion);
        QCOMPARE(registry.value(keyName + QStringLiteral("/InstallLocation")).toString(), installLocation);
    }
}

QTEST_GUILESS_MAIN(GithubUpdateServiceTest)

#include "tst_github_update_service.moc"
