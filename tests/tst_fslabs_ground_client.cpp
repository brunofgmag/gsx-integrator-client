#include <QtTest/QTest>

#include <map>
#include <optional>
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
#include "../src/infrastructure/fslabs/FslabsGroundClient.h"

namespace
{
    constexpr auto kWritesLog = "writes.log";
    constexpr auto kHeaderEnd = "\r\n\r\n";
    constexpr auto kGroundPath = "/Maintenance/Ground_Get";
    constexpr auto kForwardTogglePath = "/EFB-API/SET-PAGE-GROUND?RH_FWD_CARGO=1";
    constexpr auto kAftTogglePath = "/EFB-API/SET-PAGE-GROUND?RH_AFT_CARGO=1";
    constexpr auto kClosedHolds = "ground-holds-closed";
    constexpr auto kOpenHolds = "ground-holds-open";

    QByteArray BodyWith(const QByteArray& from, const QByteArray& to, const char* fixture = kClosedHolds)
    {
        QByteArray body = FslabsFixtureBody(fixture);
        body.replace(from, to);

        return body;
    }

    class FakeGroundServer
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

        void ServeGround(const QByteArray& body) { groundBody_ = body; }
        void FailEveryRequest() { failing_ = true; }
        void StopFailing() { failing_ = false; }
        void HoldGroundReplies() { holding_ = true; }

        void ReleaseGroundReplies()
        {
            holding_ = false;
            for (QTcpSocket* const socket : held_)
            {
                Answer(*socket, groundBody_);
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

            if (path != QLatin1String(kGroundPath))
            {
                Answer(socket, QByteArrayLiteral(R"({"Type":"Json","Value":{}})"));

                return;
            }

            if (holding_)
            {
                held_.push_back(&socket);

                return;
            }

            Answer(socket, groundBody_);
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
        QByteArray groundBody_ = FslabsFixtureBody(kClosedHolds);
        bool failing_ = false;
        bool holding_ = false;
    };
}

class FslabsGroundClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    static void startsUnavailableWithNothingRead();
    static void parsesTheMeasuredBodyWithBothHoldsClosed();
    static void parsesTheMeasuredBodyWithBothHoldsOpen();
    static void parsesEveryHoldState();
    static void rejectsMalformedBodies();
    static void rejectsABodyMissingAField();
    static void rejectsAHoldThatIsNotAState();
    static void pollsTheGroundEndpoint();
    static void anOpenReadingReachesTheHoldStates();
    static void aForwardToggleGoesToTheCommandEndpoint();
    static void anAftToggleGoesToTheCommandEndpoint();
    static void aToggleLandsInTheWritesLog();
    static void onlyOneGroundRequestIsInFlight();
    static void oneFailureKeepsTheClientAvailableAndTwoDropIt();
    static void aSuccessBetweenTwoFailuresKeepsTheClientAvailable();
    static void aClosedServerMakesTheClientUnavailable();
    static void aGarbledAnswerCountsAsAFailure();
    static void theLastReadingSurvivesAFailedPoll();

private:
    QTemporaryDir directory_;
};

void FslabsGroundClientTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void FslabsGroundClientTest::startsUnavailableWithNothingRead()
{
    const FslabsGroundClient client;

    QVERIFY(!client.IsAvailable());
    QVERIFY(!client.HoldState(FslabsHold::Forward).has_value());
    QVERIFY(!client.HoldState(FslabsHold::Aft).has_value());
}

void FslabsGroundClientTest::parsesTheMeasuredBodyWithBothHoldsClosed()
{
    const std::optional<FslabsGroundReading> reading =
        FslabsGroundClient::ParseGroundResponse(FslabsFixtureBody(kClosedHolds));

    QVERIFY(reading.has_value());
    QCOMPARE(reading->forwardHold, FslabsHoldState::Closed);
    QCOMPARE(reading->aftHold, FslabsHoldState::Closed);
}

void FslabsGroundClientTest::parsesTheMeasuredBodyWithBothHoldsOpen()
{
    const std::optional<FslabsGroundReading> reading =
        FslabsGroundClient::ParseGroundResponse(FslabsFixtureBody(kOpenHolds));

    QVERIFY(reading.has_value());
    QCOMPARE(reading->forwardHold, FslabsHoldState::Open);
    QCOMPARE(reading->aftHold, FslabsHoldState::Open);
}

void FslabsGroundClientTest::parsesEveryHoldState()
{
    const std::optional<FslabsGroundReading> opening =
        FslabsGroundClient::ParseGroundResponse(BodyWith(R"("RH_FWD_CARGO":0)", R"("RH_FWD_CARGO":1)"));
    const std::optional<FslabsGroundReading> closing =
        FslabsGroundClient::ParseGroundResponse(BodyWith(R"("RH_AFT_CARGO":0)", R"("RH_AFT_CARGO":3)"));

    QVERIFY(opening.has_value());
    QCOMPARE(opening->forwardHold, FslabsHoldState::Opening);
    QCOMPARE(opening->aftHold, FslabsHoldState::Closed);
    QVERIFY(closing.has_value());
    QCOMPARE(closing->forwardHold, FslabsHoldState::Closed);
    QCOMPARE(closing->aftHold, FslabsHoldState::Closing);
}

void FslabsGroundClientTest::rejectsMalformedBodies()
{
    QVERIFY(!FslabsGroundClient::ParseGroundResponse("<html>502 Bad Gateway</html>").has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse({}).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(R"({"Type":"Json"})").has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(R"({"Type":"Json","Value":{}})").has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(R"({"Type":"Json","Value":[]})").has_value());
}

void FslabsGroundClientTest::rejectsABodyMissingAField()
{
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(BodyWith(R"("RH_FWD_CARGO")", R"("OTHER")")).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(BodyWith(R"("RH_AFT_CARGO")", R"("OTHER")")).has_value());
}

void FslabsGroundClientTest::rejectsAHoldThatIsNotAState()
{
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(
        BodyWith(R"("RH_FWD_CARGO":0)", R"("RH_FWD_CARGO":4)")).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(
        BodyWith(R"("RH_AFT_CARGO":0)", R"("RH_AFT_CARGO":-1)")).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(
        BodyWith(R"("RH_FWD_CARGO":0)", R"("RH_FWD_CARGO":"2")")).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(
        BodyWith(R"("RH_AFT_CARGO":0)", R"("RH_AFT_CARGO":null)")).has_value());
    QVERIFY(!FslabsGroundClient::ParseGroundResponse(
        BodyWith(R"("RH_FWD_CARGO":0)", R"("RH_FWD_CARGO":1.5)")).has_value());
}

void FslabsGroundClientTest::pollsTheGroundEndpoint()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Poll();

    QTRY_VERIFY(client.IsAvailable());
    QCOMPARE(server.Paths(), QStringList{QString::fromLatin1(kGroundPath)});
    QCOMPARE(client.HoldState(FslabsHold::Forward), std::optional{FslabsHoldState::Closed});
    QCOMPARE(client.HoldState(FslabsHold::Aft), std::optional{FslabsHoldState::Closed});
}

void FslabsGroundClientTest::anOpenReadingReachesTheHoldStates()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    server.ServeGround(BodyWith(R"("RH_AFT_CARGO":2)", R"("RH_AFT_CARGO":3)", kOpenHolds));
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Poll();

    QTRY_VERIFY(client.IsAvailable());
    QCOMPARE(client.HoldState(FslabsHold::Forward), std::optional{FslabsHoldState::Open});
    QCOMPARE(client.HoldState(FslabsHold::Aft), std::optional{FslabsHoldState::Closing});
}

void FslabsGroundClientTest::aForwardToggleGoesToTheCommandEndpoint()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.ToggleHold(FslabsHold::Forward);

    QTRY_COMPARE(server.Requests(), 1);
    QCOMPARE(server.Paths(), QStringList{QString::fromLatin1(kForwardTogglePath)});
}

void FslabsGroundClientTest::anAftToggleGoesToTheCommandEndpoint()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.ToggleHold(FslabsHold::Aft);

    QTRY_COMPARE(server.Requests(), 1);
    QCOMPARE(server.Paths(), QStringList{QString::fromLatin1(kAftTogglePath)});
}

void FslabsGroundClientTest::aToggleLandsInTheWritesLog()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());
    const qsizetype before = ProbeLines(kWritesLog).size();

    client.ToggleHold(FslabsHold::Forward);
    client.ToggleHold(FslabsHold::Aft);

    QCOMPARE(ProbeLines(kWritesLog).mid(before),
             (QStringList{QStringLiteral("ground RH_FWD_CARGO toggle"), QStringLiteral("ground RH_AFT_CARGO toggle")}));
    QTRY_COMPARE(server.Requests(), 2);
}

void FslabsGroundClientTest::onlyOneGroundRequestIsInFlight()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    server.HoldGroundReplies();
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());

    client.Poll();
    client.Poll();
    client.Poll();

    QTRY_COMPARE(server.Requests(), 1);
    QTest::qWait(100);
    QCOMPARE(server.Requests(), 1);

    server.ReleaseGroundReplies();
    QTRY_VERIFY(client.IsAvailable());

    client.Poll();

    QTRY_COMPARE(server.Requests(), 2);
}

void FslabsGroundClientTest::oneFailureKeepsTheClientAvailableAndTwoDropIt()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
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

void FslabsGroundClientTest::aSuccessBetweenTwoFailuresKeepsTheClientAvailable()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
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

void FslabsGroundClientTest::aClosedServerMakesTheClientUnavailable()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.Close();

    QTRY_VERIFY_WITH_TIMEOUT(([&client] { client.Poll(); return !client.IsAvailable(); }()), 5000);
}

void FslabsGroundClientTest::aGarbledAnswerCountsAsAFailure()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.ServeGround("<html>not a ground reading</html>");

    QTRY_VERIFY_WITH_TIMEOUT(([&client] { client.Poll(); return !client.IsAvailable(); }()), 5000);
}

void FslabsGroundClientTest::theLastReadingSurvivesAFailedPoll()
{
    FakeGroundServer server;
    QVERIFY(server.Listen());
    server.ServeGround(FslabsFixtureBody(kOpenHolds));
    FslabsGroundClient client;
    client.SetEndpointForTest(server.Endpoint());
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    server.FailEveryRequest();

    QTRY_VERIFY_WITH_TIMEOUT(([&client] { client.Poll(); return !client.IsAvailable(); }()), 5000);

    QCOMPARE(client.HoldState(FslabsHold::Forward), std::optional{FslabsHoldState::Open});
}

QTEST_GUILESS_MAIN(FslabsGroundClientTest)

#include "tst_fslabs_ground_client.moc"
