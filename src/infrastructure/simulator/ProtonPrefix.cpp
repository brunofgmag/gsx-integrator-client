#include "ProtonPrefix.h"

#include <algorithm>
#include <QtCore/QFileInfo>
#include "CaseInsensitivePath.h"
#include "SteamLibraries.h"

namespace
{
    constexpr auto kPrefixSubPath = "pfx";
    constexpr auto kCompatDataDirName = "compatdata";
    constexpr auto kDosDevicesDirName = "dosdevices";
    constexpr auto kDriveCDirName = "drive_c";
    constexpr auto kUsersDirName = "users";
    constexpr auto kProtonUser = "steamuser";
    constexpr auto kRoamingSubPath = "AppData/Roaming";
    constexpr auto kLocalSubPath = "AppData/Local";

    std::vector<const SimulatorCandidate*> SteamSimulatorsNewestFirst()
    {
        std::vector<const SimulatorCandidate*> simulators;
        for (const SimulatorCandidate& candidate : kSimulatorCandidates)
        {
            if (candidate.steamAppId != 0)
            {
                simulators.push_back(&candidate);
            }
        }

        std::ranges::sort(simulators, [](const SimulatorCandidate* left, const SimulatorCandidate* right)
        {
            return left->steamAppId > right->steamAppId;
        });

        return simulators;
    }

    QString PrefixIn(const QString& steamappsDir, const unsigned appId)
    {
        return steamappsDir + u'/' + QLatin1String(kCompatDataDirName) + u'/' + QString::number(appId)
            + u'/' + QLatin1String(kPrefixSubPath);
    }

    bool NameIs(const QFileInfo& entry, const char* name)
    {
        return entry.fileName().compare(QLatin1String(name), Qt::CaseInsensitive) == 0;
    }

    QString DriveTarget(const QString& prefixRoot, const QChar letter)
    {
        const QString device = ResolveCaseInsensitive(
            prefixRoot, QLatin1String(kDosDevicesDirName) + u'/' + letter.toLower() + u':');
        const QFileInfo entry(device);
        if (entry.isSymLink())
        {
            return entry.symLinkTarget();
        }

        return entry.isDir() ? device : QString();
    }
}

std::vector<ProtonPrefix> FindProtonPrefixes(const QStringList& steamappsDirs)
{
    const QStringList libraries = SteamLibrarySteamappsDirs(steamappsDirs);

    std::vector<ProtonPrefix> prefixes;
    for (const SimulatorCandidate* simulator : SteamSimulatorsNewestFirst())
    {
        const auto library = std::ranges::find_if(libraries, [simulator](const QString& steamappsDir)
        {
            return QFileInfo(PrefixIn(steamappsDir, simulator->steamAppId)).isDir();
        });
        if (library != libraries.end())
        {
            prefixes.push_back({.simulator = simulator, .root = PrefixIn(*library, simulator->steamAppId)});
        }
    }

    return prefixes;
}

ProtonKnownFolders ProtonKnownFoldersOf(const QString& prefixRoot)
{
    const QString home = ResolveCaseInsensitive(
        prefixRoot, QLatin1String(kDriveCDirName) + u'/' + QLatin1String(kUsersDirName) + u'/'
        + QLatin1String(kProtonUser));

    return {
        .home = home,
        .roaming = ResolveCaseInsensitive(home, QLatin1String(kRoamingSubPath)),
        .local = ResolveCaseInsensitive(home, QLatin1String(kLocalSubPath))
    };
}

QString ProtonPrefixRootOfUserHome(const QString& userHome)
{
    const QFileInfo users(QFileInfo(userHome).absolutePath());
    const QFileInfo driveC(users.absolutePath());
    if (!NameIs(users, kUsersDirName) || !NameIs(driveC, kDriveCDirName))
    {
        return {};
    }

    const QString root = driveC.absolutePath();

    return QFileInfo(root + u'/' + QLatin1String(kDosDevicesDirName)).isDir() ? root : QString();
}

QString ProtonPathFromWindows(const QString& prefixRoot, const QString& windowsPath)
{
    QString path = windowsPath;
    path.replace(u'\\', u'/');
    if (path.size() < 2 || !path[0].isLetter() || path[1] != u':')
    {
        return {};
    }

    const QString target = DriveTarget(prefixRoot, path[0]);

    return target.isEmpty() ? QString() : ResolveCaseInsensitive(target, path.mid(2));
}

const ProtonPrefix* ActiveProtonPrefix(const std::vector<ProtonPrefix>& prefixes,
                                       const ProcessRunningCheck& isProcessRunning)
{
    const auto running = std::ranges::find_if(prefixes, [&isProcessRunning](const ProtonPrefix& prefix)
    {
        return isProcessRunning(QLatin1String(prefix.simulator->processName));
    });
    if (running != prefixes.end())
    {
        return &*running;
    }

    return prefixes.empty() ? nullptr : &prefixes.front();
}
