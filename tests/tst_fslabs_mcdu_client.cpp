#include <QtTest/QTest>

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "FslabsFixtures.h"
#include "ProbeLines.h"
#include "../src/infrastructure/fslabs/FslabsMcduClient.h"

namespace
{
    constexpr auto kWritesLog = "writes.log";
    constexpr auto kHeaderEnd = "\r\n\r\n";
    constexpr auto kDisplayPrefix = "/MCDU/Display/";
    constexpr auto kDisplayPath = "/MCDU/Display/3CA2";
    constexpr auto kKeyPath = "/MCDU/Key/3CA2?Key=114";
    constexpr auto kCaptainsMcdu = "3CA1";
    constexpr int kKeyPressed = 114;
    constexpr int kFlightNumberRow = 2;

    class FakeMcduServer
    {
    public:
        bool Listen()
        {
            QObject::connect(&server_, &QTcpServer::newConnection, &server_, [this] { Accept(); });

            return server_.listen(QHostAddress::LocalHost, 0);
        }

        void Close() { server_.close(); }

        [[nodiscard]] QString Endpoint() const
        {
            return QStringLiteral("http://127.0.0.1:%1").arg(server_.serverPort());
        }

        [[nodiscard]] const QStringList& Paths() const { return paths_; }
        [[nodiscard]] int Requests() const { return static_cast<int>(paths_.size()); }

        void ServeDisplay(const QByteArray& body) { displayBody_ = body; }
        void FailEveryRequest() { failing_ = true; }
        void StopFailing() { failing_ = false; }
        void HoldDisplayReplies() { holding_ = true; }

        void ReleaseDisplayReplies()
        {
            holding_ = false;
            for (QTcpSocket* const socket : held_)
            {
                Answer(*socket, displayBody_);
            }

            held_.clear();
        }

    private:
        void Accept()
        {
            while (QTcpSocket* const socket = server_.nextPendingConnection())
            {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { Serve(*socket); });
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        }

        void Serve(QTcpSocket& socket)
        {
            QByteArray& pending = pending_[&socket];
            pending += socket.readAll();

            const qsizetype headerEnd = pending.indexOf(kHeaderEnd);
            if (headerEnd < 0)
            {
                return;
            }

            const QString path = QString::fromLatin1(pending.left(headerEnd).split(' ').value(1));
            pending.clear();
            paths_.append(path);

            if (failing_)
            {
                socket.write("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                socket.disconnectFromHost();

                return;
            }

            if (!path.startsWith(QLatin1String(kDisplayPrefix)))
            {
                Answer(socket, QByteArray());

                return;
            }

            if (holding_)
            {
                held_.push_back(&socket);

                return;
            }

            Answer(socket, displayBody_);
        }

        static void Answer(QTcpSocket& socket, const QByteArray& body)
        {
            socket.write(QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ")
                         + QByteArray::number(body.size()) + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + body);
            socket.disconnectFromHost();
        }

        QTcpServer server_;
        std::map<QTcpSocket*, QByteArray> pending_;
        std::vector<QTcpSocket*> held_;
        QStringList paths_;
        QByteArray displayBody_ = FslabsFixtureBody("aoc-init");
        bool failing_ = false;
        bool holding_ = false;
    };

    QByteArray BodyWithCellCount(const int cells)
    {
        QByteArray body = R"({"Type":"Json","Value":[)";
        for (int cell = 0; cell < cells; ++cell)
        {
            body += cell == 0 ? "[]" : ",[]";
        }

        return body + "]}";
    }
}

class FslabsMcduClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    static void startsUnavailableWithABlankScreen();
    static void parsesAMeasuredFrameCellByCell();
    static void aBlankFrameParsesAsBlank();
    static void rejectsMalformedBodies();
    static void rejectsAFrameWithTheWrongCellCount();
    static void rejectsACellThatIsNotAGlyph();
    static void pollsTheRightMcduDisplay();
    static void aKeyGoesToTheRightMcduOnly();
    static void aKeyPressLandsInTheWritesLog();
    static void onlyOneDisplayRequestIsInFlight();
    static void aScreenPolledBeforeAKeyIsNotFresh();
    static void aScreenPolledAfterAKeyIsFresh();
    static void oneFailureKeepsTheClientAvailableAndTwoDropIt();
    static void aSuccessBetweenTwoFailuresKeepsTheClientAvailable();
    static void aClosedServerMakesTheClientUnavailable();
    static void aGarbledAnswerCountsAsAFailure();

private:
    QTemporaryDir directory_;
};

void FslabsMcduClientTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void FslabsMcduClientTest::startsUnavailableWithABlankScreen()
{
    const FslabsMcduClient client;

    QVERIFY(!client.IsAvailable());
    QVERIFY(client.Screen().IsBlank());
    QCOMPARE(client.ScreenSerial(), std::uint64_t{0});
}

void FslabsMcduClientTest::parsesAMeasuredFrameCellByCell()
{
    const std::optional<FslabsMcduScreen> init = FslabsMcduClient::ParseDisplayResponse(FslabsFixtureBody("aoc-init"));
    const std::optional<FslabsMcduScreen> boarding = FslabsMcduClient::ParseDisplayResponse(FslabsFixtureBody("boarding-2"));

    QVERIFY(init.has_value());
    QCOMPARE(init->Row(0), std::string("AOC   INIT/REVIEW    1/2"));
    QCOMPARE(init->Row(kFlightNumberRow), std::string("LB1210              1646"));
    QCOMPARE(init->GlyphAt(kFlightNumberRow, 0), int{'L'});
    QCOMPARE(init->GlyphAt(kFlightNumberRow, 6), 0);
    QVERIFY(boarding.has_value());
    QCOMPARE(boarding->GlyphAt(6, 0), 31);
    QCOMPARE(boarding->Row(6), std::string(" BOARD NOW     CATERING_"));
}

void FslabsMcduClientTest::aBlankFrameParsesAsBlank()
{
    const std::optional<FslabsMcduScreen> screen = FslabsMcduClient::ParseDisplayResponse(FslabsFixtureBody("blank"));

    QVERIFY(screen.has_value());
    QVERIFY(screen->IsBlank());
    QVERIFY(!FslabsFixtureScreen("mcdu-menu").IsBlank());
}

void FslabsMcduClientTest::rejectsMalformedBodies()
{
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse("<html>502 Bad Gateway</html>").has_value());
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse({}).has_value());
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse(R"({"Type":"Json"})").has_value());
}

void FslabsMcduClientTest::rejectsAFrameWithTheWrongCellCount()
{
    QVERIFY(FslabsMcduClient::ParseDisplayResponse(BodyWithCellCount(FslabsMcduScreen::kCells)).has_value());
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse(BodyWithCellCount(FslabsMcduScreen::kCells - 1)).has_value());
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse(BodyWithCellCount(FslabsMcduScreen::kCells + 1)).has_value());
}

void FslabsMcduClientTest::rejectsACellThatIsNotAGlyph()
{
    QByteArray text = BodyWithCellCount(FslabsMcduScreen::kCells);
    text.replace("[]", R"("A")");
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse(text).has_value());

    QByteArray outOfRange = BodyWithCellCount(FslabsMcduScreen::kCells);
    outOfRange.replace(QByteArrayLiteral("[]"), QByteArrayLiteral("[999,7,0]"));
    QVERIFY(!FslabsMcduClient::ParseDisplayResponse(outOfRange).has_value());
}

void FslabsMcduClientTest::pollsTheRightMcduDisplay()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Poll();

    QTRY_VERIFY(client.IsAvailable());
    QCOMPARE(server.Paths(), QStringList{QString::fromLatin1(kDisplayPath)});
    QCOMPARE(client.Screen().Row(0), std::string("AOC   INIT/REVIEW    1/2"));
    QCOMPARE(client.ScreenSerial(), std::uint64_t{1});
}

void FslabsMcduClientTest::aKeyGoesToTheRightMcduOnly()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());

    client.Press(kKeyPressed);

    QTRY_COMPARE(server.Requests(), 2);
    QCOMPARE(server.Paths().last(), QString::fromLatin1(kKeyPath));
    for (const QString& path : server.Paths())
    {
        QVERIFY2(!path.contains(QLatin1String(kCaptainsMcdu)), qPrintable(path));
    }
}

void FslabsMcduClientTest::aKeyPressLandsInTheWritesLog()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    const qsizetype before = ProbeLines(kWritesLog).size();

    client.Press(kKeyPressed);
    client.Press(kKeyPressed);

    QCOMPARE(ProbeLines(kWritesLog).mid(before),
             (QStringList{QStringLiteral("mcdu 3CA2 key=114"), QStringLiteral("mcdu 3CA2 key=114")}));
    QTRY_COMPARE(server.Requests(), 2);
}

void FslabsMcduClientTest::onlyOneDisplayRequestIsInFlight()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    server.HoldDisplayReplies();
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Poll();
    client.Poll();
    client.Poll();

    QTRY_COMPARE(server.Requests(), 1);
    QTest::qWait(100);
    QCOMPARE(server.Requests(), 1);

    server.ReleaseDisplayReplies();
    QTRY_VERIFY(client.IsAvailable());

    client.Poll();

    QTRY_COMPARE(server.Requests(), 2);
}

void FslabsMcduClientTest::aScreenPolledBeforeAKeyIsNotFresh()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    server.HoldDisplayReplies();
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_COMPARE(server.Requests(), 1);

    client.Press(kKeyPressed);
    QTRY_COMPARE(server.Requests(), 2);
    client.Poll();
    server.ReleaseDisplayReplies();

    QTRY_VERIFY(client.IsAvailable());
    QCOMPARE(client.Screen().Row(0), std::string("AOC   INIT/REVIEW    1/2"));
    QCOMPARE(client.ScreenSerial(), std::uint64_t{0});
}

void FslabsMcduClientTest::aScreenPolledAfterAKeyIsFresh()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Press(kKeyPressed);
    QTRY_COMPARE(server.Requests(), 1);
    client.Poll();

    QTRY_VERIFY(client.IsAvailable());
    QCOMPARE(client.ScreenSerial(), std::uint64_t{1});
}

void FslabsMcduClientTest::oneFailureKeepsTheClientAvailableAndTwoDropIt()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.FailEveryRequest();

    client.Poll();
    QTRY_COMPARE(server.Requests(), 2);
    QTest::qWait(150);

    QVERIFY(client.IsAvailable());

    client.Poll();
    QTRY_COMPARE(server.Requests(), 3);

    QTRY_VERIFY(!client.IsAvailable());
}

void FslabsMcduClientTest::aSuccessBetweenTwoFailuresKeepsTheClientAvailable()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());

    server.FailEveryRequest();
    client.Poll();
    QTRY_COMPARE(server.Requests(), 2);
    QTest::qWait(150);

    server.StopFailing();
    client.Poll();
    QTRY_COMPARE(server.Requests(), 3);
    QTest::qWait(150);

    server.FailEveryRequest();
    client.Poll();
    QTRY_COMPARE(server.Requests(), 4);
    QTest::qWait(150);

    QVERIFY(client.IsAvailable());
}

void FslabsMcduClientTest::aClosedServerMakesTheClientUnavailable()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.Close();

    QTRY_VERIFY_WITH_TIMEOUT(([&client] { client.Poll(); return !client.IsAvailable(); }()), 5000);
}

void FslabsMcduClientTest::aGarbledAnswerCountsAsAFailure()
{
    FakeMcduServer server;
    QVERIFY(server.Listen());
    FslabsMcduClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.ServeDisplay("<html>not a frame</html>");

    QTRY_VERIFY_WITH_TIMEOUT(([&client] { client.Poll(); return !client.IsAvailable(); }()), 5000);
    QVERIFY(client.Screen().Row(0).find("INIT/REVIEW") != std::string::npos);
}

QTEST_GUILESS_MAIN(FslabsMcduClientTest)

#include "tst_fslabs_mcdu_client.moc"
