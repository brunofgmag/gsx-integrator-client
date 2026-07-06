#include <QtTest/QTest>

#include <memory>
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/domain/ports/Aircraft.h"
#include "../src/infrastructure/aircraft/AircraftFactory.h"

class AircraftDetectionTest final : public QObject
{
    Q_OBJECT

private slots:
    static void returnsNullWhenNameUnavailable();
    static void returnsNullForUnknownAircraft();
    static void detectsPassengerVariant();
    static void detectsCargoVariant();
    static void detectsCargoFromTitleSuffixWithLivery();
    static void detectsPassengerFromTitleWithLivery();
    static void returnsNullForEmptyName();
    static void detectsTitleCaseInsensitively();
    static void detectsByAtcModelWhenLiveryRenamesTitle();
    static void detectsCargoByAtcModel();
    static void detectsWhenAtcModelUnavailable();
};

void AircraftDetectionTest::returnsNullWhenNameUnavailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftNameAvailable = false;

    QVERIFY(DetectAircraft(&gateway, &status) == nullptr);
}

void AircraftDetectionTest::returnsNullForUnknownAircraft()
{
    AutomationStatus status;
    FakeVariableGateway gateway;

    gateway.aircraftName = "Airbus 738neo";

    QVERIFY(DetectAircraft(&gateway, &status) == nullptr);
}

void AircraftDetectionTest::detectsPassengerVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "TFDi Design MD-11";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(!aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsCargoVariant()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "TFDi Design MD-11F";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsCargoFromTitleSuffixWithLivery()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "TFDi Design MD-11F FedEx";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsPassengerFromTitleWithLivery()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "TFDi Design MD-11 Delta";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(!aircraft->IsCargoVariant());
}

void AircraftDetectionTest::returnsNullForEmptyName()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "";

    QVERIFY(DetectAircraft(&gateway, &status) == nullptr);
}

void AircraftDetectionTest::detectsTitleCaseInsensitively()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "tfdi design md-11";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(!aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsByAtcModelWhenLiveryRenamesTitle()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "FLAGSHIP PAX PW";
    gateway.atcModel = "MD11";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(!aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsCargoByAtcModel()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "FedEx Heavy Freight";
    gateway.atcModel = "MD11F";

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(aircraft->IsCargoVariant());
}

void AircraftDetectionTest::detectsWhenAtcModelUnavailable()
{
    FakeVariableGateway gateway;
    AutomationStatus status;

    gateway.aircraftName = "TFDi Design MD-11 PW44";
    gateway.atcModelAvailable = false;

    const std::unique_ptr<Aircraft> aircraft = DetectAircraft(&gateway, &status);

    QVERIFY(aircraft != nullptr);
    QVERIFY(!aircraft->IsCargoVariant());
}

QTEST_APPLESS_MAIN(AircraftDetectionTest)

#include "tst_aircraft_detection.moc"
