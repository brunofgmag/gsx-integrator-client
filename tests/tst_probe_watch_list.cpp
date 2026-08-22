#include <QtTest/QTest>

#include <vector>
#include "../src/infrastructure/probe/ProbeWatchList.h"

class ProbeWatchListTest final : public QObject
{
    Q_OBJECT

private slots:
    static void emptyTextWatchesNothing();
    static void readsTheThreeKindsOfName();
    static void avarCarriesItsUnit();
    static void skipsBlankLinesAndComments();
    static void unprefixedNameIsReadAsAnLVar();
    static void unreadableKindIsDropped();
};

void ProbeWatchListTest::emptyTextWatchesNothing()
{
    QVERIFY(probe::ParseWatchList("").empty());
}

void ProbeWatchListTest::readsTheThreeKindsOfName()
{
    const std::vector<probe::WatchedVariable> watched = probe::ParseWatchList(
        "L:FSDT_GSX_LOADER_EXIT_0\n"
        "A:BRAKE PARKING POSITION|Bool\n"
        "D:doors.entry.d3l\n");

    QCOMPARE(watched.size(), std::size_t{3});
    QCOMPARE(watched[0].kind, probe::WatchKind::LVar);
    QCOMPARE(QString::fromStdString(watched[0].name), QString("FSDT_GSX_LOADER_EXIT_0"));
    QCOMPARE(watched[1].kind, probe::WatchKind::AVar);
    QCOMPARE(QString::fromStdString(watched[1].name), QString("BRAKE PARKING POSITION"));
    QCOMPARE(watched[2].kind, probe::WatchKind::Dataref);
    QCOMPARE(QString::fromStdString(watched[2].name), QString("doors.entry.d3l"));
}

void ProbeWatchListTest::avarCarriesItsUnit()
{
    const std::vector<probe::WatchedVariable> watched =
        probe::ParseWatchList("A:FUEL TOTAL QUANTITY WEIGHT|kg\n");

    QCOMPARE(watched.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(watched[0].unit), QString("kg"));
}

void ProbeWatchListTest::skipsBlankLinesAndComments()
{
    const std::vector<probe::WatchedVariable> watched = probe::ParseWatchList(
        "# the loader exits nobody has read\n"
        "\n"
        "   \n"
        "L:FSDT_GSX_LOADER_EXIT_1\n");

    QCOMPARE(watched.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(watched[0].name), QString("FSDT_GSX_LOADER_EXIT_1"));
}

void ProbeWatchListTest::unprefixedNameIsReadAsAnLVar()
{
    const std::vector<probe::WatchedVariable> watched = probe::ParseWatchList("FSDT_GSX_STAIRS\n");

    QCOMPARE(watched.size(), std::size_t{1});
    QCOMPARE(watched[0].kind, probe::WatchKind::LVar);
    QCOMPARE(QString::fromStdString(watched[0].name), QString("FSDT_GSX_STAIRS"));
}

void ProbeWatchListTest::unreadableKindIsDropped()
{
    const std::vector<probe::WatchedVariable> watched = probe::ParseWatchList(
        "A:NO UNIT HERE\n"
        "L:\n"
        "L:GOOD_ONE\n");

    QCOMPARE(watched.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(watched[0].name), QString("GOOD_ONE"));
}

QTEST_APPLESS_MAIN(ProbeWatchListTest)

#include "tst_probe_watch_list.moc"
