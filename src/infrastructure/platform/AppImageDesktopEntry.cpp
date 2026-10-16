#include "AppImageDesktopEntry.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

namespace
{
    constexpr auto kEntryFileName = "gsx-integrator-client.desktop";
    constexpr auto kIconRelativePath = "icons/hicolor/256x256/apps/gsx-integrator-client.png";
    constexpr auto kApplicationsDirName = "applications";
    constexpr auto kHiddenMarker = "NoDisplay=true";

    QString EscapeValue(const QString& value)
    {
        QString escaped = value;
        escaped.replace(u'\\', QLatin1String("\\\\"));

        return escaped;
    }

    bool IsOurHiddenEntry(const QByteArray& content)
    {
        for (const QByteArray& line : content.split('\n'))
        {
            if (line.trimmed() == kHiddenMarker)
            {
                return true;
            }
        }

        return false;
    }

    bool PathIsTaken(const QString& path)
    {
        const QFileInfo info(path);

        return info.exists() || info.isSymLink();
    }

    bool BelongsToSomeoneElse(const QString& entryPath)
    {
        if (!PathIsTaken(entryPath))
        {
            return false;
        }

        QFile file(entryPath);

        return QFileInfo(entryPath).isSymLink() || !file.open(QIODevice::ReadOnly) || !IsOurHiddenEntry(file.readAll());
    }

    bool WriteAtomically(const QString& path, const QByteArray& bytes)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        {
            return false;
        }

        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            return false;
        }
        if (file.write(bytes) != bytes.size())
        {
            file.cancelWriting();

            return false;
        }

        return file.commit();
    }

    bool HoldsExactly(const QString& path, const QByteArray& bytes)
    {
        QFile file(path);

        return file.open(QIODevice::ReadOnly) && file.readAll() == bytes;
    }

    bool WriteUnlessIdentical(const QString& path, const QByteArray& bytes)
    {
        return HoldsExactly(path, bytes) || WriteAtomically(path, bytes);
    }
}

QString QuoteDesktopExec(const QString& path)
{
    QString escaped;
    escaped.reserve(path.size() + 2);
    for (const QChar c : path)
    {
        if (c == u'"' || c == u'`' || c == u'$' || c == u'\\')
        {
            escaped.append(u'\\');
        }
        escaped.append(c);
    }

    escaped.replace(u'\\', QLatin1String("\\\\"));
    escaped.replace(u'%', QLatin1String("%%"));

    return u'"' + escaped + u'"';
}

QString BuildAppImageDesktopEntry(const QString& appImagePath, const QString& iconPath)
{
    QString text = QStringLiteral("[Desktop Entry]\nType=Application\n");
    text += QStringLiteral("Name=GSX Integrator\n");
    text += QStringLiteral("Exec=") + QuoteDesktopExec(appImagePath) + u'\n';
    text += QStringLiteral("TryExec=") + EscapeValue(appImagePath) + u'\n';
    text += QStringLiteral("Icon=") + EscapeValue(iconPath) + u'\n';
    text += QStringLiteral("Terminal=false\n");
    text += QStringLiteral("StartupWMClass=gsx-integrator-client\n");
    text += QStringLiteral("NoDisplay=true\n");

    return text;
}

QString AppImageDesktopEntryPath(const QString& dataHome)
{
    return dataHome + u'/' + QLatin1String(kApplicationsDirName) + u'/' + QLatin1String(kEntryFileName);
}

QString AppImageDesktopIconPath(const QString& dataHome)
{
    return dataHome + u'/' + QLatin1String(kIconRelativePath);
}

DesktopEntryOutcome EnsureAppImageDesktopEntry(const QString& appImagePath,
                                               const QString& dataHome,
                                               const QByteArray& iconBytes)
{
    const QString entryPath = AppImageDesktopEntryPath(dataHome);
    if (BelongsToSomeoneElse(entryPath))
    {
        return DesktopEntryOutcome::LeftAlone;
    }

    const QString iconPath = AppImageDesktopIconPath(dataHome);
    if (!WriteUnlessIdentical(iconPath, iconBytes))
    {
        return DesktopEntryOutcome::Failed;
    }

    const QByteArray entry = BuildAppImageDesktopEntry(appImagePath, iconPath).toUtf8();
    if (HoldsExactly(entryPath, entry))
    {
        return DesktopEntryOutcome::UpToDate;
    }

    return WriteAtomically(entryPath, entry) ? DesktopEntryOutcome::Written : DesktopEntryOutcome::Failed;
}
