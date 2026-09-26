#include "DistributionParser.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace
{
    constexpr auto kFlightsimToChannel = "flightsim.to";
    constexpr auto kFlightsimToHome = "https://flightsim.to/";

    QJsonObject ReadObject(const QByteArray& json)
    {
        const QJsonDocument document = QJsonDocument::fromJson(json);

        return document.isObject() ? document.object() : QJsonObject();
    }

    QString PageUrlOf(const QJsonObject& object)
    {
        const QString pageUrl = object.value(QStringLiteral("pageUrl")).toString().trimmed();

        return pageUrl.isEmpty() ? QString::fromLatin1(kFlightsimToHome) : pageUrl;
    }
}

Distribution ParseDistribution(const QByteArray& json)
{
    const QJsonObject object = ReadObject(json);
    const QString channel = object.value(QStringLiteral("channel")).toString().trimmed();
    if (channel.compare(QLatin1String(kFlightsimToChannel), Qt::CaseInsensitive) != 0)
    {
        return {};
    }

    return {true, PageUrlOf(object)};
}

Distribution ParseFlightsimToDistribution(const QByteArray& json)
{
    return {true, PageUrlOf(ReadObject(json))};
}
