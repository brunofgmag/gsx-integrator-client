#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>
#include <QtTest/QTest>

#include "../src/infrastructure/update/GithubUpdateService.h"

namespace
{
    struct StageCall
    {
        bool ok = true;
        QString error;
    };

    class StageRecorder final : public UpdateServiceObserver
    {
    public:
        void OnCheckFinished(bool, bool, const UpdateInfo&, const QString&) override
        {
        }

        void OnCommbusCheckFinished(bool, const QString&, const QString&, const QString&) override
        {
        }

        void OnDownloadProgress(qint64, qint64) override
        {
        }

        void OnStageFinished(const bool ok, const QString& error) override
        {
            stages.push_back({ok, error});
        }

        std::vector<StageCall> stages;
    };

    UpdateInfo DownloadableRelease()
    {
        UpdateInfo info;
        info.version = QStringLiteral("9.0.0");
        info.zipName = QStringLiteral("gsx-integrator-client-9.0.0.zip");
        info.zipUrl = QStringLiteral("http://127.0.0.1:9/gsx-integrator-client-9.0.0.zip");
        info.shaUrl = QStringLiteral("http://127.0.0.1:9/gsx-integrator-client-9.0.0.zip.sha256");

        return info;
    }
}

class GithubUpdateServiceFlightsimToTest final : public QObject
{
    Q_OBJECT

private slots:
    static void initTestCase();
    static void downloadIsRefusedAtOnce();
    static void nothingIsEverStagedOrApplied();
};

void GithubUpdateServiceFlightsimToTest::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("GithubUpdateServiceFlightsimToTest"));
    QStandardPaths::setTestModeEnabled(true);
}

void GithubUpdateServiceFlightsimToTest::downloadIsRefusedAtOnce()
{
    GithubUpdateService service({}, {}, QStringLiteral("1.0.0"));
    StageRecorder recorder;
    service.AddObserver(&recorder);

    service.DownloadAndStage(DownloadableRelease());

    QCOMPARE(recorder.stages.size(), 1u);
    QVERIFY(!recorder.stages[0].ok);
    QVERIFY(!recorder.stages[0].error.isEmpty());

    service.RemoveObserver(&recorder);
}

void GithubUpdateServiceFlightsimToTest::nothingIsEverStagedOrApplied()
{
    GithubUpdateService service({}, {}, QStringLiteral("1.0.0"));

    QVERIFY(!service.HasStagedUpdate());
    QVERIFY(!service.LaunchApplyHelper(true));
    QVERIFY(!service.LaunchApplyHelper(false));
}

QTEST_GUILESS_MAIN(GithubUpdateServiceFlightsimToTest)

#include "tst_github_update_service_flightsim_to.moc"
