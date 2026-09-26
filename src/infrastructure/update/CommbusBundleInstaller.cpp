#include "CommbusBundleInstaller.h"

#include <algorithm>
#include <windows.h>
#include <tlhelp32.h>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QVersionNumber>
#include "CommbusInstallProbe.h"
#include "../simulator/SimulatorCandidates.h"

namespace
{
    constexpr auto kPackageDirName = "gsx-integrator-commbus";
    constexpr auto kVersionMarker = ".gsxi-version";
    constexpr auto kManifestName = "manifest.json";
    constexpr auto kOverrideLabel = "Community";

    QByteArray ReadFileIfExists(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return file.readAll();
    }

    QString PackageDirIn(const QString& communityPath)
    {
        return communityPath + u'/' + QLatin1String(kPackageDirName);
    }

    bool CopyDirRecursively(const QString& sourceDir, const QString& destDir)
    {
        const QDir source(sourceDir);
        if (!source.exists() || !QDir().mkpath(destDir))
        {
            return false;
        }

        const QFileInfoList entries = source.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

        return std::ranges::all_of(entries, [&destDir](const QFileInfo& entry)
        {
            const QString destPath = destDir + u'/' + entry.fileName();

            return entry.isDir()
                       ? CopyDirRecursively(entry.absoluteFilePath(), destPath)
                       : QFile::copy(entry.absoluteFilePath(), destPath);
        });
    }

    bool RemoveExistingPackageDir(const QString& packageDir)
    {
        const QFileInfo entry(packageDir);
        if (entry.isJunction() || entry.isSymbolicLink())
        {
            return QDir().rmdir(packageDir);
        }

        if (!QDir(packageDir).exists())
        {
            return true;
        }

        return QDir(packageDir).removeRecursively();
    }

    void WriteCommbusVersionMarker(const QString& packageDir, const QString& version)
    {
        QFile marker(packageDir + u'/' + QLatin1String(kVersionMarker));
        if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            marker.write(version.toUtf8());
        }
    }

    bool IsUpToDate(const QString& installedVersion, const QVersionNumber& bundled)
    {
        const QVersionNumber installed = QVersionNumber::fromString(installedVersion);

        return !installed.isNull() && installed >= bundled;
    }

    bool HasPackage(const QString& communityPath)
    {
        const QFileInfo entry(PackageDirIn(communityPath));

        return entry.exists() || entry.isJunction() || entry.isSymbolicLink();
    }

    QString BundledVersionIn(const QString& bundleDir)
    {
        return ParseManifestVersion(ReadFileIfExists(bundleDir + u'/' + QLatin1String(kManifestName)));
    }

    template <typename NeedsChange>
    std::vector<CommbusBundleTargetOutcome> BlockedTargets(const std::vector<CommbusInstallTarget>& targets,
                                                           const ProcessRunningCheck& isProcessRunning,
                                                           const NeedsChange& needsChange)
    {
        std::vector<CommbusBundleTargetOutcome> blocked;
        for (const CommbusInstallTarget& target : targets)
        {
            if (needsChange(target) && !target.processName.isEmpty() && isProcessRunning(target.processName))
            {
                blocked.push_back({target.label, CommbusBundleStatus::SimRunning});
            }
        }

        return blocked;
    }

    CommbusBundleStatus RemoveFrom(const CommbusInstallTarget& target)
    {
        if (!HasPackage(target.communityPath))
        {
            return CommbusBundleStatus::Absent;
        }

        return RemoveExistingPackageDir(PackageDirIn(target.communityPath))
                   ? CommbusBundleStatus::Removed
                   : CommbusBundleStatus::Failed;
    }

    CommbusBundleStatus InstallInto(const QString& bundleDir,
                                    const CommbusInstallTarget& target,
                                    const QString& bundledVersion,
                                    const ProcessRunningCheck& isProcessRunning)
    {
        if (bundledVersion.isEmpty())
        {
            return CommbusBundleStatus::Failed;
        }

        if (IsUpToDate(InstalledCommbusPackageVersion(target.communityPath),
                       QVersionNumber::fromString(bundledVersion)))
        {
            return CommbusBundleStatus::UpToDate;
        }

        if (!target.processName.isEmpty() && isProcessRunning(target.processName))
        {
            return CommbusBundleStatus::SimRunning;
        }

        const QString packageDir = PackageDirIn(target.communityPath);
        if (!RemoveExistingPackageDir(packageDir) || !CopyDirRecursively(bundleDir, packageDir))
        {
            return CommbusBundleStatus::Failed;
        }

        WriteCommbusVersionMarker(packageDir, bundledVersion);

        return CommbusBundleStatus::Installed;
    }
}

std::vector<CommbusInstallTarget> DetectCommbusInstallTargets(const QString& homeDir)
{
    std::vector<CommbusInstallTarget> targets;
    for (const SimulatorCandidate& candidate : kSimulatorCandidates)
    {
        const QString packagesPath = ParseInstalledPackagesPath(
            ReadFileIfExists(homeDir + u'/' + QLatin1String(candidate.userCfgSubPath)));
        if (packagesPath.isEmpty())
        {
            continue;
        }

        const QString communityPath = packagesPath + QStringLiteral("/Community");
        if (!QDir(communityPath).exists())
        {
            continue;
        }

        targets.push_back({
            QLatin1String(candidate.label), communityPath, QLatin1String(candidate.processName)
        });
    }

    return targets;
}

std::vector<CommbusInstallTarget> ResolveCommbusInstallTargets(const QString& overrideDir, const QString& homeDir)
{
    if (!overrideDir.isEmpty())
    {
        return {{QLatin1String(kOverrideLabel), QDir::fromNativeSeparators(overrideDir), {}}};
    }

    return DetectCommbusInstallTargets(homeDir);
}

QString InstalledCommbusPackageVersion(const QString& communityPath)
{
    const QString packageDir = PackageDirIn(communityPath);

    const QString marker = QString::fromUtf8(
        ReadFileIfExists(packageDir + u'/' + QLatin1String(kVersionMarker))).trimmed();
    if (!marker.isEmpty())
    {
        return marker;
    }

    return ParseManifestVersion(ReadFileIfExists(packageDir + u'/' + QLatin1String(kManifestName)));
}

CommbusBundleResult InstallCommbusBundle(const QString& bundleDir,
                                         const std::vector<CommbusInstallTarget>& targets,
                                         const ProcessRunningCheck& isProcessRunning)
{
    CommbusBundleResult result;
    result.bundledVersion = ParseManifestVersion(
        ReadFileIfExists(bundleDir + u'/' + QLatin1String(kManifestName)));

    for (const CommbusInstallTarget& target : targets)
    {
        result.targets.push_back({
            target.label, InstallInto(bundleDir, target, result.bundledVersion, isProcessRunning)
        });
    }

    return result;
}

CommbusBundleResult EnableCommbusBundle(const QString& bundleDir,
                                        const std::vector<CommbusInstallTarget>& targets,
                                        const ProcessRunningCheck& isProcessRunning)
{
    const QString bundledVersion = BundledVersionIn(bundleDir);
    const QVersionNumber bundled = QVersionNumber::fromString(bundledVersion);
    std::vector<CommbusBundleTargetOutcome> blocked = BlockedTargets(
        targets, isProcessRunning, [&bundled](const CommbusInstallTarget& target)
        {
            return !IsUpToDate(InstalledCommbusPackageVersion(target.communityPath), bundled);
        });
    if (!blocked.empty())
    {
        return {bundledVersion, std::move(blocked)};
    }

    return InstallCommbusBundle(bundleDir, targets, isProcessRunning);
}

CommbusBundleResult RemoveCommbusBundle(const std::vector<CommbusInstallTarget>& targets,
                                        const ProcessRunningCheck& isProcessRunning)
{
    std::vector<CommbusBundleTargetOutcome> blocked = BlockedTargets(
        targets, isProcessRunning, [](const CommbusInstallTarget& target)
        {
            return HasPackage(target.communityPath);
        });
    if (!blocked.empty())
    {
        return {{}, std::move(blocked)};
    }

    CommbusBundleResult result;
    for (const CommbusInstallTarget& target : targets)
    {
        result.targets.push_back({target.label, RemoveFrom(target)});
    }

    return result;
}

bool IsProcessRunning(const QString& exeName)
{
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    bool found = false;
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (exeName.compare(QString::fromWCharArray(entry.szExeFile), Qt::CaseInsensitive) == 0)
            {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    return found;
}
