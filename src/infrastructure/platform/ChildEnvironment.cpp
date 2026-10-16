#include "ChildEnvironment.h"

#include <QtCore/QDir>
#include <QtCore/QStringList>

namespace
{
    constexpr const char* kAppImageVariables[] = {"APPIMAGE", "APPDIR", "ARGV0", "OWD"};
    constexpr const char* kPathVariables[] = {
        "LD_LIBRARY_PATH", "PATH", "QT_PLUGIN_PATH", "QML_IMPORT_PATH", "QML2_IMPORT_PATH",
    };

    bool IsInside(const QString& entry, const QString& appDir)
    {
        const QString cleaned = QDir::cleanPath(entry);
        return cleaned == appDir || cleaned.startsWith(appDir + u'/');
    }
}

QProcessEnvironment ChildProcessEnvironment(const QProcessEnvironment& environment)
{
    QProcessEnvironment result = environment;
    const QString appDir = QDir::cleanPath(environment.value(QStringLiteral("APPDIR")));

    if (!appDir.isEmpty() && appDir != QLatin1String("."))
    {
        for (const char* variable : kPathVariables)
        {
            const QString key = QLatin1String(variable);
            if (!result.contains(key))
            {
                continue;
            }

            QStringList kept;
            for (const QString& entry : result.value(key).split(u':', Qt::SkipEmptyParts))
            {
                if (!IsInside(entry, appDir))
                {
                    kept.append(entry);
                }
            }

            if (kept.isEmpty())
            {
                result.remove(key);
            }
            else
            {
                result.insert(key, kept.join(u':'));
            }
        }
    }

    for (const char* variable : kAppImageVariables)
    {
        result.remove(QLatin1String(variable));
    }

    return result;
}
