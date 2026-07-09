#include "CommbusInstallProbe.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QVersionNumber>

namespace
{
constexpr auto kPackageDirName = "gsx-integrator-commbus";

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
    static const QRegularExpression kInstalledPackagesPath(
        QStringLiteral("InstalledPackagesPath\\s+\"([^\"]+)\""));
    const auto match = kInstalledPackagesPath.match(QString::fromUtf8(userCfg));
    if (!match.hasMatch())
    {
        return {};
    }
    return QDir::fromNativeSeparators(match.captured(1).trimmed());
}

QString ParseManifestVersion(const QByteArray& manifestJson)
{
    const QJsonDocument document = QJsonDocument::fromJson(manifestJson);
    if (!document.isObject())
    {
        return {};
    }
    const QString version = document.object()
                                .value(QStringLiteral("package_version"))
                                .toString()
                                .trimmed();
    if (QVersionNumber::fromString(version).isNull())
    {
        return {};
    }
    return version;
}

QStringList CandidateCommunityDirs()
{
    const QString localAppData =
        QDir::fromNativeSeparators(qEnvironmentVariable("LOCALAPPDATA"));
    const QString appData = QDir::fromNativeSeparators(qEnvironmentVariable("APPDATA"));

    const QStringList simBaseDirs = {
        localAppData + QStringLiteral("/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache"),
        localAppData + QStringLiteral("/Packages/Microsoft.Limitless_8wekyb3d8bbwe/LocalCache"),
        appData + QStringLiteral("/Microsoft Flight Simulator"),
        appData + QStringLiteral("/Microsoft Flight Simulator 2024"),
    };

    QStringList result;
    for (const QString& base : simBaseDirs)
    {
        const QString packagesPath =
            ParseInstalledPackagesPath(ReadFileIfExists(base + QStringLiteral("/UserCfg.opt")));
        if (!packagesPath.isEmpty())
        {
            result.append(packagesPath + QStringLiteral("/Community"));
        }
        result.append(base + QStringLiteral("/Packages/Community"));
    }
    result.removeDuplicates();
    return result;
}

QString DetectInstalledCommbusVersion(const QString& overrideDir)
{
    const QStringList communityDirs =
        overrideDir.isEmpty() ? CandidateCommunityDirs() : QStringList{overrideDir};

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
