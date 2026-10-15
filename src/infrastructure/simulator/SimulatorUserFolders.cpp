#include "SimulatorUserFolders.h"

#include <string>
#include <QtCore/QStandardPaths>
#include "ProtonPrefix.h"
#include "SteamLibraries.h"

namespace
{
    std::filesystem::path PathOf(const QString& directory)
    {
        const QByteArray utf8 = directory.toUtf8();

        return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.constData()),
                                                    static_cast<std::size_t>(utf8.size())));
    }

    QString MachineHome()
    {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
}

QStringList SimulatorUserHomes(const QString& machineHome)
{
#ifdef Q_OS_WIN
    return {machineHome};
#else
    QStringList homes;
    for (const ProtonPrefix& prefix : FindProtonPrefixes(DefaultSteamappsDirs(machineHome)))
    {
        homes.append(ProtonKnownFoldersOf(prefix.root).home);
    }

    return homes;
#endif
}

QStringList SimulatorUserHomes()
{
    return SimulatorUserHomes(MachineHome());
}

QString SimulatorLocalPath(const QString& userHome, const QString& windowsPath)
{
    const QString prefixRoot = ProtonPrefixRootOfUserHome(userHome);
    if (prefixRoot.isEmpty())
    {
        return windowsPath;
    }

    return ProtonPathFromWindows(prefixRoot, windowsPath);
}

QString ActiveSimulatorRoamingAppData(const QString& machineHome, const ProcessRunningCheck& isProcessRunning)
{
    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(machineHome));
    const ProtonPrefix* active = ActiveProtonPrefix(prefixes, isProcessRunning);

    return active == nullptr ? QString() : ProtonKnownFoldersOf(active->root).roaming;
}

QString SimulatorRoamingAppDataDir()
{
    const QString fromEnvironment = qEnvironmentVariable("APPDATA");
    if (!fromEnvironment.isEmpty())
    {
        return fromEnvironment;
    }

#ifdef Q_OS_WIN
    return {};
#else
    return ActiveSimulatorRoamingAppData(MachineHome(), IsProcessRunning);
#endif
}

std::optional<std::filesystem::path> SimulatorRoamingAppData()
{
    const QString directory = SimulatorRoamingAppDataDir();
    if (directory.isEmpty())
    {
        return std::nullopt;
    }

    return PathOf(directory);
}
