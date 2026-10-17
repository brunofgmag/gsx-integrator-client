#include "SystemdSimulatorLauncher.h"

#include <utility>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileDevice>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include "../platform/XdgDirectories.h"
#include "SimulatorCandidates.h"
#include "SimulatorWatcherFiles.h"

namespace
{
    constexpr int kSystemctlTimeoutMs = 30000;
    constexpr auto kUnitSubPath = "systemd/user";

    constexpr QFileDevice::Permissions kScriptPermissions =
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner
        | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther;

    constexpr QFileDevice::Permissions kUnitPermissions =
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;

    bool WriteIfChanged(const QString& path, const QString& content, const QFileDevice::Permissions permissions)
    {
        QFile existing(path);
        if (existing.open(QIODevice::ReadOnly) && QString::fromUtf8(existing.readAll()) == content
            && existing.permissions() == permissions)
        {
            return true;
        }
        existing.close();

        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        {
            return false;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content.toUtf8()) < 0)
        {
            return false;
        }
        file.close();

        return QFile::setPermissions(path, permissions);
    }

    bool DiffersFrom(const QString& path, const QString& content)
    {
        QFile existing(path);

        return !existing.open(QIODevice::ReadOnly) || QString::fromUtf8(existing.readAll()) != content;
    }

    QStringList SimulatorProcessNames()
    {
        QStringList names;
        for (const SimulatorCandidate& candidate : kSimulatorCandidates)
        {
            const QString name = QLatin1String(candidate.processName);
            if (!names.contains(name))
            {
                names.append(name);
            }
        }
        names.sort();

        return names;
    }
}

SystemctlOutcome RunSystemctlUser(const QStringList& arguments)
{
    QProcess process;
    process.start(QStringLiteral("systemctl"), QStringList{QStringLiteral("--user")} + arguments);
    if (!process.waitForStarted(kSystemctlTimeoutMs))
    {
        return {};
    }

    if (!process.waitForFinished(kSystemctlTimeoutMs))
    {
        process.kill();
        process.waitForFinished();

        return {.started = true, .exitCode = -1};
    }

    return {.started = true, .exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1};
}

QString ClientLaunchPath()
{
    const QString appImage = qEnvironmentVariable("APPIMAGE");

    return QDir::cleanPath(appImage.isEmpty() ? QCoreApplication::applicationFilePath() : appImage);
}

SimulatorWatcherLocations DefaultWatcherLocations()
{
    return {
        .unitDir = XdgConfigHome() + u'/' + QLatin1String(kUnitSubPath),
        .scriptPath = ClientInstallDir() + u'/' + QLatin1String(kWatcherScriptName),
        .clientPath = ClientLaunchPath(),
        .installedClientPath = ClientInstallDir() + u'/' + QLatin1String(kClientAppImageName),
        .processNames = SimulatorProcessNames()
    };
}

SystemdSimulatorLauncher::SystemdSimulatorLauncher(SimulatorWatcherLocations locations, SystemctlRunner systemctl)
    : locations_(std::move(locations)),
      systemctl_(std::move(systemctl))
{
}

bool SystemdSimulatorLauncher::IsAvailable() const
{
    return systemctl_({QStringLiteral("show-environment")}).Succeeded();
}

bool SystemdSimulatorLauncher::IsEnabled() const
{
    return QFile::exists(UnitPath())
        && systemctl_({QStringLiteral("is-enabled"), QLatin1String(kWatcherUnitName)}).Succeeded();
}

LaunchWithSimulatorResult SystemdSimulatorLauncher::Set(const bool enabled)
{
    LaunchWithSimulatorResult result;
    if (enabled)
    {
        if (!IsAvailable())
        {
            result.unavailable = true;

            return result;
        }

        result.serviceFailed = !Enable();

        return result;
    }

    result.serviceFailed = !Disable();

    return result;
}

QString SystemdSimulatorLauncher::UnitPath() const
{
    return locations_.unitDir + u'/' + QLatin1String(kWatcherUnitName);
}

bool SystemdSimulatorLauncher::Enable() const
{
    const QString clientOverride = locations_.clientPath == locations_.installedClientPath
                                       ? QString()
                                       : locations_.clientPath;
    const SimulatorWatcherFiles files = WatcherFilesFor(locations_.scriptPath, locations_.processNames, clientOverride);

    const bool unitExisted = QFile::exists(UnitPath());
    const bool filesChanged = DiffersFrom(UnitPath(), files.unit) || DiffersFrom(locations_.scriptPath, files.script);

    if (!WriteIfChanged(locations_.scriptPath, files.script, kScriptPermissions)
        || !WriteIfChanged(UnitPath(), files.unit, kUnitPermissions))
    {
        return false;
    }

    const QString unitName = QLatin1String(kWatcherUnitName);
    if (!systemctl_({QStringLiteral("daemon-reload")}).Succeeded()
        || !systemctl_({QStringLiteral("enable"), QStringLiteral("--now"), unitName}).Succeeded())
    {
        return false;
    }

    return !(unitExisted && filesChanged) || systemctl_({QStringLiteral("restart"), unitName}).Succeeded();
}

bool SystemdSimulatorLauncher::Disable() const
{
    const bool unitExists = QFile::exists(UnitPath());
    const bool scriptExists = QFile::exists(locations_.scriptPath);
    if (!unitExists && !scriptExists)
    {
        return true;
    }

    const bool available = IsAvailable();
    if (unitExists && available
        && !systemctl_({QStringLiteral("disable"), QStringLiteral("--now"), QLatin1String(kWatcherUnitName)})
                .Succeeded())
    {
        return false;
    }

    const bool removed = (!unitExists || QFile::remove(UnitPath()))
        && (!scriptExists || QFile::remove(locations_.scriptPath));
    if (available)
    {
        (void)systemctl_({QStringLiteral("daemon-reload")});
    }

    return removed;
}
