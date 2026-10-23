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
    constexpr auto kWeightUnitSetting = "weight_unit";
    constexpr auto kSettingsRequestMarker = "softwareVersion";
    constexpr auto kSettingsReply =
        R"({"data":{"settings":{"softwareVersion":"3.0.0.7438","state":{"settings":[)"
        R"({"key":"weight_unit","value":"Pounds"},)"
        R"({"key":"navigraph_refresh_token","value":"not-a-real-token"}]}}}})";
    constexpr auto kSchemaErrorReply = R"({"errors":[{"message":"Cannot query field settings on type Query."}]})";

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

    QByteArray Reply(const QByteArray& body)
    {
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

        [[nodiscard]] int SettingsRequests() const
        {
            return settingsRequests_;
        }

        void ReplyToDataRefsWith(const QByteArray& body)
        {
            dataRefBody_ = body;
        }

        void ReplyToSettingsWith(const QByteArray& body)
        {
            settingsBody_ = body;
        }

        void HoldSettingsReplies()
        {
            holdSettings_ = true;
        }

        void HoldDataRefReplies()
        {
            holdDataRefs_ = true;
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

                const qsizetype bodyStart = headerEnd + static_cast<qsizetype>(std::strlen(kHeaderEnd));
                const QByteArray request = pending.mid(bodyStart, total - bodyStart);
                pending.remove(0, total);
                ++requests_;
                Answer(socket, request);
            }
        }

        void Answer(QTcpSocket& socket, const QByteArray& request)
        {
            if (!request.contains(kSettingsRequestMarker))
            {
                if (!holdDataRefs_)
                {
                    socket.write(Reply(dataRefBody_));
                }

                return;
            }

            ++settingsRequests_;
            if (!holdSettings_)
            {
                socket.write(Reply(settingsBody_));
            }
        }

        QTcpServer server_;
        std::map<QTcpSocket*, QByteArray> pending_;
        QByteArray dataRefBody_ = kReadyReply;
        QByteArray settingsBody_ = kSettingsReply;
        bool holdSettings_ = false;
        bool holdDataRefs_ = false;
        int requests_ = 0;
        int settingsRequests_ = 0;
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
    static void settingsQueryAsksForTheVersionAndTheStateSettings();
    static void parsesTheMajorVersionFromTheSoftwareVersion();
    static void rejectsASoftwareVersionWithoutAMajorNumber();
    static void keepsOnlyTheSubscribedSettings();
    static void rejectsASettingsResponseWithoutSettings();
    static void learnsTheVersionAndTheSubscribedSettingFromTheEfb();
    static void aSettingsAnswerNeverMakesTheClientAvailable();
    static void aFailedSettingsAnswerKeepsTheVersionAndTheSettings();
    static void settingsAreRequestedOnTheFirstPollAndEveryTenthAfter();
    static void noSettingsRequestGoesOutWhileAnotherIsInFlight();

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
}

void FenixEfbClientTest::aWriteWhileTheEfbIsUnreachableIsNotLogged()
{
    const qsizetype before = ProbeLines(kWritesLog).size();
    FenixEfbClient client;

    client.SetFloat(kFuelTargetDataref, 5000.0);

    QVERIFY(!client.IsAvailable());
    QCOMPARE(ProbeLines(kWritesLog).size(), before);
}

void FenixEfbClientTest::settingsQueryAsksForTheVersionAndTheStateSettings()
{
    const QByteArray query = FenixEfbClient::BuildSettingsQuery();

    QVERIFY(query.contains("{ settings { softwareVersion state { settings { key value } } } }"));
    QVERIFY(!query.contains("dataRef"));
    QVERIFY(query.contains("\"variables\":{}"));
}

void FenixEfbClientTest::parsesTheMajorVersionFromTheSoftwareVersion()
{
    const auto fenix3 = FenixEfbClient::ParseSettingsResponse(kSettingsReply, {kWeightUnitSetting});
    const auto fenix2 = FenixEfbClient::ParseSettingsResponse(
        R"({"data":{"settings":{"softwareVersion":"2.1.0.5120","state":{"settings":[]}}}})", {kWeightUnitSetting});

    QVERIFY(fenix3.has_value());
    QCOMPARE(fenix3->softwareMajorVersion, 3);
    QVERIFY(fenix2.has_value());
    QCOMPARE(fenix2->softwareMajorVersion, 2);
}

void FenixEfbClientTest::rejectsASoftwareVersionWithoutAMajorNumber()
{
    QVERIFY(!FenixEfbClient::ParseSettingsResponse(
        R"({"data":{"settings":{"softwareVersion":"beta.3","state":{"settings":[]}}}})",
        {kWeightUnitSetting}).has_value());
    QVERIFY(!FenixEfbClient::ParseSettingsResponse(
        R"({"data":{"settings":{"state":{"settings":[]}}}})", {kWeightUnitSetting}).has_value());
}

void FenixEfbClientTest::keepsOnlyTheSubscribedSettings()
{
    const auto snapshot = FenixEfbClient::ParseSettingsResponse(kSettingsReply, {kWeightUnitSetting});

    QVERIFY(snapshot.has_value());
    QCOMPARE(snapshot->values.size(), std::size_t{1});
    QCOMPARE(snapshot->values.at(kWeightUnitSetting), std::string("Pounds"));
    QVERIFY(!snapshot->values.contains("navigraph_refresh_token"));

    const auto none = FenixEfbClient::ParseSettingsResponse(kSettingsReply, {});

    QVERIFY(none.has_value());
    QVERIFY(none->values.empty());
}

void FenixEfbClientTest::rejectsASettingsResponseWithoutSettings()
{
    QVERIFY(!FenixEfbClient::ParseSettingsResponse(kSchemaErrorReply, {kWeightUnitSetting}).has_value());
    QVERIFY(!FenixEfbClient::ParseSettingsResponse(R"({"data":{}})", {kWeightUnitSetting}).has_value());
    QVERIFY(!FenixEfbClient::ParseSettingsResponse("<html>502 Bad Gateway</html>", {kWeightUnitSetting}).has_value());
    QVERIFY(!FenixEfbClient::ParseSettingsResponse({}, {kWeightUnitSetting}).has_value());
}

void FenixEfbClientTest::learnsTheVersionAndTheSubscribedSettingFromTheEfb()
{
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.SubscribeSetting(kWeightUnitSetting);

    QVERIFY(!client.GetSoftwareMajorVersion().has_value());
    QCOMPARE(client.GetSetting(kWeightUnitSetting, "unset"), std::string("unset"));

    client.Poll();

    QTRY_VERIFY(client.GetSoftwareMajorVersion() == 3);
    QCOMPARE(client.GetSetting(kWeightUnitSetting, "unset"), std::string("Pounds"));
    QCOMPARE(client.GetSetting("navigraph_refresh_token", "unset"), std::string("unset"));
}

void FenixEfbClientTest::aSettingsAnswerNeverMakesTheClientAvailable()
{
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    efb.ReplyToDataRefsWith(kSchemaErrorReply);
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.Subscribe(kReadyDataref);
    client.SubscribeSetting(kWeightUnitSetting);

    client.Poll();

    QTRY_VERIFY(client.GetSoftwareMajorVersion() == 3);
    QTRY_COMPARE(efb.Requests(), 2);
    QVERIFY(!client.IsAvailable());
}

void FenixEfbClientTest::aFailedSettingsAnswerKeepsTheVersionAndTheSettings()
{
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.Subscribe(kReadyDataref);
    client.SubscribeSetting(kWeightUnitSetting);
    client.Poll();
    QTRY_VERIFY(client.GetSoftwareMajorVersion() == 3);
    QTRY_VERIFY(client.IsAvailable());

    efb.HoldDataRefReplies();
    efb.ReplyToSettingsWith(kSchemaErrorReply);
    const auto pollTimes = [&client](const int times)
    {
        for (int poll = 0; poll < times; ++poll)
        {
            client.Poll();
        }
    };
    const auto pollUntilSettingsRequests = [&client, &efb](const int expected)
    {
        client.Poll();

        return efb.SettingsRequests() >= expected;
    };

    pollTimes(9);
    QTRY_VERIFY(pollUntilSettingsRequests(2));
    pollTimes(9);
    QTRY_VERIFY(pollUntilSettingsRequests(3));
    QTest::qWait(50);

    QVERIFY(client.GetSoftwareMajorVersion() == 3);
    QCOMPARE(client.GetSetting(kWeightUnitSetting, "unset"), std::string("Pounds"));
    QVERIFY(client.IsAvailable());
}

void FenixEfbClientTest::settingsAreRequestedOnTheFirstPollAndEveryTenthAfter()
{
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.SubscribeSetting(kWeightUnitSetting);

    client.Poll();

    QTRY_VERIFY(client.GetSoftwareMajorVersion().has_value());
    QCOMPARE(efb.SettingsRequests(), 1);

    for (int poll = 2; poll <= 10; ++poll)
    {
        client.Poll();
        QTest::qWait(10);
    }

    QCOMPARE(efb.SettingsRequests(), 1);

    client.Poll();

    QTRY_COMPARE(efb.SettingsRequests(), 2);
}

void FenixEfbClientTest::noSettingsRequestGoesOutWhileAnotherIsInFlight()
{
    FakeFenixEfb efb;
    QVERIFY(efb.Listen());
    efb.HoldSettingsReplies();
    FenixEfbClient client;
    client.SetEndpointForTest(efb.Endpoint());
    client.SubscribeSetting(kWeightUnitSetting);

    client.Poll();
    QTRY_COMPARE(efb.SettingsRequests(), 1);

    for (int poll = 2; poll <= 14; ++poll)
    {
        client.Poll();
    }

    QTest::qWait(100);

    QCOMPARE(efb.SettingsRequests(), 1);
}

QTEST_GUILESS_MAIN(FenixEfbClientTest)

#include "tst_fenix_efb_client.moc"
