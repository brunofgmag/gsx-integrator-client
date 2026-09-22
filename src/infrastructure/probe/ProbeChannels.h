#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>

namespace probe
{
    inline constexpr qint64 kMiB = 1024LL * 1024;
    inline constexpr qint64 kWireBudgetBytes = 32 * kMiB;
    inline constexpr qint64 kUnionBudgetBytes = 32 * kMiB;
    inline constexpr qint64 kChannelBudgetBytes = 8 * kMiB;
    inline constexpr int kKeepRuns = 5;
    inline constexpr auto kNoSession = "NoSession";
    inline constexpr auto kUnknownAircraft = "unknown";
    inline constexpr auto kCapMessage = "log file capped at %1 MiB, nothing else will be written to it this run";
    inline constexpr auto kCapNote = "%1 capped at %2 MiB, nothing else will be written to it this run";
    inline constexpr auto kRunStampPattern = R"(^\d{8}-\d{6}(?:-\d{3}-\d+)?$)";
    inline constexpr auto kWirePrefix = "wire-";
    inline constexpr auto kWireSuffix = ".jsonl";

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
        struct Budgets
        {
            qint64 wireBytes = kWireBudgetBytes;
            qint64 unionBytes = kUnionBudgetBytes;
            qint64 channelBytes = kChannelBudgetBytes;
        };

        struct LogFile
        {
            std::unique_ptr<QFile> file;
            qint64 written = 0;
            bool capped = false;
        };

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

        inline QString AircraftFile(const QString& suffix)
        {
            const QString aircraftId = Aircraft().isEmpty() ? QLatin1String(kUnknownAircraft) : Aircraft();

            return QStringLiteral("aircraft/") + aircraftId + suffix;
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
                    return QLatin1String(kWirePrefix) + Stamp() + QLatin1String(kWireSuffix);
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

        inline bool IsRunStamp(const QString& name)
        {
            static const QRegularExpression stamp(QString::fromLatin1(kRunStampPattern));

            return stamp.match(name).hasMatch();
        }

        inline bool SawTheSimulator(const QString& run)
        {
            const QDir directory(run);
            const QString wireFiles = QLatin1String(kWirePrefix) + QLatin1Char('*') + QLatin1String(kWireSuffix);

            return directory.exists(ChannelFileName(Channel::Turnaround))
                || !directory.entryList(QStringList{wireFiles}, QDir::Files).isEmpty();
        }

        inline QStringList StaleRuns(const QStringList& names, const QSet<QString>& sawTheSimulator, const int keep)
        {
            QStringList runs;
            std::ranges::copy_if(names, std::back_inserter(runs), IsRunStamp);
            runs.sort();

            QStringList simulatorRuns;
            QStringList stale;
            std::ranges::partition_copy(runs, std::back_inserter(simulatorRuns), std::back_inserter(stale),
                                        [&sawTheSimulator](const QString& run)
                                        {
                                            return sawTheSimulator.contains(run);
                                        });

            const qsizetype surplus = std::max<qsizetype>(0, simulatorRuns.size() - keep);
            stale.append(simulatorRuns.first(surplus));
            stale.sort();

            return stale;
        }

        inline QString RunDirectory()
        {
            static const QString directory = []
            {
                const QString base = Directory();
                const QStringList names =
                    QDir(base).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                QSet<QString> sawTheSimulator;
                for (const QString& name : names)
                {
                    if (SawTheSimulator(base + QLatin1Char('/') + name))
                    {
                        sawTheSimulator.insert(name);
                    }
                }

                const QStringList stale = StaleRuns(names, sawTheSimulator, kKeepRuns - 1);
                for (const QString& name : stale)
                {
                    (void)QDir(base + QLatin1Char('/') + name).removeRecursively();
                }

                const QString run = base + QLatin1Char('/') + Stamp();
                if (!QDir().mkpath(run))
                {
                    return base;
                }

                return run;
            }();

            return directory;
        }

        inline Budgets& FileBudgets()
        {
            static Budgets budgets;

            return budgets;
        }

        inline QMutex& Mutex()
        {
            static QMutex mutex;

            return mutex;
        }

        inline std::map<QString, LogFile>& Files()
        {
            static std::map<QString, LogFile> files;

            return files;
        }

        inline LogFile& FileAt(const QString& path)
        {
            auto& files = Files();
            const auto found = files.find(path);
            if (found != files.end())
            {
                return found->second;
            }

            (void)QDir().mkpath(QFileInfo(path).absolutePath());
            auto file = std::make_unique<QFile>(path);
            file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
            const auto inserted = files.emplace(path, LogFile{.file = std::move(file)});

            return inserted.first->second;
        }

        inline QString FormatLine(const QString& text)
        {
            return QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                + QStringLiteral(" [") + Phase() + QStringLiteral("] ") + text;
        }

        inline QString CapMessage(const qint64 budget)
        {
            return QString::fromLatin1(kCapMessage).arg(budget / kMiB);
        }

        inline QString CapNote(const QString& name, const qint64 budget)
        {
            return QString::fromLatin1(kCapNote).arg(name, QString::number(budget / kMiB));
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

        inline bool Record(const QString& name, const qint64 budget, const QString& line)
        {
            LogFile& log = FileAt(RunDirectory() + QLatin1Char('/') + name);
            if (log.capped)
            {
                return false;
            }

            log.written += Write(*log.file, line);
            if (log.written <= budget)
            {
                return false;
            }

            WriteLine(*log.file, FormatLine(CapMessage(budget)));
            log.capped = true;

            return true;
        }

#ifndef NDEBUG
        inline void SetBudgetForTest(const Budgets& budgets)
        {
            FileBudgets() = budgets;
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

        const detail::Budgets& budgets = detail::FileBudgets();
        const QString unionName = detail::UnionFileName();
        const QString line = detail::FormatLine(text) + QLatin1Char('\n');
        if (channel != Channel::Wire)
        {
            detail::Record(unionName, budgets.unionBytes, line);
        }

        const QString channelName = detail::ChannelFileName(channel);
        const qint64 channelBudget = channel == Channel::Wire ? budgets.wireBytes : budgets.channelBytes;
        const bool channelCapped = detail::Record(channelName, channelBudget, line);
        if (channelCapped)
        {
            detail::Record(unionName, budgets.unionBytes,
                           detail::FormatLine(detail::CapNote(channelName, channelBudget)) + QLatin1Char('\n'));
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
        detail::FileBudgets() = {};
    }
#endif
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROBECHANNELS_H
