#include <map>
#include <utility>
#include <vector>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QTest>

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
        QString releaseUrl;
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
                                    const QString& latestVersion, const QString& releaseUrl) override
        {
            commbusChecks.push_back({ok, installedVersion, latestVersion, releaseUrl});
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
};

void GithubUpdateServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("GsxIntegratorTests"));
    QCoreApplication::setApplicationName(QStringLiteral("GithubUpdateServiceTest"));
    QStandardPaths::setTestModeEnabled(true);
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
    QCOMPARE(observer.commbusChecks[0].releaseUrl, QStringLiteral("https://example.test/release"));
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

QTEST_GUILESS_MAIN(GithubUpdateServiceTest)

#include "tst_github_update_service.moc"
