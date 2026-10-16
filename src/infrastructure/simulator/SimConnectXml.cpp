#include "SimConnectXml.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include "CaseInsensitivePath.h"
#include "SteamLibraries.h"

namespace
{
    constexpr auto kFileName = "SimConnect.xml";
    constexpr auto kClosingTag = "</SimBase.Document>";
    constexpr auto kDefaultIndent = "\t";
    constexpr auto kDefaultEol = "\r\n";
    constexpr int kFirstUnprivilegedPort = 1024;
    constexpr int kLastPort = 65535;

    QString PatternFor(const char* tag)
    {
        return QStringLiteral("<%1>\\s*(.*?)\\s*</%1>").arg(QLatin1String(tag));
    }

    QString WithoutComments(const QString& text)
    {
        static const QRegularExpression kComment(
            QStringLiteral("<!--.*?-->"), QRegularExpression::DotMatchesEverythingOption);

        QString stripped = text;

        return stripped.replace(kComment, QString());
    }

    QString TagValue(const QString& block, const char* tag)
    {
        const QRegularExpression pattern(PatternFor(tag), QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpressionMatch match = pattern.match(block);

        return match.hasMatch() ? match.captured(1) : QString();
    }

    std::optional<quint16> PortOfEntry(const QString& entry)
    {
        const bool disabled = TagValue(entry, "Disabled").compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
        if (disabled || TagValue(entry, "Protocol").compare(QStringLiteral("IPv4"), Qt::CaseInsensitive) != 0)
        {
            return std::nullopt;
        }

        bool numeric = false;
        const int port = TagValue(entry, "Port").toInt(&numeric);
        if (!numeric || port < kFirstUnprivilegedPort || port > kLastPort)
        {
            return std::nullopt;
        }

        return static_cast<quint16>(port);
    }

    QByteArray LineEndingOf(const QByteArray& content)
    {
        const qsizetype newline = content.indexOf('\n');
        if (newline < 0)
        {
            return QByteArray(kDefaultEol);
        }

        return newline > 0 && content[newline - 1] == '\r' ? QByteArray("\r\n") : QByteArray("\n");
    }

    QByteArray IndentOf(const QByteArray& content)
    {
        const qsizetype tag = content.indexOf("<SimConnect.Comm>");
        if (tag < 0)
        {
            return QByteArray(kDefaultIndent);
        }

        const qsizetype lineStart = content.lastIndexOf('\n', tag) + 1;
        const QByteArray lead = content.mid(lineStart, tag - lineStart);

        return lead.trimmed().isEmpty() && !lead.isEmpty() ? lead : QByteArray(kDefaultIndent);
    }

    QByteArray EntryFor(const QByteArray& indent, const QByteArray& eol)
    {
        const QByteArray inner = indent + indent;

        return indent + "<SimConnect.Comm>" + eol
            + inner + "<Descr>Static IP4 port</Descr>" + eol
            + inner + "<Protocol>IPv4</Protocol>" + eol
            + inner + "<Scope>local</Scope>" + eol
            + inner + "<MaxClients>64</MaxClients>" + eol
            + inner + "<Port>" + QByteArray::number(kStaticSimConnectPort) + "</Port>" + eol
            + inner + "<MaxRecvSize>41088</MaxRecvSize>" + eol
            + indent + "</SimConnect.Comm>" + eol;
    }

    QByteArray NewDocument()
    {
        const QByteArray eol(kDefaultEol);
        const QByteArray indent(kDefaultIndent);

        return "<?xml version=\"1.0\" encoding=\"Windows-1252\"?>" + eol
            + eol
            + "<SimBase.Document Type=\"SimConnect\" version=\"1,0\">" + eol
            + indent + "<Descr>SimConnect</Descr>" + eol
            + indent + "<Filename>" + kFileName + "</Filename>" + eol
            + EntryFor(indent, eol)
            + kClosingTag + eol;
    }

    std::optional<QByteArray> WithEntryAdded(const QByteArray& content)
    {
        const qsizetype closing = content.lastIndexOf(kClosingTag);
        if (closing < 0)
        {
            return std::nullopt;
        }

        const QByteArray eol = LineEndingOf(content);
        const QByteArray entry = EntryFor(IndentOf(content), eol);
        const qsizetype lineStart = content.lastIndexOf('\n', closing) + 1;
        const bool closingStartsItsLine = content.mid(lineStart, closing - lineStart).trimmed().isEmpty();

        QByteArray result = content;
        if (closingStartsItsLine)
        {
            return result.insert(lineStart, entry);
        }

        return result.insert(closing, eol + entry);
    }

    bool WriteAll(const QString& path, const QByteArray& content)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        {
            return false;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        return file.write(content) == content.size();
    }

    SimConnectXmlResult Failed()
    {
        return {.status = SimConnectXmlStatus::Failed, .port = 0};
    }
}

std::optional<quint16> ParseSimConnectPort(const QByteArray& simConnectXml)
{
    static const QRegularExpression kEntry(
        QStringLiteral("<SimConnect\\.Comm>(.*?)</SimConnect\\.Comm>"), QRegularExpression::DotMatchesEverythingOption);

    const QString text = WithoutComments(QString::fromLatin1(simConnectXml));

    QRegularExpressionMatchIterator entries = kEntry.globalMatch(text);
    while (entries.hasNext())
    {
        const std::optional<quint16> port = PortOfEntry(entries.next().captured(1));
        if (port.has_value())
        {
            return port;
        }
    }

    return std::nullopt;
}

std::optional<quint16> ReadSimConnectPort(const QString& simConnectXmlPath)
{
    QFile file(simConnectXmlPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return std::nullopt;
    }

    return ParseSimConnectPort(file.readAll());
}

SimConnectXmlResult EnsureSimConnectPort(const QString& simConnectXmlPath,
                                         const std::function<bool()>& isSimulatorRunning)
{
    const bool exists = QFileInfo::exists(simConnectXmlPath);
    QByteArray content;
    if (exists)
    {
        QFile file(simConnectXmlPath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return Failed();
        }

        content = file.readAll();
        const std::optional<quint16> port = ParseSimConnectPort(content);
        if (port.has_value())
        {
            return {.status = SimConnectXmlStatus::Present, .port = *port};
        }
    }

    if (isSimulatorRunning())
    {
        return {.status = SimConnectXmlStatus::SimulatorRunning, .port = 0};
    }

    const std::optional<QByteArray> updated = exists ? WithEntryAdded(content) : std::optional(NewDocument());
    if (!updated.has_value() || !WriteAll(simConnectXmlPath, *updated))
    {
        return Failed();
    }

    return {
        .status = exists ? SimConnectXmlStatus::Added : SimConnectXmlStatus::Created,
        .port = kStaticSimConnectPort
    };
}

std::vector<SimConnectXmlTarget> SimConnectXmlTargets(const std::vector<ProtonPrefix>& prefixes)
{
    std::vector<SimConnectXmlTarget> targets;
    for (const ProtonPrefix& prefix : prefixes)
    {
        const QString home = ProtonKnownFoldersOf(prefix.root).home;

        targets.push_back({
            .label = QLatin1String(prefix.simulator->label),
            .path = ResolveCaseInsensitive(SimulatorBaseDir(home, *prefix.simulator), QLatin1String(kFileName)),
            .processName = QLatin1String(prefix.simulator->processName)
        });
    }

    return targets;
}

std::vector<SimConnectXmlTarget> SimConnectXmlTargets(const QString& machineHome)
{
    return SimConnectXmlTargets(FindProtonPrefixes(DefaultSteamappsDirs(machineHome)));
}
