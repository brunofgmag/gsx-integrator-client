#include <QtTest/QTest>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "../src/infrastructure/probe/ProbeLog.h"

namespace
{
    constexpr auto kServicesBefore =
        "[{\"id\":\"Boarding\",\"state\":\"available\",\"stateText\":\"Boarding service can be requested\"},"
        "{\"id\":\"Catering\",\"state\":\"available\",\"stateText\":\"Catering service can be requested\"}]";
    constexpr auto kServicesCateringRequested =
        "[{\"id\":\"Boarding\",\"state\":\"available\",\"stateText\":\"Boarding service can be requested\"},"
        "{\"id\":\"Catering\",\"state\":\"requested\",\"stateText\":\"Catering service requested\"}]";
    constexpr auto kServicesBothRequested =
        "[{\"id\":\"Boarding\",\"state\":\"requested\",\"stateText\":\"Boarding service can be requested\"},"
        "{\"id\":\"Catering\",\"state\":\"requested\",\"stateText\":\"Catering service requested\"}]";

    QString FreshLog(const probe::Channel channel)
    {
        const QString path = probe::RunLocation() + QLatin1Char('/') + probe::detail::ChannelFileName(channel);
        QFile::remove(path);

        return path;
    }

    QString FreshWireLog()
    {
        return FreshLog(probe::Channel::Wire);
    }

    QStringList Lines(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    QString PayloadOf(const QString& line)
    {
        const qsizetype marker = line.indexOf(QStringLiteral("] "));

        return marker < 0 ? line : line.mid(marker + 2);
    }

    QString Patch(const QString& path, const int timestamp, const QString& value)
    {
        return QStringLiteral("{\"v\":1,\"type\":\"patch\",\"ts\":%1,\"path\":\"%2\",\"value\":%3}")
            .arg(QString::number(timestamp), path, value);
    }

    QString Snapshot(const int timestamp)
    {
        return QStringLiteral("{\"v\":1,\"type\":\"snapshot\",\"ts\":%1,\"state\":5}").arg(timestamp);
    }

    QJsonValue Json(const char* text)
    {
        return QJsonDocument::fromJson(QByteArray("[") + text + "]").array().first();
    }

    QString PatchText(const char* before, const char* after)
    {
        const QJsonArray ops = probe::detail::JsonPatch(Json(before), Json(after));

        return QString::fromUtf8(QJsonDocument(ops).toJson(QJsonDocument::Compact));
    }
}

class ProbeLogTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    static void cleanupTestCase();

    static void setEnabledTurnsTheGateOn();
    static void setEnabledFalseTurnsItOff();
    static void actingOnTheSimIsOffWithoutTheEnvironmentVariable();
    static void theToggleNeverTurnsOnTheActingHalf();
    static void releaseKeepsEverythingOff();
    static void twoIdenticalServicePatchesAreStoredOnce();
    static void patchesThatDifferOnlyInTheTimestampAreStoredOnce();
    static void aDifferingPatchIsStoredAfterTheElidedReference();
    static void typesOtherThanPatchAreNeverFiltered();
    static void identicalPayloadsOnDifferentPathsAreIndependent();
    static void equalityIsDecidedOnTheElidedValue();
    static void aRepeatedPathIsRecordedAsADiffOfTheLastRecordedValue();
    static void aDiffNoShorterThanThePatchIsWrittenInFull();
    static void aSnapshotWritesTheNextPatchOfEveryPathInFull();
    static void aSnapshotWritesThePendingElidedCountsFirst();
    static void theJsonPatchEscapesKeysInThePointer();
    static void theJsonPatchReplacesItemsAndRemovesTheTailFromTheEnd();
    static void theJsonPatchAddsTheTailInOrder();
    static void theJsonPatchRecursesIntoObjectsAndReplacesOnATypeChange();
    static void theJsonPatchReplacesTheRootWhenItsTypeChanges();
    static void theJsonPatchOfEqualValuesIsEmpty();
    static void aSentMessageIsRecordedWholeEveryTime();
    static void aSentTextThatIsNotAnObjectIsRecordedRaw();
    static void aTransitionLandsInTheTurnaroundLog();
    static void anyOtherMessageStaysInTheClientLog();

private:
    QTemporaryDir directory_;
};

void ProbeLogTest::initTestCase()
{
    qunsetenv("GSXI_PROBE");

    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
}

void ProbeLogTest::init()
{
#ifndef NDEBUG
    probe::SetEnabled(true);
    probe::ResetForTest();
    probe::ResetWireMemoForTest();
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::cleanupTestCase()
{
#ifndef NDEBUG
    probe::ResetForTest();
#endif
}

void ProbeLogTest::setEnabledTurnsTheGateOn()
{
    probe::SetEnabled(true);

    QVERIFY(probe::IsOn());
}

void ProbeLogTest::setEnabledFalseTurnsItOff()
{
    probe::SetEnabled(false);

    QVERIFY(!probe::IsOn());
}

void ProbeLogTest::actingOnTheSimIsOffWithoutTheEnvironmentVariable()
{
    QVERIFY(!probe::ActsOnTheSim());
}

void ProbeLogTest::theToggleNeverTurnsOnTheActingHalf()
{
    probe::SetEnabled(true);

    QVERIFY(probe::IsOn());
    QVERIFY(!probe::ActsOnTheSim());
}

void ProbeLogTest::releaseKeepsEverythingOff()
{
#ifdef NDEBUG
    probe::SetEnabled(true);

    QVERIFY(!probe::IsOn());
#else
    QSKIP("only meaningful in release");
#endif
}

void ProbeLogTest::twoIdenticalServicePatchesAreStoredOnce()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString patch = Patch(QStringLiteral("/services"), 100, QStringLiteral("1"));

    probe::Wire(patch);
    probe::Wire(patch);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), patch);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::patchesThatDifferOnlyInTheTimestampAreStoredOnce()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = Patch(QStringLiteral("/statusHtml"), 100, QStringLiteral("\"<b>Boarding</b>\""));
    const QString second = Patch(QStringLiteral("/statusHtml"), 101, QStringLiteral("\"<b>Boarding</b>\""));

    probe::Wire(first);
    probe::Wire(second);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), first);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aDifferingPatchIsStoredAfterTheElidedReference()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = Patch(QStringLiteral("/services"), 100, QStringLiteral("1"));
    const QString second = Patch(QStringLiteral("/services"), 101, QStringLiteral("2"));
    const QString third = Patch(QStringLiteral("/services"), 104, QStringLiteral("1"));

    probe::Wire(first);
    probe::Wire(second);
    probe::Wire(Patch(QStringLiteral("/services"), 102, QStringLiteral("2")));
    probe::Wire(Patch(QStringLiteral("/services"), 103, QStringLiteral("2")));
    probe::Wire(third);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 4);
    QCOMPARE(PayloadOf(lines.at(0)), first);
    QCOMPARE(PayloadOf(lines.at(1)), second);
    QCOMPARE(PayloadOf(lines.at(2)),
             QStringLiteral("{\"type\":\"elided\",\"path\":\"/services\",\"count\":2}"));
    QCOMPARE(PayloadOf(lines.at(3)), third);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::typesOtherThanPatchAreNeverFiltered()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();

    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"hello\",\"ts\":100,\"protocol\":1}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"hello\",\"ts\":100,\"protocol\":1}"));
    probe::Wire(Snapshot(100));
    probe::Wire(Snapshot(100));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"result\",\"ts\":100,\"ok\":true}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"result\",\"ts\":100,\"ok\":true}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"event\",\"ts\":100,\"name\":\"x\"}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"event\",\"ts\":100,\"name\":\"x\"}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"patch\",\"ts\":100}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"patch\",\"ts\":100}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"patch\",\"ts\":100,\"path\":7}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":\"patch\",\"ts\":100,\"path\":7}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":7,\"ts\":100}"));
    probe::Wire(QStringLiteral("{\"v\":1,\"type\":7,\"ts\":100}"));
    probe::Wire(QStringLiteral("[1,2]"));
    probe::Wire(QStringLiteral("[1,2]"));
    probe::Wire(QStringLiteral("not json"));
    probe::Wire(QStringLiteral("not json"));

    QCOMPARE(Lines(path).size(), 18);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::identicalPayloadsOnDifferentPathsAreIndependent()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString services = Patch(QStringLiteral("/services"), 100, QStringLiteral("1"));
    const QString handlerData = Patch(QStringLiteral("/handlerData"), 100, QStringLiteral("1"));
    const QString servicesAgain = Patch(QStringLiteral("/services"), 102, QStringLiteral("2"));
    const QString handlerDataAgain = Patch(QStringLiteral("/handlerData"), 102, QStringLiteral("2"));

    probe::Wire(services);
    probe::Wire(handlerData);
    probe::Wire(Patch(QStringLiteral("/services"), 101, QStringLiteral("1")));
    probe::Wire(Patch(QStringLiteral("/handlerData"), 101, QStringLiteral("1")));
    probe::Wire(servicesAgain);
    probe::Wire(handlerDataAgain);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 6);
    QCOMPARE(PayloadOf(lines.at(0)), services);
    QCOMPARE(PayloadOf(lines.at(1)), handlerData);
    QCOMPARE(PayloadOf(lines.at(2)),
             QStringLiteral("{\"type\":\"elided\",\"path\":\"/services\",\"count\":1}"));
    QCOMPARE(PayloadOf(lines.at(3)), servicesAgain);
    QCOMPARE(PayloadOf(lines.at(4)),
             QStringLiteral("{\"type\":\"elided\",\"path\":\"/handlerData\",\"count\":1}"));
    QCOMPARE(PayloadOf(lines.at(5)), handlerDataAgain);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::equalityIsDecidedOnTheElidedValue()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = Patch(QStringLiteral("/services"), 100,
                                QStringLiteral("{\"note\":\"%1\"}").arg(QString(300, QLatin1Char('a'))));
    const QString second = Patch(QStringLiteral("/services"), 101,
                                 QStringLiteral("{\"note\":\"%1\"}").arg(QString(300, QLatin1Char('b'))));

    probe::Wire(first);
    probe::Wire(second);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()),
             Patch(QStringLiteral("/services"), 100, QStringLiteral("{\"note\":\"<elided 300 bytes>\"}")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aRepeatedPathIsRecordedAsADiffOfTheLastRecordedValue()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = Patch(QStringLiteral("/services"), 100, QLatin1String(kServicesBefore));

    probe::Wire(first);
    probe::Wire(Patch(QStringLiteral("/services"), 101, QLatin1String(kServicesCateringRequested)));
    probe::Wire(Patch(QStringLiteral("/services"), 102, QLatin1String(kServicesBothRequested)));

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 3);
    QCOMPARE(PayloadOf(lines.at(0)), first);
    QCOMPARE(PayloadOf(lines.at(1)),
             QStringLiteral("{\"type\":\"patch-diff\",\"ts\":101,\"path\":\"/services\",\"ops\":["
                 "{\"op\":\"replace\",\"path\":\"/1/state\",\"value\":\"requested\"},"
                 "{\"op\":\"replace\",\"path\":\"/1/stateText\",\"value\":\"Catering service requested\"}]}"));
    QCOMPARE(PayloadOf(lines.at(2)),
             QStringLiteral("{\"type\":\"patch-diff\",\"ts\":102,\"path\":\"/services\",\"ops\":["
                 "{\"op\":\"replace\",\"path\":\"/0/state\",\"value\":\"requested\"}]}"));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aDiffNoShorterThanThePatchIsWrittenInFull()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString second = Patch(QStringLiteral("/menuShown"), 101, QStringLiteral("false"));

    probe::Wire(Patch(QStringLiteral("/menuShown"), 100, QStringLiteral("true")));
    probe::Wire(second);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 2);
    QCOMPARE(PayloadOf(lines.at(1)), second);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aSnapshotWritesTheNextPatchOfEveryPathInFull()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString afterSnapshot = Patch(QStringLiteral("/services"), 103, QLatin1String(kServicesBothRequested));

    probe::Wire(Patch(QStringLiteral("/services"), 100, QLatin1String(kServicesBefore)));
    probe::Wire(Patch(QStringLiteral("/services"), 101, QLatin1String(kServicesCateringRequested)));
    probe::Wire(Snapshot(102));
    probe::Wire(afterSnapshot);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 4);
    QVERIFY(PayloadOf(lines.at(1)).startsWith(QStringLiteral("{\"type\":\"patch-diff\"")));
    QCOMPARE(PayloadOf(lines.at(2)), Snapshot(102));
    QCOMPARE(PayloadOf(lines.at(3)), afterSnapshot);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aSnapshotWritesThePendingElidedCountsFirst()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = Patch(QStringLiteral("/services"), 100, QStringLiteral("1"));
    const QString afterSnapshot = Patch(QStringLiteral("/services"), 104, QStringLiteral("1"));

    probe::Wire(first);
    probe::Wire(Patch(QStringLiteral("/services"), 101, QStringLiteral("1")));
    probe::Wire(Patch(QStringLiteral("/services"), 102, QStringLiteral("1")));
    probe::Wire(Snapshot(103));
    probe::Wire(afterSnapshot);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 4);
    QCOMPARE(PayloadOf(lines.at(0)), first);
    QCOMPARE(PayloadOf(lines.at(1)),
             QStringLiteral("{\"type\":\"elided\",\"path\":\"/services\",\"count\":2}"));
    QCOMPARE(PayloadOf(lines.at(2)), Snapshot(103));
    QCOMPARE(PayloadOf(lines.at(3)), afterSnapshot);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::theJsonPatchEscapesKeysInThePointer()
{
    QCOMPARE(PatchText("{\"a/b\":1,\"m~n\":1,\"~1\":1}", "{\"a/b\":2,\"m~n\":2,\"~1\":2}"),
             QStringLiteral("[{\"op\":\"replace\",\"path\":\"/a~1b\",\"value\":2},"
                 "{\"op\":\"replace\",\"path\":\"/m~0n\",\"value\":2},"
                 "{\"op\":\"replace\",\"path\":\"/~01\",\"value\":2}]"));
}

void ProbeLogTest::theJsonPatchReplacesItemsAndRemovesTheTailFromTheEnd()
{
    QCOMPARE(PatchText("[1,2,3,4]", "[1,9]"),
             QStringLiteral("[{\"op\":\"replace\",\"path\":\"/1\",\"value\":9},"
                 "{\"op\":\"remove\",\"path\":\"/3\"},{\"op\":\"remove\",\"path\":\"/2\"}]"));
}

void ProbeLogTest::theJsonPatchAddsTheTailInOrder()
{
    QCOMPARE(PatchText("{\"list\":[{\"a\":1}]}", "{\"list\":[{\"a\":2},{\"a\":3},4]}"),
             QStringLiteral("[{\"op\":\"replace\",\"path\":\"/list/0/a\",\"value\":2},"
                 "{\"op\":\"add\",\"path\":\"/list/1\",\"value\":{\"a\":3}},{\"op\":\"add\",\"path\":\"/list/2\",\"value\":4}]"));
}

void ProbeLogTest::theJsonPatchRecursesIntoObjectsAndReplacesOnATypeChange()
{
    QCOMPARE(PatchText("{\"a\":1,\"b\":{\"c\":1},\"d\":\"x\",\"g\":true}", "{\"a\":1,\"b\":{\"c\":2},\"d\":[1],\"f\":null}"),
             QStringLiteral("[{\"op\":\"replace\",\"path\":\"/b/c\",\"value\":2},"
                 "{\"op\":\"replace\",\"path\":\"/d\",\"value\":[1]},"
                 "{\"op\":\"remove\",\"path\":\"/g\"},{\"op\":\"add\",\"path\":\"/f\",\"value\":null}]"));
}

void ProbeLogTest::theJsonPatchReplacesTheRootWhenItsTypeChanges()
{
    QCOMPARE(PatchText("1", "\"x\""), QStringLiteral("[{\"op\":\"replace\",\"path\":\"\",\"value\":\"x\"}]"));
}

void ProbeLogTest::theJsonPatchOfEqualValuesIsEmpty()
{
    QCOMPARE(PatchText("{\"a\":[1,{\"b\":2}]}", "{\"a\":[1,{\"b\":2}]}"), QStringLiteral("[]"));
}

void ProbeLogTest::aSentMessageIsRecordedWholeEveryTime()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString subscribe = QStringLiteral("{\"channels\":[\"state\",\"prompts\",\"toasts\"],\"type\":\"subscribe\"}");

    probe::WireSent(subscribe);
    probe::WireSent(subscribe);

    const QStringList lines = Lines(path);
    const QString expected = QStringLiteral("{\"type\":\"sent\",\"message\":") + subscribe + QLatin1Char('}');

    QCOMPARE(lines.size(), 2);
    QCOMPARE(PayloadOf(lines.at(0)), expected);
    QCOMPARE(PayloadOf(lines.at(1)), expected);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aSentTextThatIsNotAnObjectIsRecordedRaw()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();

    probe::WireSent(QStringLiteral("menu \"open\""));

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), QStringLiteral("{\"type\":\"sent\",\"raw\":\"menu \\\"open\\\"\"}"));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aTransitionLandsInTheTurnaroundLog()
{
#ifndef NDEBUG
    const QString turnaround = FreshLog(probe::Channel::Turnaround);
    const QString client = FreshLog(probe::Channel::Client);
    const QString message =
        QStringLiteral("[GSX Integrator] Transitioning: WaitingSupportedAircraft -> WaitingAircraftReady");

    probe::Sink(message);

    const QStringList lines = Lines(turnaround);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), QStringLiteral("qt   ") + message);
    QVERIFY(Lines(client).isEmpty());
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::anyOtherMessageStaysInTheClientLog()
{
#ifndef NDEBUG
    const QString turnaround = FreshLog(probe::Channel::Turnaround);
    const QString client = FreshLog(probe::Channel::Client);
    const QString message = QStringLiteral("[GSX Integrator] Aircraft detected: FSS E195");

    probe::Sink(message);

    const QStringList lines = Lines(client);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), QStringLiteral("qt   ") + message);
    QVERIFY(Lines(turnaround).isEmpty());
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(ProbeLogTest)

#include "tst_probe_log.moc"
