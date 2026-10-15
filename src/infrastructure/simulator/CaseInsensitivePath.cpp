#include "CaseInsensitivePath.h"

#include <algorithm>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

namespace
{
    QString Join(const QString& directory, const QString& name)
    {
        return directory.endsWith(u'/') ? directory + name : directory + u'/' + name;
    }

    bool Exists(const QString& path)
    {
        const QFileInfo entry(path);

        return entry.exists() || entry.isSymLink();
    }

    QString CaseInsensitiveMatch(const QString& directory, const QString& name)
    {
        const QStringList names = QDir(directory).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        const auto match = std::ranges::find_if(names, [&name](const QString& candidate)
        {
            return candidate.compare(name, Qt::CaseInsensitive) == 0;
        });

        return match == names.end() ? QString() : *match;
    }
}

QString ResolveCaseInsensitive(const QString& baseDir, const QString& relativePath)
{
    const QStringList parts = relativePath.split(u'/', Qt::SkipEmptyParts);

    QString resolved = baseDir;
    for (qsizetype index = 0; index < parts.size(); ++index)
    {
        const QString actual = Exists(Join(resolved, parts[index]))
                                   ? parts[index]
                                   : CaseInsensitiveMatch(resolved, parts[index]);
        if (actual.isEmpty())
        {
            return Join(resolved, parts.mid(index).join(u'/'));
        }

        resolved = Join(resolved, actual);
    }

    return resolved;
}
