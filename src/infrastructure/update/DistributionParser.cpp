#include "DistributionParser.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace
{
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

Distribution ParseFlightsimToDistribution(const QByteArray& json)
{
    return {true, PageUrlOf(ReadObject(json))};
}

Distribution DistributionOfThisBuild([[maybe_unused]] const QByteArray& json)
{
#if defined(GSXI_FLIGHTSIM_TO)
    return ParseFlightsimToDistribution(json);
#else
    return {};
#endif
}
