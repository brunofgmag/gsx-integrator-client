#include "AppImageUpdate.h"

#include <filesystem>
#include <system_error>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileDevice>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QSettings>
#include <QtCore/QVersionNumber>
#include "../platform/ChildEnvironment.h"

namespace
{
    constexpr auto kStagedSuffix = ".new";
    constexpr auto kInstallDirName = "gsx-integrator-client";
    constexpr auto kInstalledAppImageName = "gsx-integrator-client.AppImage";
    constexpr auto kInstallRecordName = "install.ini";
    constexpr auto kDisplayVersionKey = "DisplayVersion";
    constexpr auto kRelaunchScript =
        "i=0; while kill -0 \"$1\" 2>/dev/null && [ \"$i\" -lt 150 ]; do i=$((i+1)); sleep 0.2; done; "
        "exec \"$0\"";
}

QString StagedAppImagePathFor(const QString& appImageEnvironment)
{
    return appImageEnvironment.isEmpty()
               ? QString()
               : QFileInfo(appImageEnvironment).absoluteFilePath() + QLatin1String(kStagedSuffix);
}

AppImageUpdateTarget ResolveAppImageUpdateTarget(const QString& appImageEnvironment)
{
    if (appImageEnvironment.isEmpty() || !QFileInfo(appImageEnvironment).isFile())
    {
        return {.error = QCoreApplication::translate(
            "GithubUpdateService",
            "This copy is not running from an AppImage, so it cannot update itself. Download the new AppImage from the release page.")};
    }

    const QFileInfo appImage(appImageEnvironment);
    if (!QFileInfo(appImage.absolutePath()).isWritable())
    {
        return {.error = QCoreApplication::translate(
                             "GithubUpdateService",
                             "GSX Integrator cannot write to %1, so it cannot update itself there.")
                             .arg(QDir::toNativeSeparators(appImage.absolutePath()))};
    }

    return {
        .appImagePath = appImage.absoluteFilePath(),
        .stagedPath = StagedAppImagePathFor(appImageEnvironment),
        .error = {}
    };
}

bool InstallStagedAppImage(const QString& stagedPath, const QString& appImagePath)
{
    QFile staged(stagedPath);
    if (!staged.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
                               | QFileDevice::ReadGroup | QFileDevice::ExeGroup
                               | QFileDevice::ReadOther | QFileDevice::ExeOther))
    {
        return false;
    }

    std::error_code error;
    std::filesystem::rename(std::filesystem::path(stagedPath.toStdString()),
                            std::filesystem::path(appImagePath.toStdString()), error);

    return !error;
}

bool RecordInstalledVersion(const QString& appImagePath, const QString& dataHome, const QString& version)
{
    const QString installDir = QDir::cleanPath(dataHome + u'/' + QLatin1String(kInstallDirName));
    const QFileInfo appImage(appImagePath);
    if (!QFileInfo(installDir).isDir()
        || appImage.fileName() != QLatin1String(kInstalledAppImageName)
        || QFileInfo(appImage.absolutePath()).canonicalFilePath() != QFileInfo(installDir).canonicalFilePath())
    {
        return false;
    }

    QSettings settings(installDir + u'/' + QLatin1String(kInstallRecordName), QSettings::IniFormat);
    settings.setValue(QLatin1String(kDisplayVersionKey), QVersionNumber::fromString(version).toString());
    settings.sync();

    return settings.status() == QSettings::NoError;
}

QStringList AppImageRelaunchArguments(const qint64 appPid, const QString& appImagePath)
{
    return {
        QStringLiteral("-c"),
        QLatin1String(kRelaunchScript),
        appImagePath,
        QString::number(appPid)
    };
}

bool RelaunchAppImage(const qint64 appPid, const QString& appImagePath)
{
    QProcess relaunch;
    relaunch.setProgram(QStringLiteral("/bin/sh"));
    relaunch.setArguments(AppImageRelaunchArguments(appPid, appImagePath));
    relaunch.setProcessEnvironment(ChildProcessEnvironment(QProcessEnvironment::systemEnvironment()));

    return relaunch.startDetached();
}
