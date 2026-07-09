#include <QtTest/QTest>

#include "doubles/FakeUpdateService.h"
#include "../src/viewmodel/UpdateViewModel.h"

namespace
{
    UpdateInfo MakeInfo(const QString& version)
    {
        UpdateInfo info;
        info.version = version;
        info.releasePageUrl = QStringLiteral("https://example.com/releases/v") + version;
        info.zipUrl = QStringLiteral("https://example.com/client.zip");
        info.shaUrl = QStringLiteral("https://example.com/client.zip.sha256");
        info.zipName = QStringLiteral("client.zip");
        return info;
    }
}

class UpdateViewModelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void notifyFlowDownloadsAndRestartsOnDemand();
    static void autoModeDownloadsSilentlyAndAppliesOnExit();
    static void silentCheckFailureStaysQuiet();
    static void explicitCheckFailureShowsError();
    static void stageFailureShowsErrorAndKeepsUpdateKnown();
    static void newerReleaseAfterStagingGoesBackToAvailable();
    static void stagedUpdateFoundOnCheckIsReadyToRestart();
    static void switchingToAutoStartsPendingDownload();
    static void disabledViewModelIgnoresChecks();
    static void commbusComparesInstalledAndLatest();
};

void UpdateViewModelTest::notifyFlowDownloadsAndRestartsOnDemand()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    viewModel.checkForUpdates();

    QCOMPARE(service.checkCalls, 1);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Checking));

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::UpdateAvailable));
    QVERIFY(viewModel.IsUpdateAvailable());
    QCOMPARE(viewModel.GetLatestVersion(), QStringLiteral("1.4.0"));
    QCOMPARE(service.downloadCalls, 0);

    viewModel.downloadAndInstall();

    QCOMPARE(service.downloadCalls, 1);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Downloading));

    service.FireDownloadProgress(50, 100);

    QCOMPARE(viewModel.GetProgress(), 0.5);

    service.FireStageFinished(true);

    QCOMPARE(service.helperCalls, 1);
    QVERIFY(service.lastRelaunch);
}

void UpdateViewModelTest::autoModeDownloadsSilentlyAndAppliesOnExit()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Auto, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(service.downloadCalls, 1);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Downloading));

    service.FireStageFinished(true);

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::ReadyToRestart));
    QVERIFY(viewModel.IsReadyToRestart());
    QCOMPARE(service.helperCalls, 0);
    QVERIFY(viewModel.ShouldApplyOnExit());
}

void UpdateViewModelTest::silentCheckFailureStaysQuiet()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    service.FireCheckFinished(false, false, {}, QStringLiteral("HTTP 403"));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Idle));
    QVERIFY(viewModel.GetErrorMessage().isEmpty());
    QVERIFY(!viewModel.IsUpdateAvailable());
}

void UpdateViewModelTest::explicitCheckFailureShowsError()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Manual, true);

    viewModel.checkForUpdates();
    service.FireCheckFinished(false, false, {}, QStringLiteral("HTTP 404"));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Error));
    QCOMPARE(viewModel.GetErrorMessage(), QStringLiteral("HTTP 404"));
}

void UpdateViewModelTest::stageFailureShowsErrorAndKeepsUpdateKnown()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));
    viewModel.downloadAndInstall();
    service.FireStageFinished(false, QStringLiteral("Checksum mismatch."));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Error));
    QCOMPARE(viewModel.GetErrorMessage(), QStringLiteral("Checksum mismatch."));
    QVERIFY(viewModel.IsUpdateAvailable());
    QCOMPARE(service.helperCalls, 0);
}

void UpdateViewModelTest::newerReleaseAfterStagingGoesBackToAvailable()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));
    viewModel.downloadAndInstall();
    service.FireStageFinished(true);

    QCOMPARE(service.helperCalls, 1);

    service.staged = false;
    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.5.0")));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::UpdateAvailable));
    QCOMPARE(viewModel.GetLatestVersion(), QStringLiteral("1.5.0"));
}

void UpdateViewModelTest::stagedUpdateFoundOnCheckIsReadyToRestart()
{
    FakeUpdateService service;
    service.staged = true;

    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);
    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::ReadyToRestart));
    QCOMPARE(service.downloadCalls, 0);
}

void UpdateViewModelTest::switchingToAutoStartsPendingDownload()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(service.downloadCalls, 0);

    viewModel.SetMode(UpdateViewModel::Auto);

    QCOMPARE(service.downloadCalls, 1);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Downloading));
}

void UpdateViewModelTest::disabledViewModelIgnoresChecks()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, false);

    viewModel.checkForUpdates();

    QCOMPARE(service.checkCalls, 0);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Idle));
    QVERIFY(!viewModel.AreChecksEnabled());
}

void UpdateViewModelTest::commbusComparesInstalledAndLatest()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, QStringLiteral("1.3.0"),
                              UpdateViewModel::Notify, true);

    service.FireCommbusCheckFinished(true, QStringLiteral("0.2.1"),
                                     QStringLiteral("0.3.0"),
                                     QStringLiteral("https://example.com"));

    QVERIFY(viewModel.IsCommbusUpdateAvailable());
    QCOMPARE(viewModel.GetCommbusInstalledVersion(), QStringLiteral("0.2.1"));
    QCOMPARE(viewModel.GetCommbusLatestVersion(), QStringLiteral("0.3.0"));

    service.FireCommbusCheckFinished(true, {}, QStringLiteral("0.3.0"));

    QVERIFY(!viewModel.IsCommbusUpdateAvailable());

    service.FireCommbusCheckFinished(true, QStringLiteral("0.3.0"),
                                     QStringLiteral("0.3.0"));
    QVERIFY(!viewModel.IsCommbusUpdateAvailable());

    service.FireCommbusCheckFinished(true, QStringLiteral("0.2.0"),
                                     QStringLiteral("0.3.0"));
    QVERIFY(viewModel.IsCommbusUpdateAvailable());

    service.FireCommbusCheckFinished(false, {}, {});
    QVERIFY(viewModel.IsCommbusUpdateAvailable());
}

QTEST_GUILESS_MAIN(UpdateViewModelTest)

#include "tst_update_viewmodel.moc"
