#include "GithubUpdateService.h"

#include <utility>
#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkReply>
#include "CommbusInstallProbe.h"
#include "GithubReleaseParser.h"

namespace
{
    constexpr auto kZipRootDir = "gsx-integrator-client";
    constexpr int kTransferTimeoutMs = 30000;

    constexpr auto kApplyScript =
        R"PS(param([int]$AppPid, [string]$Source, [string]$Dest, [string]$ExeName, [int]$Relaunch)
$root = Split-Path -Parent $PSCommandPath
Start-Transcript -Path (Join-Path $root 'apply.log') -Append | Out-Null
Wait-Process -Id $AppPid -Timeout 60 -ErrorAction SilentlyContinue
if (-not (Test-Path (Join-Path $Dest $ExeName))) { Stop-Transcript | Out-Null; exit 1 }
robocopy $Source $Dest /MIR /XD (Join-Path $Dest 'maintenance') /R:20 /W:1
if ($LASTEXITCODE -ge 8) { Stop-Transcript | Out-Null; exit 1 }
if ($Relaunch -eq 1) { Start-Process -FilePath (Join-Path $Dest $ExeName) -WorkingDirectory $Dest }
Stop-Transcript | Out-Null
Remove-Item -Recurse -Force (Join-Path $root 'download'), (Join-Path $root 'staged') -ErrorAction SilentlyContinue
Remove-Item -Force $PSCommandPath -ErrorAction SilentlyContinue
)PS";

    QString HttpError(const QNetworkReply* reply)
    {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status > 0)
        {
            return QStringLiteral("HTTP %1").arg(status);
        }

        return reply->errorString();
    }
}

GithubUpdateService::GithubUpdateService(QString clientFeedUrl,
                                         QString commbusFeedUrl,
                                         QString currentVersion,
                                         QObject* parent)
    : QObject(parent),
      clientFeedUrl_(std::move(clientFeedUrl)),
      commbusFeedUrl_(std::move(commbusFeedUrl)),
      currentVersion_(std::move(currentVersion))
{
    network_.setTransferTimeout(kTransferTimeoutMs);
    DiscardStaged();
}

void GithubUpdateService::CheckForUpdates()
{
    if (!clientCheckReply_ && !clientFeedUrl_.isEmpty())
    {
        clientCheckReply_ = StartGet(clientFeedUrl_);
        connect(clientCheckReply_, &QNetworkReply::finished,
                this, &GithubUpdateService::OnClientCheckHttpFinished);
    }

    if (!commbusCheckReply_ && !commbusFeedUrl_.isEmpty())
    {
        commbusCheckReply_ = StartGet(commbusFeedUrl_);
        connect(commbusCheckReply_, &QNetworkReply::finished,
                this, &GithubUpdateService::OnCommbusCheckHttpFinished);
    }
}

void GithubUpdateService::DownloadAndStage(const UpdateInfo& info)
{
    if (shaReply_ || zipReply_ || extractProcess_)
    {
        return;
    }

    if (info.zipUrl.isEmpty() || info.shaUrl.isEmpty())
    {
        NotifyStageFinished(false, tr("The release has no download assets."));
        return;
    }

    DiscardStaged();
    pendingDownload_ = info;

    shaReply_ = StartGet(info.shaUrl);

    connect(shaReply_, &QNetworkReply::finished,
            this, &GithubUpdateService::OnShaHttpFinished);
}

void GithubUpdateService::DiscardStaged()
{
    if (shaReply_)
    {
        shaReply_->abort();
    }
    if (zipReply_)
    {
        zipReply_->abort();
    }

    QDir(UpdatesRootDir() + QStringLiteral("/download")).removeRecursively();
    QDir(UpdatesRootDir() + QStringLiteral("/staged")).removeRecursively();

    stagedVersion_.clear();
}

bool GithubUpdateService::HasStagedUpdate() const
{
    return !stagedVersion_.isEmpty();
}

bool GithubUpdateService::LaunchApplyHelper(const bool relaunch)
{
    if (helperLaunched_)
    {
        return true;
    }

    const QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    const QString stagedExe = StagedAppDir() + QStringLiteral("/") + exeName;
    if (stagedVersion_.isEmpty() || !QFile::exists(stagedExe))
    {
        return false;
    }

    const QString scriptPath = UpdatesRootDir() + QStringLiteral("/apply.ps1");
    QFile script(scriptPath);

    if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }

    script.write(kApplyScript);
    script.close();

    const QStringList arguments = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
        QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
        QStringLiteral("-File"), QDir::toNativeSeparators(scriptPath),
        QStringLiteral("-AppPid"), QString::number(QCoreApplication::applicationPid()),
        QStringLiteral("-Source"), QDir::toNativeSeparators(StagedAppDir()),
        QStringLiteral("-Dest"),
        QDir::toNativeSeparators(QCoreApplication::applicationDirPath()),
        QStringLiteral("-ExeName"), exeName,
        QStringLiteral("-Relaunch"), relaunch ? QStringLiteral("1") : QStringLiteral("0"),
    };

    helperLaunched_ =
        QProcess::startDetached(QStringLiteral("powershell.exe"), arguments);

    return helperLaunched_;
}

void GithubUpdateService::AddObserver(UpdateServiceObserver* observer)
{
    observers_.push_back(observer);
}

void GithubUpdateService::RemoveObserver(UpdateServiceObserver* observer)
{
    std::erase(observers_, observer);
}

void GithubUpdateService::OnClientCheckHttpFinished()
{
    QNetworkReply* reply = std::exchange(clientCheckReply_, nullptr);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        for (auto* observer : observers_)
        {
            observer->OnCheckFinished(false, false, {}, HttpError(reply));
        }

        return;
    }

    const auto info = ParseLatestRelease(reply->readAll());
    if (!info.has_value())
    {
        for (auto* observer : observers_)
        {
            observer->OnCheckFinished(false, false, {}, tr("Unexpected release feed format."));
        }

        return;
    }

    if (!stagedVersion_.isEmpty() && info->version != stagedVersion_)
    {
        DiscardStaged();
    }

    const bool available = IsNewerVersion(info->version, currentVersion_);
    for (auto* observer : observers_)
    {
        observer->OnCheckFinished(true, available, *info, {});
    }
}

void GithubUpdateService::OnCommbusCheckHttpFinished()
{
    QNetworkReply* reply = std::exchange(commbusCheckReply_, nullptr);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        for (auto* observer : observers_)
        {
            observer->OnCommbusCheckFinished(false, {}, {}, {});
        }

        return;
    }

    const auto info = ParseLatestRelease(reply->readAll());
    if (!info.has_value())
    {
        for (auto* observer : observers_)
        {
            observer->OnCommbusCheckFinished(false, {}, {}, {});
        }

        return;
    }

    const QString installed = DetectInstalledCommbusVersion(qEnvironmentVariable("GSXI_COMMBUS_COMMUNITY_DIR"));
    for (auto* observer : observers_)
    {
        observer->OnCommbusCheckFinished(true, installed, info->version, info->releasePageUrl);
    }
}

void GithubUpdateService::OnShaHttpFinished()
{
    QNetworkReply* reply = std::exchange(shaReply_, nullptr);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        NotifyStageFinished(false, HttpError(reply));
        return;
    }

    expectedSha256_ = ParseSha256File(reply->readAll());
    if (expectedSha256_.isEmpty())
    {
        NotifyStageFinished(false, tr("Invalid checksum file."));
        return;
    }

    QDir().mkpath(UpdatesRootDir() + QStringLiteral("/download"));
    zipReply_ = StartGet(pendingDownload_.zipUrl);
    connect(zipReply_, &QNetworkReply::downloadProgress, this,
            [this](const qint64 received, const qint64 total)
            {
                for (auto* observer : observers_)
                {
                    observer->OnDownloadProgress(received, total);
                }
            });
    connect(zipReply_, &QNetworkReply::finished,
            this, &GithubUpdateService::OnZipHttpFinished);
}

void GithubUpdateService::OnZipHttpFinished()
{
    QNetworkReply* reply = std::exchange(zipReply_, nullptr);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        NotifyStageFinished(false, HttpError(reply));
        return;
    }

    const QString zipPath = UpdatesRootDir() + QStringLiteral("/download/")
        + pendingDownload_.zipName;
    QFile zipFile(zipPath);
    if (!zipFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        NotifyStageFinished(false, tr("Could not write the download."));
        return;
    }
    zipFile.write(reply->readAll());
    zipFile.close();

    zipFile.open(QIODevice::ReadOnly);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&zipFile);
    zipFile.close();

    if (QString::fromLatin1(hash.result().toHex()) != expectedSha256_)
    {
        QFile::remove(zipPath);
        NotifyStageFinished(false, tr("Checksum mismatch. Download discarded."));
        return;
    }

    const QString stagedRoot = UpdatesRootDir() + QStringLiteral("/staged");
    (void)QDir().mkpath(stagedRoot);

    extractProcess_ = new QProcess(this);
    connect(extractProcess_, &QProcess::finished, this,
            [this](const int exitCode, QProcess::ExitStatus)
            {
                OnExtractFinished(exitCode);
            });

    extractProcess_->start(
        QStringLiteral("powershell.exe"),
        {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
            QStringLiteral("-NonInteractive"),
            QStringLiteral("-Command"),
            QStringLiteral("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
            .arg(QDir::toNativeSeparators(zipPath),
                 QDir::toNativeSeparators(stagedRoot))
        });
}

void GithubUpdateService::OnExtractFinished(const int exitCode)
{
    extractProcess_->deleteLater();
    extractProcess_ = nullptr;

    const QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    if (exitCode != 0 || !QFile::exists(StagedAppDir() + QStringLiteral("/") + exeName))
    {
        NotifyStageFinished(false, tr("Could not extract the update."));

        return;
    }

    stagedVersion_ = pendingDownload_.version;
    NotifyStageFinished(true, {});
}

QNetworkReply* GithubUpdateService::StartGet(const QString& url)
{
    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent",
                         QStringLiteral("gsx-integrator-client/%1")
                         .arg(currentVersion_).toUtf8());

    return network_.get(request);
}

QString GithubUpdateService::UpdatesRootDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/updates");
}

QString GithubUpdateService::StagedAppDir() const
{
    return UpdatesRootDir() + QStringLiteral("/staged/") + QLatin1String(kZipRootDir);
}

void GithubUpdateService::NotifyStageFinished(const bool ok, const QString& error)
{
    if (!ok)
    {
        expectedSha256_.clear();
    }
    for (auto* observer : observers_)
    {
        observer->OnStageFinished(ok, error);
    }
}
