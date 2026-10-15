#include "SteamLibraries.h"

#include <vector>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringView>

namespace
{
    constexpr auto kLibraryFoldersFile = "libraryfolders.vdf";
    constexpr auto kPathKey = "path";
    constexpr auto kSteamappsDirName = "steamapps";
    constexpr int kLibraryEntryDepth = 2;

    enum class TokenKind
    {
        Text,
        Open,
        Close
    };

    struct Token
    {
        TokenKind kind;
        QString text;
    };

    QChar Unescaped(const QChar letter)
    {
        if (letter == u'n')
        {
            return u'\n';
        }

        return letter == u't' ? u'\t' : letter;
    }

    qsizetype ReadQuoted(const QString& text, const qsizetype opening, QString* value)
    {
        qsizetype index = opening + 1;
        while (index < text.size() && text[index] != u'"')
        {
            if (text[index] == u'\\' && index + 1 < text.size())
            {
                ++index;
                value->append(Unescaped(text[index]));
            }
            else
            {
                value->append(text[index]);
            }

            ++index;
        }

        return index + 1;
    }

    qsizetype SkipLine(const QString& text, const qsizetype from)
    {
        const qsizetype end = text.indexOf(u'\n', from);

        return end < 0 ? text.size() : end + 1;
    }

    std::vector<Token> Tokenize(const QString& text)
    {
        std::vector<Token> tokens;
        qsizetype index = 0;
        while (index < text.size())
        {
            const QChar letter = text[index];
            if (letter == u'"')
            {
                QString value;
                index = ReadQuoted(text, index, &value);
                tokens.push_back({TokenKind::Text, value});
            }
            else if (letter == u'{' || letter == u'}')
            {
                tokens.push_back({letter == u'{' ? TokenKind::Open : TokenKind::Close, QString()});
                ++index;
            }
            else if (QStringView(text).mid(index).startsWith(u"//"))
            {
                index = SkipLine(text, index);
            }
            else
            {
                ++index;
            }
        }

        return tokens;
    }

    QByteArray ReadFileIfExists(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return {};
        }

        return file.readAll();
    }

    void AppendExistingDir(QStringList* dirs, const QString& dir)
    {
        const QString canonical = QFileInfo(dir).canonicalFilePath();
        if (!canonical.isEmpty() && !dirs->contains(canonical))
        {
            dirs->append(canonical);
        }
    }
}

QStringList ParseSteamLibraryPaths(const QByteArray& libraryFoldersVdf)
{
    QStringList paths;
    QString key;
    int depth = 0;
    for (const Token& token : Tokenize(QString::fromUtf8(libraryFoldersVdf)))
    {
        if (token.kind == TokenKind::Open)
        {
            ++depth;
            key.clear();
        }
        else if (token.kind == TokenKind::Close)
        {
            --depth;
        }
        else if (key.isEmpty())
        {
            key = token.text;
        }
        else
        {
            if (depth == kLibraryEntryDepth && key.compare(QLatin1String(kPathKey), Qt::CaseInsensitive) == 0)
            {
                paths.append(token.text);
            }

            key.clear();
        }
    }

    return paths;
}

QStringList DefaultSteamappsDirs(const QString& machineHome)
{
    return {
        machineHome + QStringLiteral("/.local/share/Steam/") + QLatin1String(kSteamappsDirName),
        machineHome + QStringLiteral("/.steam/steam/") + QLatin1String(kSteamappsDirName)
    };
}

QStringList SteamLibrarySteamappsDirs(const QStringList& steamappsDirs)
{
    QStringList libraries;
    for (const QString& steamappsDir : steamappsDirs)
    {
        AppendExistingDir(&libraries, steamappsDir);

        for (const QString& path : ParseSteamLibraryPaths(
                 ReadFileIfExists(steamappsDir + u'/' + QLatin1String(kLibraryFoldersFile))))
        {
            AppendExistingDir(&libraries, path + u'/' + QLatin1String(kSteamappsDirName));
        }
    }

    return libraries;
}
