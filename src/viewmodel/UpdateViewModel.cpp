#include "UpdateViewModel.h"

#include <algorithm>
#include <utility>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QVersionNumber>

namespace
{
    constexpr int kStartupCheckDelayMs = 3000;
    constexpr int kPeriodicCheckIntervalMs = 6 * 60 * 60 * 1000;
    constexpr auto kTargetSeparator = ", ";
    constexpr auto kInstallerUrl = "https://github.com/brunofgmag/gsx-integrator-installer/releases/latest";

    bool IsVersionNewer(const QString& candidate, const QString& reference)
    {
        const QVersionNumber lhs = QVersionNumber::fromString(candidate);
        const QVersionNumber rhs = QVersionNumber::fromString(reference);
        if (lhs.isNull() || rhs.isNull())
        {
            return false;
        }
        return lhs > rhs;
    }

    bool HasTargetWith(const CommbusBundleResult& result, const std::initializer_list<CommbusBundleStatus> statuses)
    {
        return std::ranges::any_of(result.targets, [statuses](const CommbusBundleTargetOutcome& target)
        {
            return std::ranges::find(statuses, target.status) != statuses.end();
        });
    }

    QString LabelsOfTargetsWith(const CommbusBundleResult& result, const CommbusBundleStatus status)
    {
        QStringList labels;
        for (const CommbusBundleTargetOutcome& target : result.targets)
        {
            if (target.status == status)
            {
                labels.append(target.label);
            }
        }

        return labels.join(QLatin1String(kTargetSeparator));
    }
}

UpdateViewModel::UpdateViewModel(UpdateService* service,
                                 const int initialMode,
                                 const bool updatesEnabled,
                                 Distribution distribution,
                                 QObject* parent)
    : QObject(parent),
      service_(service),
      mode_(distribution.flightsimTo ? Notify : initialMode),
      updatesEnabled_(updatesEnabled),
      distribution_(std::move(distribution))
{
    service_->AddObserver(this);

    if (!updatesEnabled_)
    {
        return;
    }

    startupTimer_.setSingleShot(true);
    connect(&startupTimer_, &QTimer::timeout,
            this, &UpdateViewModel::StartBackgroundCheck);
    startupTimer_.start(kStartupCheckDelayMs);

    connect(&periodicTimer_, &QTimer::timeout,
            this, &UpdateViewModel::StartBackgroundCheck);
    periodicTimer_.start(kPeriodicCheckIntervalMs);
}

UpdateViewModel::~UpdateViewModel()
{
    service_->RemoveObserver(this);
}

int UpdateViewModel::GetState() const
{
    return state_;
}

bool UpdateViewModel::IsUpdateAvailable() const
{
    return updateKnown_;
}

bool UpdateViewModel::IsReadyToRestart() const
{
    return state_ == ReadyToRestart;
}

QString UpdateViewModel::GetStatusText() const
{
    switch (state_)
    {
    case Checking:
        return tr("Checking for updates…");
    case UpToDate:
        return tr("Up to date");
    case UpdateAvailable:
        return tr("Update available · v%1").arg(latest_.version);
    case Downloading:
        return tr("Downloading v%1").arg(latest_.version);
    case ReadyToRestart:
        return tr("Update ready: restart to apply");
    case Error:
        return errorMessage_;
    default:
        return {};
    }
}

bool UpdateViewModel::IsDownloading() const
{
    return state_ == Downloading;
}

bool UpdateViewModel::HasError() const
{
    return state_ == Error;
}

bool UpdateViewModel::CanDownload() const
{
    return AreDownloadsAllowed() && state_ == UpdateAvailable;
}

bool UpdateViewModel::CanCheckForUpdates() const
{
    return updatesEnabled_ && (state_ == Idle || state_ == UpToDate || state_ == Error);
}

QString UpdateViewModel::GetLatestVersion() const
{
    return latest_.version;
}

double UpdateViewModel::GetProgress() const
{
    return progress_;
}

QString UpdateViewModel::GetErrorMessage() const
{
    return errorMessage_;
}

QString UpdateViewModel::GetReleaseUrl() const
{
    return latest_.releasePageUrl;
}

bool UpdateViewModel::AreChecksEnabled() const
{
    return updatesEnabled_;
}

bool UpdateViewModel::IsCommbusUpdateAvailable() const
{
    return commbusUpdateAvailable_;
}

bool UpdateViewModel::IsCommbusInstallMissing() const
{
    return commbusInstallMissing_;
}

QString UpdateViewModel::GetCommbusInstalledVersion() const
{
    return commbusInstalledVersion_;
}

QString UpdateViewModel::GetCommbusLatestVersion() const
{
    return commbusLatestVersion_;
}

QString UpdateViewModel::GetInstallerUrl() const
{
    return QLatin1String(kInstallerUrl);
}

bool UpdateViewModel::IsCommbusBundled() const
{
    return distribution_.flightsimTo;
}

bool UpdateViewModel::IsCommbusSimRunning() const
{
    return commbusSimRunning_;
}

QString UpdateViewModel::GetCommbusFailedTargets() const
{
    return commbusFailedTargets_;
}

bool UpdateViewModel::AreDownloadsAllowed() const
{
    return !distribution_.flightsimTo;
}

QString UpdateViewModel::GetExternalDownloadUrl() const
{
    return distribution_.pageUrl;
}

void UpdateViewModel::checkForUpdates()
{
    if (!updatesEnabled_ || state_ == Checking || state_ == Downloading)
    {
        return;
    }
    explicitCheck_ = true;
    errorMessage_.clear();
    SetState(Checking);
    service_->CheckForUpdates();
}

void UpdateViewModel::downloadAndInstall()
{
    if (!AreDownloadsAllowed() || !updateKnown_ || state_ == Downloading || state_ == ReadyToRestart)
    {
        return;
    }

    BeginDownload(mode_ != Auto);
}

void UpdateViewModel::restartNow()
{
    if (!AreDownloadsAllowed())
    {
        return;
    }

    if (service_->LaunchApplyHelper(true))
    {
        QCoreApplication::exit(0);
        return;
    }

    errorMessage_ = tr("Could not start the updater.");

    SetState(Error);
}

void UpdateViewModel::SetMode(const int mode)
{
    if (!AreDownloadsAllowed() || mode_ == mode)
    {
        return;
    }

    mode_ = mode;
    if (updatesEnabled_ && mode_ == Auto && state_ == UpdateAvailable)
    {
        restartWhenStaged_ = false;
        SetState(Downloading);
        service_->DownloadAndStage(latest_);
    }
}

bool UpdateViewModel::ShouldApplyOnExit() const
{
    return AreDownloadsAllowed() && mode_ == Auto && service_->HasStagedUpdate();
}

void UpdateViewModel::SetCommbusBundleResult(const CommbusBundleResult& result)
{
    const bool present = HasTargetWith(result, {CommbusBundleStatus::Installed, CommbusBundleStatus::UpToDate});

    commbusInstalledVersion_ = present ? result.bundledVersion : QString();
    commbusLatestVersion_ = result.bundledVersion;
    commbusUpdateAvailable_ = false;
    commbusInstallMissing_ = result.targets.empty();
    commbusSimRunning_ = HasTargetWith(result, {CommbusBundleStatus::SimRunning});
    commbusFailedTargets_ = LabelsOfTargetsWith(result, CommbusBundleStatus::Failed);

    emit CommbusChanged();
}

void UpdateViewModel::StartSimulatorAddons(SimulatorAddonService* addons, const bool commbusManaged)
{
    addons_ = addons;
    commbusManaged_ = commbusManaged;
    launchWithSimulator_ = addons_->IsLaunchedWithSimulator();
    if (commbusManaged_)
    {
        SetCommbusBundleResult(addons_->InstallCommbus());
    }

    emit AddonsChanged();
}

bool UpdateViewModel::AreAddonsAvailable() const
{
    return addons_ != nullptr;
}

bool UpdateViewModel::IsLaunchWithSimulator() const
{
    return launchWithSimulator_;
}

void UpdateViewModel::SetLaunchWithSimulator(const bool enabled)
{
    if (addons_ == nullptr || launchWithSimulator_ == enabled)
    {
        return;
    }

    const LaunchWithSimulatorResult result = addons_->SetLaunchedWithSimulator(enabled);
    launchWithSimulator_ = addons_->IsLaunchedWithSimulator();

    if (result.noSimulator)
    {
        SetAddonNotice(AddonNotice::NoSimulator);
        return;
    }

    SetAddonNotice(result.failedTargets.isEmpty() ? AddonNotice::None : AddonNotice::ExeXmlFailed,
                   result.failedTargets.join(QLatin1String(kTargetSeparator)));
}

bool UpdateViewModel::IsCommbusManaged() const
{
    return commbusManaged_;
}

void UpdateViewModel::SetCommbusManaged(const bool managed)
{
    if (addons_ == nullptr || commbusManaged_ == managed)
    {
        return;
    }

    const CommbusBundleResult result = managed ? addons_->EnableCommbus() : addons_->RemoveCommbus();
    const QString running = LabelsOfTargetsWith(result, CommbusBundleStatus::SimRunning);
    if (!running.isEmpty())
    {
        SetAddonNotice(AddonNotice::SimulatorRunning, running);
        return;
    }

    commbusManaged_ = managed;
    if (managed)
    {
        SetCommbusBundleResult(result);
    }
    else
    {
        ClearInstalledCommbus();
    }

    const QString failed = LabelsOfTargetsWith(result, CommbusBundleStatus::Failed);
    const AddonNotice failure = managed ? AddonNotice::CommbusInstallFailed : AddonNotice::CommbusRemoveFailed;
    SetAddonNotice(failed.isEmpty() ? AddonNotice::None : failure, failed);
}

bool UpdateViewModel::IsCommbusRemoved() const
{
    return addons_ != nullptr && !commbusManaged_;
}

QString UpdateViewModel::GetAddonNotice() const
{
    switch (addonNotice_)
    {
    case AddonNotice::SimulatorRunning:
        return tr("Close %1 before changing the CommBus plugin.").arg(addonNoticeTargets_);
    case AddonNotice::CommbusInstallFailed:
        return tr("Could not install CommBus in %1.").arg(addonNoticeTargets_);
    case AddonNotice::CommbusRemoveFailed:
        return tr("Could not remove CommBus from %1.").arg(addonNoticeTargets_);
    case AddonNotice::ExeXmlFailed:
        return tr("Could not update EXE.xml for %1.").arg(addonNoticeTargets_);
    case AddonNotice::NoSimulator:
        return tr("No MSFS 2020 or 2024 installation found.");
    default:
        return {};
    }
}

void UpdateViewModel::SetAddonNotice(const AddonNotice notice, const QString& targets)
{
    addonNotice_ = notice;
    addonNoticeTargets_ = targets;

    emit AddonsChanged();
}

void UpdateViewModel::ClearInstalledCommbus()
{
    commbusInstalledVersion_.clear();
    commbusInstallMissing_ = false;
    commbusSimRunning_ = false;
    commbusFailedTargets_.clear();

    emit CommbusChanged();
}

void UpdateViewModel::OnCheckFinished(const bool ok, const bool updateAvailable,
                                      const UpdateInfo& info, const QString& error)
{
    const bool wasExplicit = std::exchange(explicitCheck_, false);

    if (!ok)
    {
        if (wasExplicit)
        {
            errorMessage_ = error;
            SetState(Error);
        }
        else if (state_ == Checking)
        {
            SetState(Idle);
        }
        return;
    }

    latest_ = info;
    updateKnown_ = updateAvailable;
    errorMessage_.clear();

    if (AreDownloadsAllowed() && service_->HasStagedUpdate())
    {
        SetState(ReadyToRestart);
        return;
    }

    if (!updateAvailable)
    {
        SetState(UpToDate);
        return;
    }

    if (mode_ == Auto)
    {
        BeginDownload(false);
        return;
    }

    SetState(UpdateAvailable);
}

void UpdateViewModel::OnCommbusCheckFinished(const bool ok,
                                             const QString& installedVersion,
                                             const QString& latestVersion)
{
    if (!ok || IsCommbusBundled())
    {
        return;
    }
    commbusInstalledVersion_ = installedVersion;
    commbusLatestVersion_ = latestVersion;
    commbusInstallMissing_ = installedVersion.isEmpty();
    commbusUpdateAvailable_ = !installedVersion.isEmpty()
        && IsVersionNewer(latestVersion, installedVersion);

    emit CommbusChanged();
}

void UpdateViewModel::OnDownloadProgress(const qint64 received, const qint64 total)
{
    progress_ = total > 0 ? static_cast<double>(received) / static_cast<double>(total) : 0.0;

    emit ProgressChanged();
}

void UpdateViewModel::OnStageFinished(const bool ok, const QString& error)
{
    if (!ok)
    {
        restartWhenStaged_ = false;
        errorMessage_ = error;
        SetState(Error);
        return;
    }

    SetState(ReadyToRestart);
    if (std::exchange(restartWhenStaged_, false))
    {
        restartNow();
    }
}

void UpdateViewModel::BeginDownload(const bool restartWhenStaged)
{
    restartWhenStaged_ = restartWhenStaged;
    progress_ = 0.0;
    emit ProgressChanged();
    SetState(Downloading);
    service_->DownloadAndStage(latest_);
}

void UpdateViewModel::StartBackgroundCheck()
{
    if (mode_ == Manual || state_ == Checking || state_ == Downloading)
    {
        return;
    }
    explicitCheck_ = false;
    service_->CheckForUpdates();
}

void UpdateViewModel::SetState(const State state)
{
    if (state_ == state)
    {
        emit StateChanged();
        return;
    }

    state_ = state;

    emit StateChanged();
}
