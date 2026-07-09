#include "GithubReleaseParser.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QVersionNumber>

namespace
{
    QString StripTagPrefix(const QString& tag)
    {
        QString trimmed = tag.trimmed();
        if (trimmed.startsWith(u'v') || trimmed.startsWith(u'V'))
        {
            return trimmed.mid(1);
        }

        return trimmed;
    }
}

std::optional<UpdateInfo> ParseLatestRelease(const QByteArray& json)
{
    const QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
    {
        return std::nullopt;
    }

    const QJsonObject release = document.object();
    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    if (tag.trimmed().isEmpty())
    {
        return std::nullopt;
    }

    UpdateInfo info;
    info.version = StripTagPrefix(tag);
    info.releasePageUrl = release.value(QStringLiteral("html_url")).toString();

    for (const auto assets = release.value(QStringLiteral("assets")).toArray(); const QJsonValue value : assets)
    {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QString url = asset.value(QStringLiteral("browser_download_url")).toString();

        if (!name.startsWith(QStringLiteral("gsx-integrator-client-")))
        {
            continue;
        }

        if (name.endsWith(QStringLiteral(".zip")))
        {
            info.zipName = name;
            info.zipUrl = url;
        }
        else if (name.endsWith(QStringLiteral(".zip.sha256")))
        {
            info.shaUrl = url;
        }
    }

    return info;
}

QString ParseSha256File(const QByteArray& content)
{
    const QString text = QString::fromUtf8(content).trimmed();
    QString hash = text.section(QRegularExpression(QStringLiteral("\\s")), 0, 0).toLower();

    static const QRegularExpression kHexHash(QStringLiteral("^[0-9a-f]{64}$"));
    if (!kHexHash.match(hash).hasMatch())
    {
        return {};
    }

    return hash;
}

bool IsNewerVersion(const QString& tagName, const QString& currentVersion)
{
    const QVersionNumber latest = QVersionNumber::fromString(StripTagPrefix(tagName));
    const QVersionNumber current = QVersionNumber::fromString(StripTagPrefix(currentVersion));

    if (latest.isNull() || current.isNull())
    {
        return false;
    }

    return latest > current;
}
