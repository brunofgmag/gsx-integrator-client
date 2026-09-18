#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H

#include <map>
#include <memory>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>

namespace probe
{
    inline constexpr qint64 kRunBudgetBytes = 32LL * 1024 * 1024;
    inline constexpr int kKeepRuns = 5;
    inline constexpr auto kNoSession = "NoSession";
    inline constexpr auto kUnknownAircraft = "unknown";
    inline constexpr auto kCapMessage = "log capped at 32 MiB, nothing else will be written this run";

    enum class Channel
    {
        Client,
        Turnaround,
        GsxMenu,
        Wire,
        GsxLVars,
        SimAVars,
        Writes,
        AircraftLVars,
        AircraftAVars,
        AircraftVendor
    };

    namespace detail
    {
        inline bool& Gate()
        {
            static bool gate = false;

            return gate;
        }
    }

    inline bool IsOn()
    {
#ifdef NDEBUG
        return false;
#else
        return detail::Gate();
#endif
    }

    inline void SetEnabled(const bool enabled)
    {
#ifdef NDEBUG
        Q_UNUSED(enabled)
#else
        detail::Gate() = enabled;
#endif
    }

    inline bool ActsOnTheSim()
    {
#ifdef NDEBUG
        return false;
#else
        static const bool acting = qEnvironmentVariableIsSet("GSXI_PROBE");

        return acting;
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

                return base;
            }();

            return directory;
        }

        inline QString Stamp()
        {
            static const QString stamp =
                QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"))
                + QStringLiteral("-") + QString::number(QCoreApplication::applicationPid());

            return stamp;
        }

        inline QStringList StaleRuns(const QStringList& sortedNames, const int keep)
        {
            QStringList stale;

            for (int i = 0; i + keep < sortedNames.size(); ++i)
            {
                stale.append(sortedNames.at(i));
            }

            return stale;
        }

        inline QString RunDirectory()
        {
            static const QString directory = []
            {
                const QString base = Directory();
                const QStringList names =
                    QDir(base).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                const QStringList stale = StaleRuns(names, kKeepRuns - 1);
                for (const QString& name : stale)
                {
                    (void)QDir(base + QStringLiteral("/") + name).removeRecursively();
                }

                const QString run = base + QStringLiteral("/") + Stamp();
                if (!QDir().mkpath(run))
                {
                    return base;
                }

                return run;
            }();

            return directory;
        }

        inline QString& Phase()
        {
            static QString phase = QLatin1String(kNoSession);

            return phase;
        }

        inline QString& Aircraft()
        {
            static QString aircraft;

            return aircraft;
        }

        inline qint64& Budget()
        {
            static qint64 budget = 0;

            return budget;
        }

        inline qint64& RunBudget()
        {
            static qint64 budget = kRunBudgetBytes;

            return budget;
        }

        inline bool& Capped()
        {
            static bool capped = false;

            return capped;
        }

        inline QMutex& Mutex()
        {
            static QMutex mutex;

            return mutex;
        }

        inline std::map<QString, std::unique_ptr<QFile>>& Files()
        {
            static std::map<QString, std::unique_ptr<QFile>> files;

            return files;
        }

        inline QFile& FileAt(const QString& path)
        {
            auto& files = Files();
            const auto found = files.find(path);
            if (found != files.end())
            {
                return *found->second;
            }

            (void)QDir().mkpath(QFileInfo(path).absolutePath());
            auto file = std::make_unique<QFile>(path);
            file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
            const auto inserted = files.emplace(path, std::move(file));

            return *inserted.first->second;
        }

        inline QString AircraftFile(const QString& suffix)
        {
            const QString id = Aircraft().isEmpty() ? QLatin1String(kUnknownAircraft) : Aircraft();

            return QStringLiteral("aircraft/") + id + suffix;
        }

        inline QString ChannelFileName(const Channel channel)
        {
            switch (channel)
            {
                case Channel::Client:
                    return QStringLiteral("client.log");
                case Channel::Turnaround:
                    return QStringLiteral("turnaround.log");
                case Channel::GsxMenu:
                    return QStringLiteral("gsx-menu.log");
                case Channel::Wire:
                    return QStringLiteral("wire-") + Stamp() + QStringLiteral(".jsonl");
                case Channel::GsxLVars:
                    return QStringLiteral("gsx-lvars.log");
                case Channel::SimAVars:
                    return QStringLiteral("sim-avars.log");
                case Channel::Writes:
                    return QStringLiteral("writes.log");
                case Channel::AircraftLVars:
                    return AircraftFile(QStringLiteral("-lvars.log"));
                case Channel::AircraftAVars:
                    return AircraftFile(QStringLiteral("-avars.log"));
                case Channel::AircraftVendor:
                    return AircraftFile(QStringLiteral("-vendor.log"));
            }

            return {};
        }

        inline QString UnionFileName()
        {
            return QStringLiteral("session-") + Stamp() + QStringLiteral(".log");
        }

        inline QString FormatLine(const QString& text)
        {
            return QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                + QStringLiteral(" [") + Phase() + QStringLiteral("] ") + text;
        }

        inline qint64 Write(QFile& file, const QString& text)
        {
            if (!file.isOpen())
            {
                return 0;
            }

            const qint64 written = file.write(text.toUtf8());
            file.flush();

            return written;
        }

        inline void WriteLine(QFile& file, const QString& text)
        {
            Write(file, text + QLatin1Char('\n'));
        }

#ifndef NDEBUG
        inline void SetBudgetForTest(const qint64 bytes)
        {
            RunBudget() = bytes;
        }
#endif
    }

    inline void SetPhase(const char* phase)
    {
        QMutexLocker locker(&detail::Mutex());
        detail::Phase() = QString::fromUtf8(phase);
    }

    inline void SetAircraft(const QString& id)
    {
        QMutexLocker locker(&detail::Mutex());
        detail::Aircraft() = id;
    }

    inline void Append(const Channel channel, const QString& text)
    {
        if (!IsOn())
        {
            return;
        }

        QMutexLocker locker(&detail::Mutex());

        if (detail::Capped())
        {
            return;
        }

        const QString directory = detail::RunDirectory();
        QFile& channelFile =
            detail::FileAt(directory + QLatin1Char('/') + detail::ChannelFileName(channel));
        QFile& unionFile = detail::FileAt(directory + QLatin1Char('/') + detail::UnionFileName());
        const QString line = detail::FormatLine(text) + QLatin1Char('\n');

        detail::Budget() += detail::Write(channelFile, line);
        detail::Budget() += detail::Write(unionFile, line);

        if (detail::Budget() > detail::RunBudget())
        {
            const QString cap = detail::FormatLine(QLatin1String(kCapMessage));
            detail::WriteLine(channelFile, cap);
            detail::WriteLine(unionFile, cap);
            detail::Capped() = true;
        }
    }

    inline QString RunLocation()
    {
        if (!IsOn())
        {
            return {};
        }

        return detail::RunDirectory();
    }

#ifndef NDEBUG
    inline void ResetForTest()
    {
        QMutexLocker locker(&detail::Mutex());

        detail::Files().clear();
        detail::Phase() = QLatin1String(kNoSession);
        detail::Aircraft().clear();
        detail::Budget() = 0;
        detail::RunBudget() = kRunBudgetBytes;
        detail::Capped() = false;
    }
#endif
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H
