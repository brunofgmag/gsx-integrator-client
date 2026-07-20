#include <QtTest/QTest>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include "../src/infrastructure/fenix/FenixEfbClient.h"

class FenixEfbClientTest final : public QObject
{
    Q_OBJECT

private slots:
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
};

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

QTEST_APPLESS_MAIN(FenixEfbClientTest)

#include "tst_fenix_efb_client.moc"
