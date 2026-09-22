#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QStringList>
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
            QJsonValue value;
            int skipped = 0;
        };

        inline std::map<QString, WireMemo>& WireMemos()
        {
            static std::map<QString, WireMemo> memos;

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

        inline QString JsonText(const QJsonValue& value)
        {
            const QJsonDocument document(QJsonArray{value});
            const QString encoded = QString::fromUtf8(document.toJson(QJsonDocument::Compact));

            return encoded.mid(1, encoded.size() - 2);
        }

        inline QString ElidedReference(const QString& path, const int count)
        {
            return QStringLiteral("{\"type\":\"elided\",\"path\":") + JsonText(path)
                + QStringLiteral(",\"count\":") + QString::number(count) + QLatin1Char('}');
        }

        inline QString PointerToken(const QString& key)
        {
            QString token = key;
            token.replace(QLatin1Char('~'), QLatin1String("~0"));
            token.replace(QLatin1Char('/'), QLatin1String("~1"));

            return token;
        }

        inline QJsonObject Operation(const QString& kind, const QString& pointer)
        {
            return QJsonObject{{QStringLiteral("op"), kind}, {QStringLiteral("path"), pointer}};
        }

        inline QJsonObject Operation(const QString& kind, const QString& pointer, const QJsonValue& value)
        {
            QJsonObject operation = Operation(kind, pointer);
            operation.insert(QStringLiteral("value"), value);

            return operation;
        }

        inline void DiffInto(QJsonArray& ops, const QString& pointer, const QJsonValue& before,
                             const QJsonValue& after);

        inline void DiffObjectsInto(QJsonArray& ops, const QString& pointer, const QJsonObject& before,
                                    const QJsonObject& after)
        {
            const QStringList beforeKeys = before.keys();
            for (const QString& key : beforeKeys)
            {
                const QString child = pointer + QLatin1Char('/') + PointerToken(key);
                if (!after.contains(key))
                {
                    ops.append(Operation(QStringLiteral("remove"), child));

                    continue;
                }

                DiffInto(ops, child, before.value(key), after.value(key));
            }

            const QStringList afterKeys = after.keys();
            for (const QString& key : afterKeys)
            {
                if (!before.contains(key))
                {
                    ops.append(Operation(QStringLiteral("add"), pointer + QLatin1Char('/') + PointerToken(key),
                                         after.value(key)));
                }
            }
        }

        inline void DiffArraysInto(QJsonArray& ops, const QString& pointer, const QJsonArray& before,
                                   const QJsonArray& after)
        {
            const qsizetype common = (std::min)(before.size(), after.size());

            for (qsizetype i = 0; i < common; ++i)
            {
                DiffInto(ops, pointer + QLatin1Char('/') + QString::number(i), before.at(i), after.at(i));
            }

            for (qsizetype i = common; i < after.size(); ++i)
            {
                ops.append(Operation(QStringLiteral("add"), pointer + QLatin1Char('/') + QString::number(i),
                                     after.at(i)));
            }

            for (qsizetype i = before.size() - 1; i >= common; --i)
            {
                ops.append(Operation(QStringLiteral("remove"), pointer + QLatin1Char('/') + QString::number(i)));
            }
        }

        inline void DiffInto(QJsonArray& ops, const QString& pointer, const QJsonValue& before,
                             const QJsonValue& after)
        {
            if (before == after)
            {
                return;
            }

            if (before.isObject() && after.isObject())
            {
                DiffObjectsInto(ops, pointer, before.toObject(), after.toObject());

                return;
            }

            if (before.isArray() && after.isArray())
            {
                DiffArraysInto(ops, pointer, before.toArray(), after.toArray());

                return;
            }

            ops.append(Operation(QStringLiteral("replace"), pointer, after));
        }

        inline QJsonArray JsonPatch(const QJsonValue& before, const QJsonValue& after)
        {
            QJsonArray ops;
            DiffInto(ops, QString(), before, after);

            return ops;
        }

        inline QString PatchDiffLine(const QJsonObject& patch, const QString& path, const QJsonArray& ops)
        {
            QString line = QStringLiteral("{\"type\":\"patch-diff\"");
            const QJsonValue timestamp = patch.value(QStringLiteral("ts"));
            if (!timestamp.isUndefined())
            {
                line += QStringLiteral(",\"ts\":") + JsonText(timestamp);
            }

            return line + QStringLiteral(",\"path\":") + JsonText(path) + QStringLiteral(",\"ops\":")
                + QString::fromUtf8(QJsonDocument(ops).toJson(QJsonDocument::Compact)) + QLatin1Char('}');
        }

        inline QString PatchRecord(const QJsonObject& patch, const QString& path, const QJsonValue& before,
                                   const QJsonValue& after, const QString& elided)
        {
            if (before.isUndefined() || after.isUndefined())
            {
                return elided;
            }

            const QString diff = PatchDiffLine(patch, path, JsonPatch(before, after));

            return diff.size() < elided.size() ? diff : elided;
        }

        inline QStringList RecordPatch(const QJsonObject& patch, const QString& path, const QString& elided)
        {
            const QJsonValue value = patch.value(QStringLiteral("value"));

            QMutexLocker locker(&Mutex());
            auto& memos = WireMemos();
            const auto found = memos.find(path);
            if (found == memos.end())
            {
                memos.emplace(path, WireMemo{.value = value});

                return {elided};
            }

            WireMemo& memo = found->second;
            if (memo.value == value)
            {
                ++memo.skipped;

                return {};
            }

            QStringList lines;
            if (memo.skipped > 0)
            {
                lines.append(ElidedReference(path, memo.skipped));
            }

            lines.append(PatchRecord(patch, path, memo.value, value, elided));
            memo.value = value;
            memo.skipped = 0;

            return lines;
        }

        inline QStringList ForgetWirePaths()
        {
            QMutexLocker locker(&Mutex());
            QStringList references;

            for (const auto& [path, memo] : WireMemos())
            {
                if (memo.skipped > 0)
                {
                    references.append(ElidedReference(path, memo.skipped));
                }
            }

            WireMemos().clear();

            return references;
        }

        inline Channel SinkChannel(const QString& message)
        {
            if (message.contains(QLatin1String("[GSX Integrator] RemoteAPI ")))
            {
                return Channel::GsxMenu;
            }

            if (message.contains(QLatin1String("Transitioning: ")))
            {
                return Channel::Turnaround;
            }

            return Channel::Client;
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
        if (type.toString() == QStringLiteral("snapshot"))
        {
            const QStringList references = detail::ForgetWirePaths();
            for (const QString& reference : references)
            {
                Append(Channel::Wire, reference);
            }

            Append(Channel::Wire, elided);

            return;
        }

        const QJsonValue path = object.value(QStringLiteral("path"));
        if (type.toString() != QStringLiteral("patch") || !path.isString())
        {
            Append(Channel::Wire, elided);

            return;
        }

        const QStringList lines = detail::RecordPatch(object, path.toString(), elided);
        for (const QString& line : lines)
        {
            Append(Channel::Wire, line);
        }
    }

    inline void WireSent(const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
        if (!document.isObject())
        {
            Append(Channel::Wire, QStringLiteral("{\"type\":\"sent\",\"raw\":") + detail::JsonText(text)
                       + QLatin1Char('}'));

            return;
        }

        Append(Channel::Wire, QStringLiteral("{\"type\":\"sent\",\"message\":")
                   + QString::fromUtf8(document.toJson(QJsonDocument::Compact)) + QLatin1Char('}'));
    }

    inline void Sink(const QString& message)
    {
        if (!IsOn())
        {
            return;
        }

        Append(detail::SinkChannel(message), QStringLiteral("qt   ") + message);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
