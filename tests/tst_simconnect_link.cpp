#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SimConnectLink.h"
#include "../src/infrastructure/simulator/SimConnectXml.h"

namespace
{
    QString XmlOf(const QString& userHome, const char* folder)
    {
        return userHome + QStringLiteral("/AppData/Roaming/") + QLatin1String(folder) + QStringLiteral("/SimConnect.xml");
    }

    QByteArray StockFile()
    {
        return FakeProtonTree::ReadFile(QStringLiteral(GSX_FIXTURES_DIR "/SimConnect.xml"));
    }

    QByteArray WithPort(QByteArray content, const QByteArray& port)
    {
        return content.replace("<Port>500</Port>", "<Port>" + port + "</Port>");
    }

    ProcessRunningCheck Running(const QString& processName)
    {
        return [processName](const QString& name) { return name == processName; };
    }

    bool NothingRuns(const QString&)
    {
        return false;
    }
}

class SimConnectLinkTest final : public QObject
{
    Q_OBJECT

private slots:
    static void withNothingRunningBothFilesGetThePortAndThe2024IsActive();
    static void theNewestSimulatorsPortIsUsedWhenNothingRuns();
    static void theRunningSimulatorsPortWins();
    static void aRunningSimulatorWithoutAPortIsToldToRestartAndItsFileIsLeftAlone();
    static void theClosedSimulatorGetsItsPortWhileTheOtherRuns();
    static void noSteamMeansTheDefaultPortAndNothingToRestart();
};

void SimConnectLinkTest::withNothingRunningBothFilesGetThePortAndThe2024IsActive()
{
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());

    const SimConnectLink link = PrepareSimConnectLink(tree.home, NothingRuns);

    QCOMPARE(link.port, kStaticSimConnectPort);
    QVERIFY(link.simulatorsToRestart.isEmpty());
    QCOMPARE(ReadSimConnectPort(XmlOf(tree.userHome2024, FakeProtonTree::kFolder2024)).value_or(0), kStaticSimConnectPort);
    QCOMPARE(ReadSimConnectPort(XmlOf(tree.userHome2020, FakeProtonTree::kFolder2020)).value_or(0), kStaticSimConnectPort);
}

void SimConnectLinkTest::theNewestSimulatorsPortIsUsedWhenNothingRuns()
{
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());
    FakeProtonTree::WriteFile(XmlOf(tree.userHome2024, FakeProtonTree::kFolder2024), WithPort(StockFile(), "5222"));
    FakeProtonTree::WriteFile(XmlOf(tree.userHome2020, FakeProtonTree::kFolder2020), WithPort(StockFile(), "6000"));

    QCOMPARE(PrepareSimConnectLink(tree.home, NothingRuns).port, quint16{5222});
}

void SimConnectLinkTest::theRunningSimulatorsPortWins()
{
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());
    FakeProtonTree::WriteFile(XmlOf(tree.userHome2024, FakeProtonTree::kFolder2024), WithPort(StockFile(), "5222"));
    FakeProtonTree::WriteFile(XmlOf(tree.userHome2020, FakeProtonTree::kFolder2020), WithPort(StockFile(), "6000"));

    const SimConnectLink link = PrepareSimConnectLink(tree.home, Running(QStringLiteral("FlightSimulator.exe")));

    QCOMPARE(link.port, quint16{6000});
    QVERIFY(link.simulatorsToRestart.isEmpty());
}

void SimConnectLinkTest::aRunningSimulatorWithoutAPortIsToldToRestartAndItsFileIsLeftAlone()
{
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());
    const QString xml2024 = XmlOf(tree.userHome2024, FakeProtonTree::kFolder2024);
    FakeProtonTree::WriteFile(xml2024, StockFile());

    const SimConnectLink link = PrepareSimConnectLink(tree.home, Running(QStringLiteral("FlightSimulator2024.exe")));

    QCOMPARE(link.simulatorsToRestart, QStringList{QStringLiteral("MSFS 2024 (Steam)")});
    QCOMPARE(link.port, kStaticSimConnectPort);
    QCOMPARE(FakeProtonTree::ReadFile(xml2024), StockFile());
}

void SimConnectLinkTest::theClosedSimulatorGetsItsPortWhileTheOtherRuns()
{
    const QTemporaryDir root;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(root.path());
    FakeProtonTree::WriteFile(XmlOf(tree.userHome2020, FakeProtonTree::kFolder2020), WithPort(StockFile(), "6000"));

    const SimConnectLink link = PrepareSimConnectLink(tree.home, Running(QStringLiteral("FlightSimulator.exe")));

    QCOMPARE(link.port, quint16{6000});
    QCOMPARE(ReadSimConnectPort(XmlOf(tree.userHome2024, FakeProtonTree::kFolder2024)).value_or(0), kStaticSimConnectPort);
}

void SimConnectLinkTest::noSteamMeansTheDefaultPortAndNothingToRestart()
{
    const QTemporaryDir root;

    const SimConnectLink link = PrepareSimConnectLink(root.path(), NothingRuns);

    QCOMPARE(link.port, kStaticSimConnectPort);
    QVERIFY(link.simulatorsToRestart.isEmpty());
}

QTEST_APPLESS_MAIN(SimConnectLinkTest)

#include "tst_simconnect_link.moc"
