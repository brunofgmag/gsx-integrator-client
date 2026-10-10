#ifndef GSX_INTEGRATOR_CLIENT_TESTS_RECORDEDWIRE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_RECORDEDWIRE_H

#include <vector>
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

struct RecordedMessage
{
    QString stamp;
    QJsonObject message;

    [[nodiscard]] QString Type() const
    {
        return message.value("type").toString();
    }

    [[nodiscard]] QString Path() const
    {
        return message.value("path").toString();
    }

    [[nodiscard]] QJsonValue Value() const
    {
        return message.value("value");
    }
};

[[nodiscard]] inline std::vector<RecordedMessage> ParseRecordedWire(const QByteArray& text)
{
    std::vector<RecordedMessage> wire;

    for (const QByteArray& line : text.split('\n'))
    {
        const qsizetype json = line.indexOf('{');
        if (json < 0)
        {
            continue;
        }

        wire.push_back({QString::fromLatin1(line.left(line.indexOf(' '))),
                        QJsonDocument::fromJson(line.mid(json)).object()});
    }

    return wire;
}

[[nodiscard]] inline std::vector<RecordedMessage> LoadRecordedWire(const QString& fileName)
{
    QFile file(QStringLiteral(GSX_FIXTURES_DIR) + QLatin1Char('/') + fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    return ParseRecordedWire(file.readAll());
}

#endif // GSX_INTEGRATOR_CLIENT_TESTS_RECORDEDWIRE_H
