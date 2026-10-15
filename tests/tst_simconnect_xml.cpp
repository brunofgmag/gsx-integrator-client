#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "FakeProtonTree.h"
#include "../src/infrastructure/simulator/SimConnectXml.h"
#include "../src/infrastructure/simulator/SteamLibraries.h"

namespace
{
    QByteArray StockFile(const QByteArray& eol = "\r\n")
    {
        QByteArray content = FakeProtonTree::ReadFile(QStringLiteral(GSX_FIXTURES_DIR "/SimConnect.xml"));
        content.replace("\r\n", "\n");

        return content.replace("\n", eol);
    }

    QByteArray EntryText(const QByteArray& indent, const QByteArray& eol)
    {
        const QByteArray inner = indent + indent;

        return indent + "<SimConnect.Comm>" + eol
            + inner + "<Descr>Static IP4 port</Descr>" + eol
            + inner + "<Protocol>IPv4</Protocol>" + eol
            + inner + "<Scope>local</Scope>" + eol
            + inner + "<MaxClients>64</MaxClients>" + eol
            + inner + "<Port>5111</Port>" + eol
            + inner + "<MaxRecvSize>41088</MaxRecvSize>" + eol
            + indent + "</SimConnect.Comm>" + eol;
    }

    QByteArray WithPort(QByteArray content, const QByteArray& port)
    {
        return content.replace("<Port>500</Port>", "<Port>" + port + "</Port>");
    }

    bool NeverRunning()
    {
        return false;
    }

    bool AlwaysRunning()
    {
        return true;
    }
}

class SimConnectXmlTest final : public QObject
{
    Q_OBJECT

private slots:
    static void aStockFileHasNoUsablePort();
    static void readsThePortOfTheStaticIpv4EntryOnceItIsRaised();
    static void ignoresPortsOfOtherProtocols();
    static void ignoresDisabledEntries();
    static void ignoresCommentedEntries();
    static void theFirstUsableEntryWins();
    static void ignoresPortsOutOfRange();
    static void readsThePortFromDisk();
    static void aMissingFileHasNoPort();
    static void aPresentPortIsKeptAndNothingIsWritten();
    static void aPresentPortIsReportedEvenWhileTheSimulatorRuns();
    static void addsTheEntryKeepingEveryOtherByteAndCrlf();
    static void keepsLfLineEndings();
    static void keepsTheIndentationOfTheExistingEntries();
    static void addsOnlyOnceAcrossCalls();
    static void refusesToWriteWhileTheSimulatorRuns();
    static void createsTheFileWithOnlyTheEntry();
    static void creatingIsRefusedWhileTheSimulatorRuns();
    static void createsTheMissingFolder();
    static void anUnrecognisedDocumentIsLeftUntouched();
    static void targetsListEverySimulatorWithItsFile();
};

void SimConnectXmlTest::aStockFileHasNoUsablePort()
{
    QVERIFY(!ParseSimConnectPort(StockFile()).has_value());
}

void SimConnectXmlTest::readsThePortOfTheStaticIpv4EntryOnceItIsRaised()
{
    QCOMPARE(ParseSimConnectPort(WithPort(StockFile(), "5111")), std::optional<quint16>(5111));
    QCOMPARE(ParseSimConnectPort(WithPort(StockFile(), "1024")), std::optional<quint16>(1024));
    QCOMPARE(ParseSimConnectPort(WithPort(StockFile(), " 7000 ")), std::optional<quint16>(7000));
    QCOMPARE(ParseSimConnectPort(WithPort(StockFile("\n"), "5111").replace("IPv4", "ipv4")),
             std::optional<quint16>(5111));
}

void SimConnectXmlTest::ignoresPortsOfOtherProtocols()
{
    QByteArray content = WithPort(StockFile(), "5111");
    content.replace("<Protocol>IPv4</Protocol>", "<Protocol>IPv6</Protocol>");

    QVERIFY(!ParseSimConnectPort(content).has_value());
}

void SimConnectXmlTest::ignoresDisabledEntries()
{
    QByteArray content = WithPort(StockFile(), "5111");
    content.replace("<Disabled>False</Disabled>\r\n\t\t<Descr>Static IP4 port</Descr>",
                    "<Disabled>True</Disabled>\r\n\t\t<Descr>Static IP4 port</Descr>");

    QVERIFY(!ParseSimConnectPort(content).has_value());
}

void SimConnectXmlTest::ignoresCommentedEntries()
{
    const QByteArray content =
        "<SimBase.Document Type=\"SimConnect\" version=\"1,0\">\n"
        "<!--\n<SimConnect.Comm>\n<Protocol>IPv4</Protocol>\n<Port>5111</Port>\n</SimConnect.Comm>\n-->\n"
        "</SimBase.Document>\n";

    QVERIFY(!ParseSimConnectPort(content).has_value());
}

void SimConnectXmlTest::theFirstUsableEntryWins()
{
    const QByteArray content =
        "<SimBase.Document><SimConnect.Comm><Protocol>IPv4</Protocol><Port>500</Port></SimConnect.Comm>"
        "<SimConnect.Comm><Protocol>IPv4</Protocol><Port>6000</Port></SimConnect.Comm>"
        "<SimConnect.Comm><Protocol>IPv4</Protocol><Port>7000</Port></SimConnect.Comm></SimBase.Document>";

    QCOMPARE(ParseSimConnectPort(content), std::optional<quint16>(6000));
}

void SimConnectXmlTest::ignoresPortsOutOfRange()
{
    QVERIFY(!ParseSimConnectPort(WithPort(StockFile(), "70000")).has_value());
    QVERIFY(!ParseSimConnectPort(WithPort(StockFile(), "1023")).has_value());
    QVERIFY(!ParseSimConnectPort(WithPort(StockFile(), "abc")).has_value());
    QVERIFY(!ParseSimConnectPort(WithPort(StockFile(), "")).has_value());
}

void SimConnectXmlTest::readsThePortFromDisk()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    FakeProtonTree::WriteFile(path, WithPort(StockFile(), "5111"));

    QCOMPARE(ReadSimConnectPort(path), std::optional<quint16>(5111));
}

void SimConnectXmlTest::aMissingFileHasNoPort()
{
    const QTemporaryDir dir;

    QVERIFY(!ReadSimConnectPort(dir.filePath(QStringLiteral("SimConnect.xml"))).has_value());
}

void SimConnectXmlTest::aPresentPortIsKeptAndNothingIsWritten()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    const QByteArray original = WithPort(StockFile(), "6001");
    FakeProtonTree::WriteFile(path, original);

    const SimConnectXmlResult result = EnsureSimConnectPort(path, NeverRunning);

    QCOMPARE(result.status, SimConnectXmlStatus::Present);
    QCOMPARE(result.port, quint16(6001));
    QCOMPARE(FakeProtonTree::ReadFile(path), original);
}

void SimConnectXmlTest::aPresentPortIsReportedEvenWhileTheSimulatorRuns()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    FakeProtonTree::WriteFile(path, WithPort(StockFile(), "5111"));

    const SimConnectXmlResult result = EnsureSimConnectPort(path, AlwaysRunning);

    QCOMPARE(result.status, SimConnectXmlStatus::Present);
    QCOMPARE(result.port, quint16(5111));
}

void SimConnectXmlTest::addsTheEntryKeepingEveryOtherByteAndCrlf()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    const QByteArray original = StockFile();
    FakeProtonTree::WriteFile(path, original);

    const SimConnectXmlResult result = EnsureSimConnectPort(path, NeverRunning);

    QCOMPARE(result.status, SimConnectXmlStatus::Added);
    QCOMPARE(result.port, quint16(5111));

    QByteArray expected = original;
    expected.replace("</SimBase.Document>", EntryText("\t", "\r\n") + "</SimBase.Document>");
    const QByteArray written = FakeProtonTree::ReadFile(path);

    QCOMPARE(written, expected);
    QCOMPARE(written.count("\n"), written.count("\r\n"));
    QCOMPARE(ReadSimConnectPort(path), std::optional<quint16>(5111));
}

void SimConnectXmlTest::keepsLfLineEndings()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    const QByteArray original = StockFile("\n");
    FakeProtonTree::WriteFile(path, original);

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Added);

    QByteArray expected = original;
    expected.replace("</SimBase.Document>", EntryText("\t", "\n") + "</SimBase.Document>");

    QCOMPARE(FakeProtonTree::ReadFile(path), expected);
    QVERIFY(!FakeProtonTree::ReadFile(path).contains('\r'));
}

void SimConnectXmlTest::keepsTheIndentationOfTheExistingEntries()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    QByteArray original = StockFile("\n");
    original.replace("\t", "  ");
    FakeProtonTree::WriteFile(path, original);

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Added);

    QByteArray expected = original;
    expected.replace("</SimBase.Document>", EntryText("  ", "\n") + "</SimBase.Document>");

    QCOMPARE(FakeProtonTree::ReadFile(path), expected);
}

void SimConnectXmlTest::addsOnlyOnceAcrossCalls()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    FakeProtonTree::WriteFile(path, StockFile());

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Added);
    const QByteArray afterFirst = FakeProtonTree::ReadFile(path);

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Present);
    QCOMPARE(FakeProtonTree::ReadFile(path), afterFirst);
}

void SimConnectXmlTest::refusesToWriteWhileTheSimulatorRuns()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    const QByteArray original = StockFile();
    FakeProtonTree::WriteFile(path, original);

    const SimConnectXmlResult result = EnsureSimConnectPort(path, AlwaysRunning);

    QCOMPARE(result.status, SimConnectXmlStatus::SimulatorRunning);
    QCOMPARE(FakeProtonTree::ReadFile(path), original);
}

void SimConnectXmlTest::createsTheFileWithOnlyTheEntry()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));

    const SimConnectXmlResult result = EnsureSimConnectPort(path, NeverRunning);

    QCOMPARE(result.status, SimConnectXmlStatus::Created);
    QCOMPARE(result.port, quint16(5111));

    const QByteArray written = FakeProtonTree::ReadFile(path);

    QVERIFY(written.contains("<SimBase.Document Type=\"SimConnect\" version=\"1,0\">"));
    QVERIFY(written.contains("<Filename>SimConnect.xml</Filename>"));
    QCOMPARE(written.count("<SimConnect.Comm>"), 1);
    QCOMPARE(written.count("\n"), written.count("\r\n"));
    QCOMPARE(ReadSimConnectPort(path), std::optional<quint16>(5111));
    QVERIFY(written.endsWith("</SimBase.Document>\r\n"));
}

void SimConnectXmlTest::creatingIsRefusedWhileTheSimulatorRuns()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));

    QCOMPARE(EnsureSimConnectPort(path, AlwaysRunning).status, SimConnectXmlStatus::SimulatorRunning);
    QVERIFY(!QFileInfo::exists(path));
}

void SimConnectXmlTest::createsTheMissingFolder()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("AppData/Roaming/Microsoft Flight Simulator/SimConnect.xml"));

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Created);
    QVERIFY(QFileInfo::exists(path));
}

void SimConnectXmlTest::anUnrecognisedDocumentIsLeftUntouched()
{
    const QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("SimConnect.xml"));
    const QByteArray original = "not xml at all\n";
    FakeProtonTree::WriteFile(path, original);

    QCOMPARE(EnsureSimConnectPort(path, NeverRunning).status, SimConnectXmlStatus::Failed);
    QCOMPARE(FakeProtonTree::ReadFile(path), original);
}

void SimConnectXmlTest::targetsListEverySimulatorWithItsFile()
{
#ifdef Q_OS_WIN
    QSKIP("Proton prefixes exist only on Linux");
#endif
    const QTemporaryDir home;
    const FakeProtonTree::Tree tree = FakeProtonTree::Build(home.path());
    const QString existing2024 = tree.userHome2024 + QStringLiteral("/AppData/Roaming/")
        + QLatin1String(FakeProtonTree::kFolder2024) + QStringLiteral("/simconnect.xml");
    FakeProtonTree::WriteFile(existing2024, StockFile());

    const std::vector<SimConnectXmlTarget> targets = SimConnectXmlTargets(tree.home);

    QCOMPARE(targets.size(), 2u);
    QCOMPARE(targets[0].label, QStringLiteral("MSFS 2024 (Steam)"));
    QCOMPARE(targets[0].path, existing2024);
    QCOMPARE(targets[0].processName, QStringLiteral("FlightSimulator2024.exe"));
    QCOMPARE(targets[1].label, QStringLiteral("MSFS 2020 (Steam)"));
    QCOMPARE(targets[1].path, tree.userHome2020 + QStringLiteral("/AppData/Roaming/")
                                  + QLatin1String(FakeProtonTree::kFolder2020) + QStringLiteral("/SimConnect.xml"));
    QCOMPARE(targets[1].processName, QStringLiteral("FlightSimulator.exe"));
}

QTEST_APPLESS_MAIN(SimConnectXmlTest)

#include "tst_simconnect_xml.moc"
