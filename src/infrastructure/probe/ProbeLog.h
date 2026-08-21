#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H

#include <string>
#include <unordered_map>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtLogging>

namespace probe
{
    inline constexpr int kRepeatSeconds = 60;
    inline constexpr int kElideOver = 200;

    inline bool IsOn()
    {
#ifdef NDEBUG
        return false;
#else
        static const bool on = qEnvironmentVariableIsSet("GSXI_PROBE");

        return on;
#endif
    }

    namespace detail
    {
        inline QString Directory()
        {
            static const QString directory = []
            {
                QString base = qEnvironmentVariable("GSXI_PROBE_DIR");
                if (base.isEmpty())
                {
                    base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                        + QStringLiteral("/probe");
                }
                QDir().mkpath(base);

                return base;
            }();

            return directory;
        }

        inline QString Stamp()
        {
            static const QString stamp =
                QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));

            return stamp;
        }

        inline void Append(QFile& file, const QString& line)
        {
            if (!file.isOpen())
            {
                return;
            }

            QTextStream stream(&file);
            stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << ' ' << line << '\n';
            stream.flush();
        }

        inline QFile& SessionFile()
        {
            static QFile file(Directory() + QStringLiteral("/session-") + Stamp() + QStringLiteral(".log"));
            static const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
            Q_UNUSED(opened)

            return file;
        }

        inline QFile& WireFile()
        {
            static QFile file(Directory() + QStringLiteral("/wire-") + Stamp() + QStringLiteral(".jsonl"));
            static const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
            Q_UNUSED(opened)

            return file;
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
    }

    inline QString Location()
    {
        return detail::Directory();
    }

    inline void Line(const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        detail::Append(detail::SessionFile(), text);
    }

    inline void Change(const std::string& key, const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        struct Memo
        {
            QString text;
            qint64 at = 0;
        };

        static std::unordered_map<std::string, Memo> memo;

        const qint64 now = QDateTime::currentSecsSinceEpoch();
        Memo& previous = memo[key];
        if (previous.text == text && now - previous.at < kRepeatSeconds)
        {
            return;
        }

        previous.text = text;
        previous.at = now;
        detail::Append(detail::SessionFile(), text);
    }

    inline void Wire(const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        detail::Append(detail::WireFile(), detail::Elide(text));
    }

    inline void Sink(const QString& message)
    {
        if (!IsOn())
        {
            return;
        }

        detail::Append(detail::SessionFile(), QStringLiteral("qt   ") + message);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBELOG_H
