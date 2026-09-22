#include <QtTest/QTest>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "../src/infrastructure/probe/ProbeChannels.h"
#include "../src/infrastructure/probe/ProbeLog.h"

namespace
{
    QStringList EntriesRecursively(const QString& root)
    {
        QStringList entries;
        QDirIterator iterator(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            entries.append(QDir(root).relativeFilePath(iterator.next()));
        }
        entries.sort();

        return entries;
    }

    QString ReadAll(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        return QString::fromUtf8(file.readAll());
    }

    QString LastLine(const QString& path)
    {
        const QStringList lines = ReadAll(path).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        return lines.isEmpty() ? QString() : lines.last();
    }

    QStringList Lines(const QString& path)
    {
        return ReadAll(path).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    QString FreshClientLog()
    {
        const QString path = probe::RunLocation() + QStringLiteral("/client.log");
        QFile::remove(path);

        return path;
    }

    QString FreshChannelLog(const QString& name)
    {
        const QString path = probe::RunLocation() + QLatin1Char('/') + name;
        QFile::remove(path);

        return path;
    }

    QString FreshUnionLog()
    {
        return FreshChannelLog(probe::detail::UnionFileName());
    }

    QString FreshWireLog()
    {
        return FreshChannelLog(probe::detail::ChannelFileName(probe::Channel::Wire));
    }

    QString Line(const int bytes)
    {
        return {bytes, QLatin1Char('x')};
    }

    void Touch(const QString& path)
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
    }
}

class ProbeChannelsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    static void theFourNewestSimulatorRunsAreKept();
    static void runsThatNeverSawTheSimulatorAreDeletedAndNotCounted();
    static void directoriesThatAreNotRunStampsAreNeverDeleted();
    static void aRunSawTheSimulatorWhenItHoldsTheGateLogOrAWireLog();
    static void eachChannelGetsItsOwnFile();
    static void everyChannelButTheWireAlsoLandsInTheSessionUnion();
    static void everyLineCarriesTheTimestampAndThePhase();
    static void theAircraftChannelsFollowTheAircraftId();
    static void aRunFolderIsCreatedPerLaunch();
    static void aCappedFileWritesOneCapLineAndThenGoesSilent();
    static void aCappedChannelLeavesTheOtherFilesWriting();
    static void theUnionNamesTheFileThatCapped();
    static void theUnionNamesACappedWireFile();
    static void theUnionCapsOnItsOwnBudget();
    static void appendIsSilentWhenTheGateIsOff();
    static void aFirstChangeWritesImmediately();
    static void anUnchangedSignatureNeverWritesAgain();
    static void aChangedSignatureWritesOnTheNextTickWhenTheFloorBlocksIt();
    static void theSignatureDecidesAndTheTextIsWhatLands();
    static void aRemoteApiMessageLandsInTheMenuChannel();
    static void anyOtherMessageLandsInTheClientChannel();

private:
    QTemporaryDir directory_;
};

void ProbeChannelsTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void ProbeChannelsTest::init()
{
#ifndef NDEBUG
    probe::SetEnabled(true);
    probe::ResetForTest();
    probe::ResetChangeMemoForTest();
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::cleanup()
{
    probe::SetEnabled(true);
}

void ProbeChannelsTest::theFourNewestSimulatorRunsAreKept()
{
    const QStringList names{
        QStringLiteral("20260104-100000-003-9"),
        QStringLiteral("20260101-000002"),
        QStringLiteral("20260102-080000-001-7"),
        QStringLiteral("20260101-000001"),
        QStringLiteral("20260103-090000-002-8"),
        QStringLiteral("20260101-000002-500-100")};
    const QSet<QString> sawTheSimulator(names.cbegin(), names.cend());

    const QStringList stale = probe::detail::StaleRuns(names, sawTheSimulator, probe::kKeepRuns - 1);

    QCOMPARE(stale, (QStringList{QStringLiteral("20260101-000001"), QStringLiteral("20260101-000002")}));

    const QStringList few{QStringLiteral("20260101-000001"), QStringLiteral("20260101-000002")};

    QVERIFY(probe::detail::StaleRuns(few, QSet<QString>(few.cbegin(), few.cend()), probe::kKeepRuns - 1).isEmpty());
}

void ProbeChannelsTest::runsThatNeverSawTheSimulatorAreDeletedAndNotCounted()
{
    const QStringList simulatorRuns{
        QStringLiteral("20260901-100000-000-1"),
        QStringLiteral("20260902-100000-000-2"),
        QStringLiteral("20260903-100000-000-3"),
        QStringLiteral("20260904-100000-000-4")};
    const QStringList launchesWithoutTheSimulator{
        QStringLiteral("20260831-100000"),
        QStringLiteral("20260905-100000-000-5"),
        QStringLiteral("20260906-100000-000-6"),
        QStringLiteral("20260907-100000-000-7")};

    const QStringList stale =
        probe::detail::StaleRuns(simulatorRuns + launchesWithoutTheSimulator,
                                 QSet<QString>(simulatorRuns.cbegin(), simulatorRuns.cend()), probe::kKeepRuns - 1);

    QCOMPARE(stale, launchesWithoutTheSimulator);
}

void ProbeChannelsTest::directoriesThatAreNotRunStampsAreNeverDeleted()
{
    const QStringList strangers{
        QStringLiteral("measurements"),
        QStringLiteral("a340-2026-09-17"),
        QStringLiteral("20260917"),
        QStringLiteral("20260917-223100-extra"),
        QStringLiteral("20260917-223100-123-"),
        QStringLiteral("x20260917-223100"),
        QStringLiteral("job-42")};
    const QStringList runs{
        QStringLiteral("20260101-000001"),
        QStringLiteral("20260102-000001-000-1"),
        QStringLiteral("20260103-000001-000-2"),
        QStringLiteral("20260104-000001-000-3"),
        QStringLiteral("20260105-000001-000-4"),
        QStringLiteral("20260106-000001-000-5")};
    const QSet<QString> sawTheSimulator{
        QStringLiteral("measurements"),
        QStringLiteral("20260917-223100-extra"),
        QStringLiteral("20260101-000001"),
        QStringLiteral("20260102-000001-000-1"),
        QStringLiteral("20260103-000001-000-2"),
        QStringLiteral("20260104-000001-000-3"),
        QStringLiteral("20260105-000001-000-4"),
        QStringLiteral("20260106-000001-000-5")};

    const QStringList stale = probe::detail::StaleRuns(strangers + runs, sawTheSimulator, probe::kKeepRuns - 1);

    QCOMPARE(stale, (QStringList{QStringLiteral("20260101-000001"), QStringLiteral("20260102-000001-000-1")}));
    QVERIFY(probe::detail::StaleRuns(strangers, {}, probe::kKeepRuns - 1).isEmpty());
}

void ProbeChannelsTest::aRunSawTheSimulatorWhenItHoldsTheGateLogOrAWireLog()
{
    const QTemporaryDir base;

    QVERIFY(base.isValid());

    Touch(base.filePath(QStringLiteral("gate/turnaround.log")));
    Touch(base.filePath(QStringLiteral("wire/wire-20260101-000001-000-1.jsonl")));
    Touch(base.filePath(QStringLiteral("noise/client.log")));
    Touch(base.filePath(QStringLiteral("noise/session-20260101-000001-000-1.log")));
    QVERIFY(QDir().mkpath(base.filePath(QStringLiteral("empty"))));

    QVERIFY(probe::detail::SawTheSimulator(base.filePath(QStringLiteral("gate"))));
    QVERIFY(probe::detail::SawTheSimulator(base.filePath(QStringLiteral("wire"))));
    QVERIFY(!probe::detail::SawTheSimulator(base.filePath(QStringLiteral("noise"))));
    QVERIFY(!probe::detail::SawTheSimulator(base.filePath(QStringLiteral("empty"))));
}

void ProbeChannelsTest::eachChannelGetsItsOwnFile()
{
#ifndef NDEBUG
    probe::Append(probe::Channel::Client, QStringLiteral("c"));
    probe::Append(probe::Channel::GsxLVars, QStringLiteral("g"));
    probe::Append(probe::Channel::Writes, QStringLiteral("w"));

    const QString run = probe::RunLocation();

    QVERIFY(QFile::exists(run + QStringLiteral("/client.log")));
    QVERIFY(QFile::exists(run + QStringLiteral("/gsx-lvars.log")));
    QVERIFY(QFile::exists(run + QStringLiteral("/writes.log")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::everyChannelButTheWireAlsoLandsInTheSessionUnion()
{
#ifndef NDEBUG
    const QString wirePath = FreshWireLog();
    const QString unionPath = FreshUnionLog();

    probe::Append(probe::Channel::Turnaround, QStringLiteral("first-line"));
    probe::Append(probe::Channel::GsxMenu, QStringLiteral("second-line"));
    probe::Append(probe::Channel::Wire, QStringLiteral("wire-line"));

    const QString content = ReadAll(unionPath);

    QVERIFY(content.contains(QStringLiteral("first-line")));
    QVERIFY(content.contains(QStringLiteral("second-line")));
    QVERIFY(!content.contains(QStringLiteral("wire-line")));
    QVERIFY(ReadAll(wirePath).contains(QStringLiteral("wire-line")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::everyLineCarriesTheTimestampAndThePhase()
{
#ifndef NDEBUG
    probe::Append(probe::Channel::SimAVars, QStringLiteral("silent"));

    const QString path = probe::RunLocation() + QStringLiteral("/sim-avars.log");

    QVERIFY(QRegularExpression(QStringLiteral(
                "^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{3} \\[NoSession\\] silent$"))
                .match(LastLine(path))
                .hasMatch());

    probe::SetPhase("Boarding");
    probe::Append(probe::Channel::SimAVars, QStringLiteral("hello"));

    QVERIFY(QRegularExpression(QStringLiteral(
                "^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}\\.\\d{3} \\[Boarding\\] hello$"))
                .match(LastLine(path))
                .hasMatch());
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::theAircraftChannelsFollowTheAircraftId()
{
#ifndef NDEBUG
    probe::SetAircraft(QStringLiteral("pmdg-737"));
    probe::Append(probe::Channel::AircraftLVars, QStringLiteral("x"));

    const QString run = probe::RunLocation();

    QVERIFY(QFile::exists(run + QStringLiteral("/aircraft/pmdg-737-lvars.log")));

    probe::SetAircraft(QString());
    probe::Append(probe::Channel::AircraftLVars, QStringLiteral("y"));

    QVERIFY(QFile::exists(run + QStringLiteral("/aircraft/unknown-lvars.log")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::aRunFolderIsCreatedPerLaunch()
{
#ifndef NDEBUG
    const QString run = probe::RunLocation();

    QVERIFY(run.startsWith(qEnvironmentVariable("GSXI_PROBE_DIR")));
    QVERIFY(QRegularExpression(QStringLiteral("^\\d{8}-\\d{6}-\\d{3}-\\d+$"))
                .match(run.section(QLatin1Char('/'), -1))
                .hasMatch());
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::aCappedFileWritesOneCapLineAndThenGoesSilent()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest({.channelBytes = 200});
    const QString clientPath = FreshClientLog();
    const QString cap = probe::detail::CapMessage(200);

    probe::Append(probe::Channel::Client, Line(100));

    QVERIFY(!ReadAll(clientPath).contains(cap));

    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(clientPath).count(cap), 1);
    QVERIFY(LastLine(clientPath).endsWith(cap));

    const qint64 clientSize = QFileInfo(clientPath).size();

    probe::Append(probe::Channel::Client, Line(100));
    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(QFileInfo(clientPath).size(), clientSize);
    QCOMPARE(ReadAll(clientPath).count(cap), 1);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::aCappedChannelLeavesTheOtherFilesWriting()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest({.channelBytes = 200});
    const QString clientPath = FreshClientLog();
    const QString lvarsPath = FreshChannelLog(QStringLiteral("gsx-lvars.log"));
    const QString unionPath = FreshUnionLog();

    probe::Append(probe::Channel::Client, Line(100));
    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(clientPath).count(probe::detail::CapMessage(200)), 1);

    probe::Append(probe::Channel::Client, QStringLiteral("client-after-cap"));
    probe::Append(probe::Channel::GsxLVars, QStringLiteral("lvars-after-cap"));

    QVERIFY(!ReadAll(clientPath).contains(QStringLiteral("client-after-cap")));
    QVERIFY(ReadAll(lvarsPath).contains(QStringLiteral("lvars-after-cap")));
    QVERIFY(ReadAll(unionPath).contains(QStringLiteral("lvars-after-cap")));
    QVERIFY(ReadAll(unionPath).contains(QStringLiteral("client-after-cap")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::theUnionNamesTheFileThatCapped()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest({.channelBytes = 200});
    FreshClientLog();
    const QString unionPath = FreshUnionLog();
    const QString note = probe::detail::CapNote(QStringLiteral("client.log"), 200);

    probe::Append(probe::Channel::Client, Line(100));

    QVERIFY(!ReadAll(unionPath).contains(note));

    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(unionPath).count(note), 1);

    probe::Append(probe::Channel::Client, Line(100));
    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(unionPath).count(note), 1);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::theUnionNamesACappedWireFile()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest({.wireBytes = 200});
    const QString wirePath = FreshWireLog();
    const QString unionPath = FreshUnionLog();

    probe::Append(probe::Channel::Wire, Line(100));
    probe::Append(probe::Channel::Wire, Line(100));

    QCOMPARE(ReadAll(wirePath).count(probe::detail::CapMessage(200)), 1);
    QCOMPARE(ReadAll(unionPath).count(
                 probe::detail::CapNote(probe::detail::ChannelFileName(probe::Channel::Wire), 200)),
             1);
    QVERIFY(!ReadAll(unionPath).contains(Line(100)));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::theUnionCapsOnItsOwnBudget()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest({.unionBytes = 200, .channelBytes = 400});
    const QString clientPath = FreshClientLog();
    const QString unionPath = FreshUnionLog();

    probe::Append(probe::Channel::Client, Line(100));
    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(unionPath).count(probe::detail::CapMessage(200)), 1);
    QVERIFY(!ReadAll(clientPath).contains(probe::detail::CapMessage(400)));

    const qint64 unionSize = QFileInfo(unionPath).size();

    probe::Append(probe::Channel::Client, QStringLiteral("after-the-union-capped"));

    QVERIFY(ReadAll(clientPath).contains(QStringLiteral("after-the-union-capped")));
    QCOMPARE(QFileInfo(unionPath).size(), unionSize);

    probe::Append(probe::Channel::Client, Line(100));

    QCOMPARE(ReadAll(clientPath).count(probe::detail::CapMessage(400)), 1);
    QCOMPARE(QFileInfo(unionPath).size(), unionSize);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::appendIsSilentWhenTheGateIsOff()
{
    const QString base = qEnvironmentVariable("GSXI_PROBE_DIR");
    const QStringList before = EntriesRecursively(base);

    probe::SetEnabled(false);
    probe::Append(probe::Channel::Wire, QStringLiteral("hidden"));
    probe::Append(probe::Channel::Client, QStringLiteral("hidden"));

    QVERIFY(probe::RunLocation().isEmpty());
    QCOMPARE(EntriesRecursively(base), before);
}

void ProbeChannelsTest::aFirstChangeWritesImmediately()
{
#ifndef NDEBUG
    const QString path = FreshClientLog();

    probe::Change(probe::Channel::Client, "k", QStringLiteral("a"));

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QVERIFY(lines.last().endsWith(QStringLiteral(" a")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::anUnchangedSignatureNeverWritesAgain()
{
#ifndef NDEBUG
    const QString path = FreshClientLog();

    probe::Change(probe::Channel::Client, "k", QStringLiteral("a"));
    probe::Change(probe::Channel::Client, "k", QStringLiteral("a"));

    QCOMPARE(Lines(path).size(), 1);

    QTest::qWait(2100);
    probe::Change(probe::Channel::Client, "k", QStringLiteral("a"));

    QCOMPARE(Lines(path).size(), 1);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::aChangedSignatureWritesOnTheNextTickWhenTheFloorBlocksIt()
{
#ifndef NDEBUG
    const QString path = FreshClientLog();

    probe::Change(probe::Channel::Client, "k", QStringLiteral("a"));
    probe::Change(probe::Channel::Client, "k", QStringLiteral("b"));

    QCOMPARE(Lines(path).size(), 1);

    QTest::qWait(2100);
    probe::Change(probe::Channel::Client, "k", QStringLiteral("b"));

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines.last().endsWith(QStringLiteral(" b")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::theSignatureDecidesAndTheTextIsWhatLands()
{
#ifndef NDEBUG
    const QString path = FreshClientLog();

    probe::Change(probe::Channel::Client, "k", QStringLiteral("1"), QStringLiteral("value=1.04"));
    probe::Change(probe::Channel::Client, "k", QStringLiteral("1"), QStringLiteral("value=1.06"));

    QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QVERIFY(lines.last().endsWith(QStringLiteral("value=1.04")));

    QTest::qWait(2100);
    probe::Change(probe::Channel::Client, "k", QStringLiteral("2"), QStringLiteral("value=1.6"));

    lines = Lines(path);

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines.last().endsWith(QStringLiteral("value=1.6")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::aRemoteApiMessageLandsInTheMenuChannel()
{
#ifndef NDEBUG
    const QString menuPath = FreshChannelLog(QStringLiteral("gsx-menu.log"));
    const QString clientPath = FreshChannelLog(QStringLiteral("client.log"));

    probe::Sink(QStringLiteral("[GSX Integrator] RemoteAPI menu.open sent"));

    QVERIFY(QFile::exists(menuPath));
    QVERIFY(ReadAll(menuPath).contains(QStringLiteral("RemoteAPI menu.open sent")));
    QVERIFY(!ReadAll(clientPath).contains(QStringLiteral("RemoteAPI menu.open sent")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeChannelsTest::anyOtherMessageLandsInTheClientChannel()
{
#ifndef NDEBUG
    const QString menuPath = FreshChannelLog(QStringLiteral("gsx-menu.log"));
    const QString clientPath = FreshChannelLog(QStringLiteral("client.log"));

    probe::Sink(QStringLiteral("[GSX Integrator] something else"));

    QVERIFY(ReadAll(clientPath).contains(QStringLiteral("something else")));
    QVERIFY(!QFile::exists(menuPath) || !ReadAll(menuPath).contains(QStringLiteral("something else")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(ProbeChannelsTest)

#include "tst_probe_channels.moc"
