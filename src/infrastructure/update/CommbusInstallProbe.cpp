#include "CommbusInstallProbe.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QVersionNumber>
#include "../simulator/SimulatorCandidates.h"
#include "../simulator/SimulatorUserFolders.h"

namespace
{
    constexpr auto kPackageDirName = "gsx-integrator-commbus";
    constexpr auto kCommunityPrefix = "Community";

    QByteArray ReadFileIfExists(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }
        return file.readAll();
    }
}

QString ParseInstalledPackagesPath(const QByteArray& userCfg)
{
    static const QRegularExpression kInstalledPackagesPath(QStringLiteral("InstalledPackagesPath\\s+\"([^\"]+)\""));

    const auto match = kInstalledPackagesPath.match(QString::fromUtf8(userCfg));
    if (!match.hasMatch())
    {
        return {};
    }

    QString path = match.captured(1).trimmed();
    path.replace(u'\\', u'/');

    return path;
}

QString InstalledPackagesPathOf(const QString& userHome, const SimulatorCandidate& candidate)
{
    return SimulatorLocalPath(
        userHome, ParseInstalledPackagesPath(ReadFileIfExists(SimulatorUserCfgPath(userHome, candidate))));
}

QString ParseManifestVersion(const QByteArray& manifestJson)
{
    const QJsonDocument document = QJsonDocument::fromJson(manifestJson);
    if (!document.isObject())
    {
        return {};
    }

    QString version = document.object()
                              .value(QStringLiteral("package_version"))
                              .toString()
                              .trimmed();
    if (QVersionNumber::fromString(version).isNull())
    {
        return {};
    }

    return version;
}

QStringList CommunityDirsUnder(const QString& packagesPath)
{
    if (packagesPath.isEmpty())
    {
        return {};
    }

    const QDir base(packagesPath);
    if (!base.exists())
    {
        return {};
    }

    QStringList dirs;
    for (const QString& name : base.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (name.startsWith(QLatin1String(kCommunityPrefix), Qt::CaseInsensitive))
        {
            dirs.append(base.filePath(name));
        }
    }

    return dirs;
}

QStringList CandidateCommunityDirs()
{
    return CandidateCommunityDirsIn(SimulatorUserHomes());
}

QStringList CandidateCommunityDirsIn(const QStringList& userHomes)
{
    QStringList result;
    for (const QString& userHome : userHomes)
    {
        for (const SimulatorCandidate& candidate : kSimulatorCandidates)
        {
            result.append(CommunityDirsUnder(InstalledPackagesPathOf(userHome, candidate)));
            result.append(SimulatorFallbackCommunityDir(userHome, candidate));
        }
    }

    result.removeDuplicates();

    return result;
}

QString DetectInstalledCommbusVersion(const QString& overrideDir)
{
    const QStringList communityDirs = overrideDir.isEmpty() ? CandidateCommunityDirs() : QStringList{overrideDir};

    QVersionNumber lowest;
    for (const QString& communityDir : communityDirs)
    {
        const QString manifestPath = communityDir + QStringLiteral("/")
            + QLatin1String(kPackageDirName)
            + QStringLiteral("/manifest.json");

        const QString version = ParseManifestVersion(ReadFileIfExists(manifestPath));
        if (version.isEmpty())
        {
            continue;
        }

        const QVersionNumber parsed = QVersionNumber::fromString(version);
        if (lowest.isNull() || parsed < lowest)
        {
            lowest = parsed;
        }
    }

    return lowest.isNull() ? QString() : lowest.toString();
}
