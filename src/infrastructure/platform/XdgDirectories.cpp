#include "XdgDirectories.h"

#include <QtCore/QDir>

namespace
{
    constexpr auto kClientInstallDirName = "gsx-integrator-client";

    QString AbsoluteEnvironmentDir(const char* variable, const QString& fallbackUnderHome)
    {
        const QString value = qEnvironmentVariable(variable);
        if (!value.isEmpty() && QDir::isAbsolutePath(value))
        {
            return QDir::cleanPath(value);
        }

        return QDir::cleanPath(QDir::homePath() + u'/' + fallbackUnderHome);
    }
}

QString XdgDataHome()
{
    return AbsoluteEnvironmentDir("XDG_DATA_HOME", QStringLiteral(".local/share"));
}

QString XdgConfigHome()
{
    return AbsoluteEnvironmentDir("XDG_CONFIG_HOME", QStringLiteral(".config"));
}

QString ClientInstallDir()
{
    return XdgDataHome() + u'/' + QLatin1String(kClientInstallDirName);
}
