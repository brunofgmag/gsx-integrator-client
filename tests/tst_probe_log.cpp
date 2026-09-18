#include <QtTest/QTest>

#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "../src/infrastructure/probe/ProbeLog.h"

namespace
{
    QString WirePath()
    {
        return probe::RunLocation() + QLatin1Char('/')
            + probe::detail::ChannelFileName(probe::Channel::Wire);
    }

    QString FreshWireLog()
    {
        const QString path = WirePath();
        QFile::remove(path);

        return path;
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
}

class ProbeLogTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    static void setEnabledTurnsTheGateOn();
    static void setEnabledFalseTurnsItOff();
    static void actingOnTheSimIsOffWithoutTheEnvironmentVariable();
    static void theToggleNeverTurnsOnTheActingHalf();
    static void releaseKeepsEverythingOff();
    static void twoIdenticalServicePatchesAreStoredOnce();
    static void aDifferingPatchIsStoredAfterTheElidedReference();
    static void typesOtherThanPatchAreNeverFiltered();
    static void identicalPayloadsOnDifferentPathsAreIndependent();
    static void equalityIsDecidedOnTheElidedText();

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
    const QString patch =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"revision\":1}");

    probe::Wire(patch);
    probe::Wire(patch);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()), patch);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::aDifferingPatchIsStoredAfterTheElidedReference()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"revision\":1}");
    const QString second =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"revision\":2}");

    probe::Wire(first);
    probe::Wire(second);
    probe::Wire(second);
    probe::Wire(second);
    probe::Wire(first);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 4);
    QCOMPARE(PayloadOf(lines.at(0)), first);
    QCOMPARE(PayloadOf(lines.at(1)), second);
    QCOMPARE(PayloadOf(lines.at(2)),
             QStringLiteral("{\"type\":\"elided\",\"path\":\"/services\",\"count\":2}"));
    QCOMPARE(PayloadOf(lines.at(3)), first);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void ProbeLogTest::typesOtherThanPatchAreNeverFiltered()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();

    probe::Wire(QStringLiteral("{\"type\":\"hello\",\"protocol\":1}"));
    probe::Wire(QStringLiteral("{\"type\":\"hello\",\"protocol\":1}"));
    probe::Wire(QStringLiteral("{\"type\":\"snapshot\",\"state\":{}}"));
    probe::Wire(QStringLiteral("{\"type\":\"snapshot\",\"state\":{}}"));
    probe::Wire(QStringLiteral("{\"type\":\"result\",\"ok\":true}"));
    probe::Wire(QStringLiteral("{\"type\":\"result\",\"ok\":true}"));
    probe::Wire(QStringLiteral("{\"type\":\"event\",\"name\":\"x\"}"));
    probe::Wire(QStringLiteral("{\"type\":\"event\",\"name\":\"x\"}"));
    probe::Wire(QStringLiteral("{\"type\":\"patch\"}"));
    probe::Wire(QStringLiteral("{\"type\":\"patch\"}"));
    probe::Wire(QStringLiteral("{\"type\":\"patch\",\"path\":7}"));
    probe::Wire(QStringLiteral("{\"type\":\"patch\",\"path\":7}"));
    probe::Wire(QStringLiteral("{\"type\":7}"));
    probe::Wire(QStringLiteral("{\"type\":7}"));
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
    const QString services =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"revision\":1}");
    const QString handlerData =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/handlerData\",\"revision\":1}");
    const QString servicesAgain =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"revision\":2}");
    const QString handlerDataAgain =
        QStringLiteral("{\"type\":\"patch\",\"path\":\"/handlerData\",\"revision\":2}");

    probe::Wire(services);
    probe::Wire(handlerData);
    probe::Wire(services);
    probe::Wire(handlerData);
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

void ProbeLogTest::equalityIsDecidedOnTheElidedText()
{
#ifndef NDEBUG
    const QString path = FreshWireLog();
    const QString first = QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"note\":\"%1\"}")
                              .arg(QString(300, QLatin1Char('a')));
    const QString second = QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"note\":\"%1\"}")
                               .arg(QString(300, QLatin1Char('b')));

    probe::Wire(first);
    probe::Wire(second);

    const QStringList lines = Lines(path);

    QCOMPARE(lines.size(), 1);
    QCOMPARE(PayloadOf(lines.first()),
             QStringLiteral("{\"type\":\"patch\",\"path\":\"/services\",\"note\":\"<elided 300 bytes>\"}"));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(ProbeLogTest)

#include "tst_probe_log.moc"
