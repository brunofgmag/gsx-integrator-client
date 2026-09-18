#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H

#include <string>
#include <unordered_map>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include "ProbeChannels.h"

namespace probe
{
    inline constexpr int kMinSecondsBetweenChanges = 2;
    inline constexpr int kElideOver = 200;

    namespace detail
    {
        struct ChangeMemo
        {
            QString signature;
            qint64 at = 0;
        };

        inline std::unordered_map<std::string, ChangeMemo>& ChangeMemos()
        {
            static std::unordered_map<std::string, ChangeMemo> memos;

            return memos;
        }

        inline QMutex& ChangeMutex()
        {
            static QMutex mutex;

            return mutex;
        }

        struct WireMemo
        {
            QString payload;
            int skipped = 0;
            bool seen = false;
        };

        inline std::unordered_map<std::string, WireMemo>& WireMemos()
        {
            static std::unordered_map<std::string, WireMemo> memos;

            return memos;
        }

        inline QString Elide(const QString& text)
        {
            QString out;
            out.reserve(text.size());

            for (int i = 0; i < text.size(); ++i)
            {
                const QChar character = text.at(i);
                if (character != QLatin1Char('"'))
                {
                    out.append(character);

                    continue;
                }

                int end = i + 1;
                while (end < text.size() && text.at(end) != QLatin1Char('"'))
                {
                    end += text.at(end) == QLatin1Char('\\') ? 2 : 1;
                }

                const int length = end - i - 1;
                if (length > kElideOver)
                {
                    out.append(QStringLiteral("\"<elided %1 bytes>\"").arg(length));
                }
                else
                {
                    out.append(text.mid(i, end - i + 1));
                }

                i = end;
            }

            return out;
        }

        inline QString JsonQuoted(const QString& value)
        {
            const QJsonDocument document(QJsonArray{value});
            const QString encoded = QString::fromUtf8(document.toJson(QJsonDocument::Compact));

            return encoded.mid(1, encoded.size() - 2);
        }

        inline QString ElidedReference(const QString& path, const int count)
        {
            return QStringLiteral("{\"type\":\"elided\",\"path\":") + JsonQuoted(path)
                + QStringLiteral(",\"count\":") + QString::number(count) + QLatin1Char('}');
        }
    }

    inline QString Location()
    {
        return detail::Directory();
    }

    inline void Line(const Channel channel, const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        Append(channel, text);
    }

    inline void Change(const Channel channel, const std::string& key, const QString& signature, const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        const std::string memoKey =
            std::to_string(static_cast<int>(channel)) + '\x1f' + key;
        const bool shouldWrite = [&]
        {
            QMutexLocker locker(&detail::ChangeMutex());
            detail::ChangeMemo& previous = detail::ChangeMemos()[memoKey];
            if (previous.signature == signature || now - previous.at < kMinSecondsBetweenChanges)
            {
                return false;
            }

            previous.signature = signature;
            previous.at = now;

            return true;
        }();

        if (shouldWrite)
        {
            Append(channel, text);
        }
    }

    inline void Change(const Channel channel, const std::string& key, const QString& text)
    {
        Change(channel, key, text, text);
    }

#ifndef NDEBUG
    inline void ResetChangeMemoForTest()
    {
        QMutexLocker locker(&detail::ChangeMutex());

        detail::ChangeMemos().clear();
    }

    inline void ResetWireMemoForTest()
    {
        QMutexLocker locker(&detail::Mutex());

        detail::WireMemos().clear();
    }
#endif

    inline void Wire(const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        const QString elided = detail::Elide(text);
        const QJsonDocument document = QJsonDocument::fromJson(elided.toUtf8());
        if (!document.isObject())
        {
            Append(Channel::Wire, elided);

            return;
        }

        const QJsonObject object = document.object();
        const QJsonValue type = object.value(QStringLiteral("type"));
        const QJsonValue path = object.value(QStringLiteral("path"));
        if (type.toString() != QStringLiteral("patch") || !path.isString())
        {
            Append(Channel::Wire, elided);

            return;
        }

        const QString pathText = path.toString();
        const std::string memoKey = pathText.toStdString();
        QString reference;
        const bool record = [&]
        {
            QMutexLocker locker(&detail::Mutex());
            detail::WireMemo& memo = detail::WireMemos()[memoKey];
            if (memo.seen && memo.payload == elided)
            {
                ++memo.skipped;

                return false;
            }

            if (memo.seen && memo.skipped > 0)
            {
                reference = detail::ElidedReference(pathText, memo.skipped);
            }

            memo.payload = elided;
            memo.seen = true;
            memo.skipped = 0;

            return true;
        }();

        if (!record)
        {
            return;
        }

        if (!reference.isEmpty())
        {
            Append(Channel::Wire, reference);
        }

        Append(Channel::Wire, elided);
    }

    inline void Sink(const QString& message)
    {
        if (!IsOn())
        {
            return;
        }

        const Channel channel = message.contains(QLatin1String("[GSX Integrator] RemoteAPI "))
            ? Channel::GsxMenu
            : Channel::Client;

        Append(channel, QStringLiteral("qt   ") + message);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
