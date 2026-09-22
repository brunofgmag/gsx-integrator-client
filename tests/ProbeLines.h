#ifndef GSX_INTEGRATOR_CLIENT_TESTS_PROBELINES_H
#define GSX_INTEGRATOR_CLIENT_TESTS_PROBELINES_H

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "../src/infrastructure/probe/ProbeLog.h"

inline QStringList ProbeLines(const QString& fileName)
{
    QFile file(probe::RunLocation() + QLatin1Char('/') + fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }

    QStringList payloads;
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines)
    {
        const qsizetype marker = line.indexOf(QStringLiteral("] "));
        payloads.append(marker < 0 ? line : line.mid(marker + 2));
    }

    return payloads;
}

#endif // GSX_INTEGRATOR_CLIENT_TESTS_PROBELINES_H
