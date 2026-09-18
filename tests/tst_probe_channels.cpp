#include <QtTest/QTest>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
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

    QString UnionPath(const QString& run)
    {
        const QStringList names =
            QDir(run).entryList(QStringList{QStringLiteral("session-*.log")}, QDir::Files);

        return names.isEmpty() ? QString() : run + QLatin1Char('/') + names.first();
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
}

class ProbeChannelsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    static void staleRunsKeepsTheNewestAndReturnsTheRest();
    static void eachChannelGetsItsOwnFile();
    static void everyLineAlsoLandsInTheSessionUnion();
    static void everyLineCarriesTheTimestampAndThePhase();
    static void theAircraftChannelsFollowTheAircraftId();
    static void aRunFolderIsCreatedPerLaunch();
    static void theBudgetWritesOneCapLineAndThenGoesSilent();
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

void ProbeChannelsTest::staleRunsKeepsTheNewestAndReturnsTheRest()
{
    const QStringList names{
        QStringLiteral("20260101-000001"),
        QStringLiteral("20260101-000002"),
        QStringLiteral("20260101-000003"),
        QStringLiteral("20260101-000004"),
        QStringLiteral("20260101-000005"),
        QStringLiteral("20260101-000006"),
        QStringLiteral("20260101-000007")};

    const QStringList stale = probe::detail::StaleRuns(names, probe::kKeepRuns - 1);

    QCOMPARE(stale.size(), 3);
    QCOMPARE(stale.first(), QStringLiteral("20260101-000001"));
    QCOMPARE(stale.last(), QStringLiteral("20260101-000003"));
    QVERIFY(probe::detail::StaleRuns(QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")},
                                     probe::kKeepRuns - 1)
                    .isEmpty());
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

void ProbeChannelsTest::everyLineAlsoLandsInTheSessionUnion()
{
#ifndef NDEBUG
    probe::Append(probe::Channel::Turnaround, QStringLiteral("first-line"));
    probe::Append(probe::Channel::GsxMenu, QStringLiteral("second-line"));

    const QString unionPath = UnionPath(probe::RunLocation());

    QVERIFY(!unionPath.isEmpty());

    const QString content = ReadAll(unionPath);

    QVERIFY(content.contains(QStringLiteral("first-line")));
    QVERIFY(content.contains(QStringLiteral("second-line")));
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

void ProbeChannelsTest::theBudgetWritesOneCapLineAndThenGoesSilent()
{
#ifndef NDEBUG
    probe::detail::SetBudgetForTest(200);

    probe::Append(probe::Channel::Client, QString(100, QLatin1Char('x')));
    probe::Append(probe::Channel::Client, QString(100, QLatin1Char('x')));

    const QString run = probe::RunLocation();
    const QString clientPath = run + QStringLiteral("/client.log");
    const QString unionPath = UnionPath(run);

    QCOMPARE(ReadAll(clientPath).count(probe::kCapMessage), 1);
    QCOMPARE(ReadAll(unionPath).count(probe::kCapMessage), 1);

    const qint64 clientSize = QFileInfo(clientPath).size();
    const qint64 unionSize = QFileInfo(unionPath).size();

    probe::Append(probe::Channel::Client, QString(100, QLatin1Char('x')));

    QCOMPARE(QFileInfo(clientPath).size(), clientSize);
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
