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

    Distribution FlightsimTo()
    {
        return {true, QStringLiteral("https://flightsim.to/file/1/gsx-integrator")};
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
    static void derivedFlagsAndStatusTextFollowState();
    static void stagedUpdateExposesRestartText();
    static void errorStateExposesHasErrorAndMessage();
    static void githubChannelAllowsDownloads();
    static void flightsimToNeverDownloadsEvenInAutoMode();
    static void flightsimToIgnoresModeChanges();
    static void flightsimToNeverAppliesOrRestarts();
    static void flightsimToKeepsCheckingForUpdates();
    static void flightsimToReportsTheBundledCommbus();
    static void flightsimToIgnoresTheCommbusFeed();
};

void UpdateViewModelTest::notifyFlowDownloadsAndRestartsOnDemand()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

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
    UpdateViewModel viewModel(&service, UpdateViewModel::Auto, true);

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
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

    service.FireCheckFinished(false, false, {}, QStringLiteral("HTTP 403"));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Idle));
    QVERIFY(viewModel.GetErrorMessage().isEmpty());
    QVERIFY(!viewModel.IsUpdateAvailable());
}

void UpdateViewModelTest::explicitCheckFailureShowsError()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Manual, true);

    viewModel.checkForUpdates();
    service.FireCheckFinished(false, false, {}, QStringLiteral("HTTP 404"));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Error));
    QCOMPARE(viewModel.GetErrorMessage(), QStringLiteral("HTTP 404"));
}

void UpdateViewModelTest::stageFailureShowsErrorAndKeepsUpdateKnown()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

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
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

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

    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);
    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::ReadyToRestart));
    QCOMPARE(service.downloadCalls, 0);
}

void UpdateViewModelTest::switchingToAutoStartsPendingDownload()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(service.downloadCalls, 0);

    viewModel.SetMode(UpdateViewModel::Auto);

    QCOMPARE(service.downloadCalls, 1);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Downloading));
}

void UpdateViewModelTest::disabledViewModelIgnoresChecks()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, false);

    viewModel.checkForUpdates();

    QCOMPARE(service.checkCalls, 0);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::Idle));
    QVERIFY(!viewModel.AreChecksEnabled());
}

void UpdateViewModelTest::commbusComparesInstalledAndLatest()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

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

void UpdateViewModelTest::derivedFlagsAndStatusTextFollowState()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

    QVERIFY(viewModel.CanCheckForUpdates());
    QVERIFY(!viewModel.CanDownload());
    QVERIFY(!viewModel.IsDownloading());
    QVERIFY(!viewModel.HasError());

    viewModel.checkForUpdates();

    QVERIFY(!viewModel.CanCheckForUpdates());
    QCOMPARE(viewModel.GetStatusText(), QStringLiteral("Checking for updates…"));

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QVERIFY(viewModel.CanDownload());
    QVERIFY(!viewModel.IsDownloading());
    QCOMPARE(viewModel.GetStatusText(), QStringLiteral("Update available · v1.4.0"));

    viewModel.downloadAndInstall();

    QVERIFY(viewModel.IsDownloading());
    QVERIFY(!viewModel.CanDownload());
    QCOMPARE(viewModel.GetStatusText(), QStringLiteral("Downloading v1.4.0"));
}

void UpdateViewModelTest::stagedUpdateExposesRestartText()
{
    FakeUpdateService service;
    service.staged = true;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QVERIFY(viewModel.IsReadyToRestart());
    QVERIFY(!viewModel.CanDownload());
    QCOMPARE(viewModel.GetStatusText(), QStringLiteral("Update ready: restart to apply"));
}

void UpdateViewModelTest::errorStateExposesHasErrorAndMessage()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Manual, true);

    viewModel.checkForUpdates();
    service.FireCheckFinished(false, false, {}, QStringLiteral("HTTP 500"));

    QVERIFY(viewModel.HasError());
    QVERIFY(viewModel.CanCheckForUpdates());
    QCOMPARE(viewModel.GetStatusText(), QStringLiteral("HTTP 500"));
}

void UpdateViewModelTest::githubChannelAllowsDownloads()
{
    FakeUpdateService service;
    const UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true);

    QVERIFY(viewModel.AreDownloadsAllowed());
    QVERIFY(viewModel.GetExternalDownloadUrl().isEmpty());
    QVERIFY(!viewModel.IsCommbusBundled());
}

void UpdateViewModelTest::flightsimToNeverDownloadsEvenInAutoMode()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Auto, true, FlightsimTo());

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QCOMPARE(service.downloadCalls, 0);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::UpdateAvailable));
    QVERIFY(viewModel.IsUpdateAvailable());
    QVERIFY(!viewModel.CanDownload());
    QVERIFY(!viewModel.AreDownloadsAllowed());
    QCOMPARE(viewModel.GetExternalDownloadUrl(), QStringLiteral("https://flightsim.to/file/1/gsx-integrator"));

    viewModel.downloadAndInstall();

    QCOMPARE(service.downloadCalls, 0);
}

void UpdateViewModelTest::flightsimToIgnoresModeChanges()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true, FlightsimTo());

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));
    viewModel.SetMode(UpdateViewModel::Auto);

    QCOMPARE(service.downloadCalls, 0);
    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::UpdateAvailable));

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.5.0")));

    QCOMPARE(service.downloadCalls, 0);
}

void UpdateViewModelTest::flightsimToNeverAppliesOrRestarts()
{
    FakeUpdateService service;
    service.staged = true;
    UpdateViewModel viewModel(&service, UpdateViewModel::Auto, true, FlightsimTo());

    service.FireCheckFinished(true, true, MakeInfo(QStringLiteral("1.4.0")));

    QVERIFY(!viewModel.IsReadyToRestart());
    QVERIFY(!viewModel.ShouldApplyOnExit());

    viewModel.restartNow();

    QCOMPARE(service.helperCalls, 0);
}

void UpdateViewModelTest::flightsimToKeepsCheckingForUpdates()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Manual, true, FlightsimTo());

    viewModel.checkForUpdates();

    QCOMPARE(service.checkCalls, 1);

    service.FireCheckFinished(true, false, MakeInfo(QStringLiteral("1.3.0")));

    QCOMPARE(viewModel.GetState(), static_cast<int>(UpdateViewModel::UpToDate));
}

void UpdateViewModelTest::flightsimToReportsTheBundledCommbus()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true, FlightsimTo());

    QVERIFY(viewModel.IsCommbusBundled());

    viewModel.SetCommbusBundleResult({
        QStringLiteral("0.4.0"),
        {
            {QStringLiteral("MSFS 2020 (Steam)"), CommbusBundleStatus::UpToDate},
            {QStringLiteral("MSFS 2024 (Steam)"), CommbusBundleStatus::SimRunning},
            {QStringLiteral("MSFS 2024 (Microsoft Store)"), CommbusBundleStatus::Failed},
        }
    });

    QCOMPARE(viewModel.GetCommbusInstalledVersion(), QStringLiteral("0.4.0"));
    QCOMPARE(viewModel.GetCommbusLatestVersion(), QStringLiteral("0.4.0"));
    QVERIFY(viewModel.IsCommbusSimRunning());
    QCOMPARE(viewModel.GetCommbusFailedTargets(), QStringLiteral("MSFS 2024 (Microsoft Store)"));
    QVERIFY(!viewModel.IsCommbusInstallMissing());
    QVERIFY(!viewModel.IsCommbusUpdateAvailable());

    viewModel.SetCommbusBundleResult({QStringLiteral("0.4.0"), {}});

    QVERIFY(viewModel.IsCommbusInstallMissing());
    QVERIFY(viewModel.GetCommbusInstalledVersion().isEmpty());
    QVERIFY(!viewModel.IsCommbusSimRunning());
    QVERIFY(viewModel.GetCommbusFailedTargets().isEmpty());

    viewModel.SetCommbusBundleResult({
        QStringLiteral("0.4.0"),
        {{QStringLiteral("MSFS 2024 (Steam)"), CommbusBundleStatus::SimRunning}}
    });

    QVERIFY(viewModel.GetCommbusInstalledVersion().isEmpty());
    QVERIFY(viewModel.IsCommbusSimRunning());
}

void UpdateViewModelTest::flightsimToIgnoresTheCommbusFeed()
{
    FakeUpdateService service;
    UpdateViewModel viewModel(&service, UpdateViewModel::Notify, true, FlightsimTo());

    viewModel.SetCommbusBundleResult({
        QStringLiteral("0.4.0"),
        {{QStringLiteral("MSFS 2024 (Steam)"), CommbusBundleStatus::Installed}}
    });
    service.FireCommbusCheckFinished(true, QStringLiteral("0.3.0"), QStringLiteral("0.5.0"),
                                     QStringLiteral("https://example.com"));

    QVERIFY(!viewModel.IsCommbusUpdateAvailable());
    QCOMPARE(viewModel.GetCommbusInstalledVersion(), QStringLiteral("0.4.0"));
}

QTEST_GUILESS_MAIN(UpdateViewModelTest)

#include "tst_update_viewmodel.moc"
