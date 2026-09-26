#include <QtTest/QTest>

#include <optional>
#include <string>
#include "../src/infrastructure/probe/ProbeWriteMemo.h"

namespace
{
    constexpr auto kDoor = "L:FSS_B727_DOOR_FWD_PAX";
    constexpr auto kStation = "PAYLOAD STATION WEIGHT:1";
    constexpr long long kStart = 1000000;
}

class ProbeWriteMemoTest final : public QObject
{
    Q_OBJECT

private slots:
    static void theFirstWriteToANameIsLoggedAsTheFirst();
    static void aRepeatedValueStaysSilentUntilTenSecondsPass();
    static void aDifferentValueIsLoggedAtOnceWithEveryWriteCounted();
    static void theTenSecondsRunFromTheLastLoggedWrite();
    static void eachNameKeepsItsOwnMemo();
};

void ProbeWriteMemoTest::theFirstWriteToANameIsLoggedAsTheFirst()
{
    probe::WriteMemo memo;

    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart), std::optional<int>(1));
}

void ProbeWriteMemoTest::aRepeatedValueStaysSilentUntilTenSecondsPass()
{
    probe::WriteMemo memo;

    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart), std::optional<int>(1));
    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart + 5000), std::optional<int>());
    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart + 9999), std::optional<int>());
    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart + 10000), std::optional<int>(4));
}

void ProbeWriteMemoTest::aDifferentValueIsLoggedAtOnceWithEveryWriteCounted()
{
    probe::WriteMemo memo;

    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart), std::optional<int>(1));
    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart + 100), std::optional<int>());
    QCOMPARE(memo.Record(kDoor, QStringLiteral("0"), kStart + 200), std::optional<int>(3));
}

void ProbeWriteMemoTest::theTenSecondsRunFromTheLastLoggedWrite()
{
    probe::WriteMemo memo;

    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart), std::optional<int>(1));
    QCOMPARE(memo.Record(kDoor, QStringLiteral("0"), kStart + 8000), std::optional<int>(2));
    QCOMPARE(memo.Record(kDoor, QStringLiteral("0"), kStart + 12000), std::optional<int>());
    QCOMPARE(memo.Record(kDoor, QStringLiteral("0"), kStart + 18000), std::optional<int>(4));
}

void ProbeWriteMemoTest::eachNameKeepsItsOwnMemo()
{
    probe::WriteMemo memo;

    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart), std::optional<int>(1));
    QCOMPARE(memo.Record(kStation, QStringLiteral("1"), kStart), std::optional<int>(1));
    QCOMPARE(memo.Record(kDoor, QStringLiteral("1"), kStart + 1), std::optional<int>());
    QCOMPARE(memo.Record(kStation, QStringLiteral("2"), kStart + 1), std::optional<int>(2));
}

QTEST_APPLESS_MAIN(ProbeWriteMemoTest)

#include "tst_probe_write_memo.moc"
