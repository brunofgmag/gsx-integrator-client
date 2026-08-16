#include <QtTest/QTest>

#include <algorithm>
#include <string>
#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/pmdg/Pmdg737DataClient.h"
#include "../src/infrastructure/pmdg/Pmdg737SdkData.h"

namespace
{
    PMDG_NG3_Data MakeSampleData()
    {
        PMDG_NG3_Data data{};
        data.AircraftModel = 5;
        data.ELEC_annunGRD_POWER_AVAILABLE = true;
        data.ELEC_BusPowered[11] = true;
        data.GroundConnAvailable = true;
        data.LTS_AntiCollisionSw = true;
        data.PED_annunParkingBrake = true;
        data.IRS_aligned = true;
        data.FUEL_QtyLeft = 8000.0f;
        data.FUEL_QtyRight = 8000.0f;
        data.FUEL_QtyCenter = 2000.0f;
        data.DOOR_annunFWD_ENTRY = true;
        data.DOOR_annunAFT_CARGO = true;

        return data;
    }

    bool MappedEvent(const std::string& name)
    {
        return std::ranges::find(FakeSimConnectApi::mappedEventNames, name)
            != FakeSimConnectApi::mappedEventNames.end();
    }
}

class Pmdg737DataClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void init();

    static void noDataBeforeFirstPacket();
    static void receivesClientDataThroughSession();
    static void exposesTypedFields();
    static void invalidPacketDoesNotLatchData();
    static void pollNeverTransmitsAnEvent();
    static void doorEventsSkipTheTwoNumbersTheSdkReserves();
    static void mainCargoHasNoAnnunciatorToReadBack();
    static void groundPowerSeparatesAvailableFromPowered();
};

void Pmdg737DataClientTest::init()
{
    FakeSimConnectApi::Reset();
}

void Pmdg737DataClientTest::noDataBeforeFirstPacket()
{
    const Pmdg737DataClient client;

    QVERIFY(!client.HasData());
    QCOMPARE(client.AircraftModel(), 0);
    QCOMPARE(client.TotalFuelLbs(), 0.0);
}

void Pmdg737DataClientTest::receivesClientDataThroughSession()
{
    Pmdg737DataClient client;

    client.Poll();

    QVERIFY(std::ranges::find(FakeSimConnectApi::mappedClientDataAreas, std::string(PMDG_NG3_DATA_NAME))
        != FakeSimConnectApi::mappedClientDataAreas.end());

    const PMDG_NG3_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));

    client.Poll();

    QVERIFY(client.HasData());
}

void Pmdg737DataClientTest::exposesTypedFields()
{
    Pmdg737DataClient client;

    client.Poll();

    const PMDG_NG3_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QCOMPARE(client.AircraftModel(), 5);
    QVERIFY(client.GroundPowerAvailable());
    QVERIFY(client.AnyMainBusPowered());
    QVERIFY(client.GroundConnAvailable());
    QVERIFY(client.BeaconOn());
    QVERIFY(client.ParkingBrakeOn());
    QVERIFY(client.IrsAligned());
    QCOMPARE(client.TotalFuelLbs(), 18000.0);
    QVERIFY(client.DoorOpen(Pmdg737Door::FwdEntry));
    QVERIFY(client.DoorOpen(Pmdg737Door::AftCargo));
    QVERIFY(!client.DoorOpen(Pmdg737Door::AftEntry));
}

void Pmdg737DataClientTest::invalidPacketDoesNotLatchData()
{
    Pmdg737DataClient client;

    client.Poll();

    const PMDG_NG3_Data empty{};
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &empty, sizeof(empty));
    client.Poll();

    QVERIFY(!client.HasData());
}

void Pmdg737DataClientTest::pollNeverTransmitsAnEvent()
{
    Pmdg737DataClient client;

    client.Poll();
    client.Poll();
    client.Poll();

    QCOMPARE(FakeSimConnectApi::transmittedEvents, 0);
}

void Pmdg737DataClientTest::doorEventsSkipTheTwoNumbersTheSdkReserves()
{
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::FwdEntry), 14005u);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::AftService), 14008u);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::FwdCargo), 14013u);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::Airstair), 14017u);

    Pmdg737DataClient client;
    client.Poll();
    FakeSimConnectApi::mappedEventNames.clear();

    client.ToggleDoor(Pmdg737Door::FwdEntry);
    QVERIFY(MappedEvent("#83637"));

    client.ToggleDoor(Pmdg737Door::FwdCargo);
    QVERIFY(MappedEvent("#83645"));
    QVERIFY(!MappedEvent("#83641"));

    client.ToggleDoor(Pmdg737Door::Airstair);
    QVERIFY(MappedEvent("#83649"));
}

void Pmdg737DataClientTest::mainCargoHasNoAnnunciatorToReadBack()
{
    QVERIFY(Pmdg737DataClient::HasAnnunciator(Pmdg737Door::FwdCargo));
    QVERIFY(!Pmdg737DataClient::HasAnnunciator(Pmdg737Door::MainCargo));
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::MainCargo), 14015u);
}

void Pmdg737DataClientTest::groundPowerSeparatesAvailableFromPowered()
{
    Pmdg737DataClient client;

    client.Poll();

    PMDG_NG3_Data sample = MakeSampleData();
    sample.ELEC_BusPowered[11] = false;
    sample.ELEC_BusPowered[12] = false;
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QVERIFY(client.GroundPowerAvailable());
    QVERIFY(!client.AnyMainBusPowered());
}

QTEST_APPLESS_MAIN(Pmdg737DataClientTest)

#include "tst_pmdg737_data_client.moc"
