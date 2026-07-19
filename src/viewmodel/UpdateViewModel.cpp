#include "UpdateViewModel.h"

#include <utility>
#include <QtCore/QCoreApplication>
#include <QtCore/QVersionNumber>

namespace
{
    constexpr int kStartupCheckDelayMs = 3000;
    constexpr int kPeriodicCheckIntervalMs = 6 * 60 * 60 * 1000;

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
}

UpdateViewModel::UpdateViewModel(UpdateService* service,
                                 QString currentVersion,
                                 const int initialMode,
                                 const bool updatesEnabled,
                                 QObject* parent)
    : QObject(parent),
      service_(service),
      currentVersion_(std::move(currentVersion)),
      mode_(initialMode),
      updatesEnabled_(updatesEnabled)
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

QString UpdateViewModel::GetCurrentVersion() const
{
    return currentVersion_;
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

QString UpdateViewModel::GetCommbusInstalledVersion() const
{
    return commbusInstalledVersion_;
}

QString UpdateViewModel::GetCommbusLatestVersion() const
{
    return commbusLatestVersion_;
}

QString UpdateViewModel::GetCommbusReleaseUrl() const
{
    return commbusReleaseUrl_;
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
    if (!updateKnown_ || state_ == Downloading || state_ == ReadyToRestart)
    {
        return;
    }

    BeginDownload(mode_ != Auto);
}

void UpdateViewModel::restartNow()
{
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
    if (mode_ == mode)
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
    return mode_ == Auto && service_->HasStagedUpdate();
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

    if (service_->HasStagedUpdate())
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
                                             const QString& latestVersion,
                                             const QString& releaseUrl)
{
    if (!ok)
    {
        return;
    }
    commbusInstalledVersion_ = installedVersion;
    commbusLatestVersion_ = latestVersion;
    commbusReleaseUrl_ = releaseUrl;
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
