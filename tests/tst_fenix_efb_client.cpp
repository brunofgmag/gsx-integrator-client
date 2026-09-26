#include <QtTest/QTest>

#include <cstring>
#include <map>
#include <QtCore/QByteArray>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "ProbeLines.h"
#include "../src/infrastructure/fenix/FenixEfbClient.h"

namespace
{
    constexpr auto kWritesLog = "writes.log";
    constexpr auto kReadyDataref = "fenix.test.ready";
    constexpr auto kFuelTargetDataref = "aircraft.refuel.fuelTarget.kg";
    constexpr auto kChocksDataref = "fenix.efb.chocks";
    constexpr auto kSeatOccupationDataref = "aircraft.passengers.seatOccupation.string";
    constexpr auto kHeaderEnd = "\r\n\r\n";
    constexpr auto kContentLength = "content-length:";
    constexpr auto kReadyReply = R"({"data":{"dataRef":{"fenixtestready":{"value":true,"__typename":"DataRef"}}}})";

    qsizetype ContentLength(const QByteArray& headers)
    {
        const QList<QByteArray> lines = headers.split('\n');
        for (const QByteArray& line : lines)
        {
            const QByteArray field = line.trimmed().toLower();
            if (field.startsWith(kContentLength))
            {
                return field.mid(static_cast<qsizetype>(std::strlen(kContentLength))).trimmed().toLongLong();
            }
        }

        return 0;
    }

    QByteArray Reply()
    {
        const QByteArray body(kReadyReply);

        return QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ")
            + QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body;
    }

    class FakeFenixEfb
    {
    public:
        bool Listen()
        {
            QObject::connect(&server_, &QTcpServer::newConnection, &server_, [this] { Accept(); });

            return server_.listen(QHostAddress::LocalHost, 0);
        }

        [[nodiscard]] QString Endpoint() const
        {
            return QStringLiteral("http://127.0.0.1:%1/graphql").arg(server_.serverPort());
        }

        [[nodiscard]] int Requests() const
        {
            return requests_;
        }

    private:
        void Accept()
        {
            while (QTcpSocket* socket = server_.nextPendingConnection())
            {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { Serve(*socket); });
            }
        }

        void Serve(QTcpSocket& socket)
        {
            QByteArray& pending = pending_[&socket];
            pending += socket.readAll();

            for (qsizetype headerEnd = pending.indexOf(kHeaderEnd); headerEnd >= 0;
                 headerEnd = pending.indexOf(kHeaderEnd))
            {
                const qsizetype total = headerEnd + static_cast<qsizetype>(std::strlen(kHeaderEnd))
                    + ContentLength(pending.left(headerEnd));
                if (pending.size() < total)
                {
                    return;
                }

                pending.remove(0, total);
                ++requests_;
                socket.write(Reply());
            }
        }

        QTcpServer server_;
        std::map<QTcpSocket*, QByteArray> pending_;
        int requests_ = 0;
    };
}

class FenixEfbClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    static void startsUnavailable();
    static void valuesQueryNestsAliasedDataRefs();
    static void parsesNumericAndBooleanValues();
    static void parsesBoolArrayValues();
    static void skipsNullValues();
    static void rejectsResponseWithoutDataRefs();
    static void rejectsMalformedJson();
    static void writeMutationUsesVariables();
    static void writeMutationQuotesStrings();
    static void readsCoerceNumbersAndBools();
    static void aSentWriteLandsInTheWritesLog();
    static void aWriteWhileTheEfbIsUnreachableIsNotLogged();

private:
    QTemporaryDir directory_;
};

void FenixEfbClientTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void FenixEfbClientTest::startsUnavailable()
{
    const FenixEfbClient client;

    QVERIFY(!client.IsAvailable());
}

void FenixEfbClientTest::valuesQueryNestsAliasedDataRefs()
{
    const QByteArray query = FenixEfbClient::BuildValuesQuery(
        {"fenix.efb.simbriefPlanImported", "fenix.efb.passengers.booked"});

    QVERIFY(query.contains("{ dataRef { "));
    QVERIFY(query.contains("fenixefbsimbriefPlanImported: dataRef(name: \\\"fenix.efb.simbriefPlanImported\\\")"));
    QVERIFY(query.contains("fenixefbpassengersbooked: dataRef(name: \\\"fenix.efb.passengers.booked\\\")"));
    QVERIFY(query.contains("{ value __typename }"));
    QVERIFY(query.contains("\"variables\":{}"));
}

void FenixEfbClientTest::parsesNumericAndBooleanValues()
{
    const auto values = FenixEfbClient::ParseValuesResponse(
        R"({"data":{"dataRef":{"aircraftfueltotalamountkg":{"value":8450.5,"__typename":"DataRef"},)"
        R"("fenixefbsimbriefPlanImported":{"value":true,"__typename":"DataRef"},)"
        R"("__typename":"DataReferencesQuery"}}})",
        {"aircraft.fuel.total.amount.kg", "fenix.efb.simbriefPlanImported"});

    QVERIFY(values.has_value());
    QCOMPARE(values->at("aircraft.fuel.total.amount.kg").toDouble(), 8450.5);
    QCOMPARE(values->at("fenix.efb.simbriefPlanImported").toBool(), true);
}

void FenixEfbClientTest::parsesBoolArrayValues()
{
    const auto values = FenixEfbClient::ParseValuesResponse(
        R"({"data":{"dataRef":{"fenixefbpassengersbooked":{"value":[true,false,true],"__typename":"DataRef"}}}})",
        {"fenix.efb.passengers.booked"});

    QVERIFY(values.has_value());
    QVERIFY(values->at("fenix.efb.passengers.booked").isArray());
    QCOMPARE(values->at("fenix.efb.passengers.booked").toArray().size(), 3);
}

void FenixEfbClientTest::skipsNullValues()
{
    const auto values = FenixEfbClient::ParseValuesResponse(
        R"({"data":{"dataRef":{"fenixefbsimbriefPlanImported":{"value":null,"__typename":"DataRef"},)"
        R"("__typename":"DataReferencesQuery"}}})",
        {"fenix.efb.simbriefPlanImported"});

    QVERIFY(values.has_value());
    QVERIFY(!values->contains("fenix.efb.simbriefPlanImported"));
}

void FenixEfbClientTest::rejectsResponseWithoutDataRefs()
{
    QVERIFY(!FenixEfbClient::ParseValuesResponse(R"({"data":{}})", {"any.ref"}).has_value());
    QVERIFY(!FenixEfbClient::ParseValuesResponse(
        R"({"errors":[{"message":"unknown"}]})", {"any.ref"}).has_value());
}

void FenixEfbClientTest::rejectsMalformedJson()
{
    QVERIFY(!FenixEfbClient::ParseValuesResponse("<html>502 Bad Gateway</html>", {"any.ref"}).has_value());
    QVERIFY(!FenixEfbClient::ParseValuesResponse({}, {"any.ref"}).has_value());
}

void FenixEfbClientTest::writeMutationUsesVariables()
{
    const QByteArray mutation = FenixEfbClient::BuildWriteMutation(
        "writeBool", "Boolean", "fenix.efb.autoDoor", QJsonValue(false));

    QVERIFY(mutation.contains("mutation ($fenixefbautoDoor: Boolean!)"));
    QVERIFY(mutation.contains("writeBool(name: \\\"fenix.efb.autoDoor\\\", value: $fenixefbautoDoor)"));
    QVERIFY(mutation.contains("\"variables\":{\"fenixefbautoDoor\":false}"));
}

void FenixEfbClientTest::writeMutationQuotesStrings()
{
    const QByteArray mutation = FenixEfbClient::BuildWriteMutation(
        "writeString", "String", "aircraft.passengers.seatOccupation.string",
        QJsonValue(QStringLiteral("true,false")));

    QVERIFY(mutation.contains("mutation ($aircraftpassengersseatOccupationstring: String!)"));
    QVERIFY(mutation.contains("\"variables\":{\"aircraftpassengersseatOccupationstring\":\"true,false\"}"));
}

void FenixEfbClientTest::readsCoerceNumbersAndBools()
{
    FenixEfbClient client;
    client.Subscribe("some.number");

    QCOMPARE(client.GetNumber("some.number", 42.0), 42.0);
    QVERIFY(client.GetBoolArray("some.number").empty());
}

void FenixEfbClientTest::aSentWriteLandsInTheWritesLog()
{
#ifndef NDEBUG
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.Subscribe(kReadyDataref);
    client.Poll();
    QTRY_VERIFY(client.IsAvailable());
    const qsizetype before = ProbeLines(kWritesLog).size();

    client.SetFloat(kFuelTargetDataref, 5000.0);
    client.SetFloat(kFuelTargetDataref, 5000.0);
    client.SetFloat(kFuelTargetDataref, 5200.5);
    client.SetBool(kChocksDataref, false);
    client.SetString(kSeatOccupationDataref, "true,false");

    QCOMPARE(ProbeLines(kWritesLog).mid(before),
             (QStringList{
                 QStringLiteral("graphql aircraft.refuel.fuelTarget.kg=5000 n=1"),
                 QStringLiteral("graphql aircraft.refuel.fuelTarget.kg=5200.5 n=3"),
                 QStringLiteral("graphql fenix.efb.chocks=false n=1"),
                 QStringLiteral("graphql aircraft.passengers.seatOccupation.string=\"true,false\" n=1")
             }));
    QTRY_COMPARE(efb.Requests(), 6);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void FenixEfbClientTest::aWriteWhileTheEfbIsUnreachableIsNotLogged()
{
#ifndef NDEBUG
    const qsizetype before = ProbeLines(kWritesLog).size();
    FenixEfbClient client;

    client.SetFloat(kFuelTargetDataref, 5000.0);

    QVERIFY(!client.IsAvailable());
    QCOMPARE(ProbeLines(kWritesLog).size(), before);
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(FenixEfbClientTest)

#include "tst_fenix_efb_client.moc"
