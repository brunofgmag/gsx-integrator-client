#include "ProbeWatchList.h"

#include <sstream>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include "ProbeLog.h"

namespace
{
    std::string Trim(const std::string& value)
    {
        const std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return {};
        }

        return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    }

    bool ReadEntry(const std::string& line, probe::WatchedVariable& entry)
    {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed.front() == '#')
        {
            return false;
        }

        std::string body = trimmed;
        entry.kind = probe::WatchKind::LVar;

        if (trimmed.size() >= 2 && trimmed[1] == ':')
        {
            switch (trimmed.front())
            {
            case 'L':
                entry.kind = probe::WatchKind::LVar;
                break;
            case 'A':
                entry.kind = probe::WatchKind::AVar;
                break;
            case 'D':
                entry.kind = probe::WatchKind::Dataref;
                break;
            default:
                return false;
            }

            body = Trim(trimmed.substr(2));
        }

        if (entry.kind == probe::WatchKind::AVar)
        {
            const std::size_t separator = body.find('|');
            if (separator == std::string::npos)
            {
                return false;
            }

            entry.unit = Trim(body.substr(separator + 1));
            body = Trim(body.substr(0, separator));

            if (entry.unit.empty())
            {
                return false;
            }
        }

        entry.name = body;

        return !entry.name.empty();
    }

    std::string ReadWatchFile()
    {
        QString path = qEnvironmentVariable("GSXI_PROBE_WATCH");
        if (path.isEmpty())
        {
            path = probe::Location() + QStringLiteral("/watch.txt");
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        QTextStream stream(&file);

        return stream.readAll().toStdString();
    }
}

namespace probe
{
    std::vector<WatchedVariable> ParseWatchList(const std::string& text)
    {
        std::vector<WatchedVariable> watched;
        std::istringstream lines(text);

        for (std::string line; std::getline(lines, line);)
        {
            if (WatchedVariable entry; ReadEntry(line, entry))
            {
                watched.push_back(std::move(entry));
            }
        }

        return watched;
    }

    const std::vector<WatchedVariable>& WatchList()
    {
        static const std::vector<WatchedVariable> watched = IsOn() ? ParseWatchList(ReadWatchFile())
                                                                   : std::vector<WatchedVariable>{};

        return watched;
    }
}
