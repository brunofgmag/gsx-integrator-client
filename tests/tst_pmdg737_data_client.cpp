#include <QtTest/QTest>

#include <algorithm>
#include <string>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include "ProbeLines.h"
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
        data.FUEL_QtyLeft = 8000.0F;
        data.FUEL_QtyRight = 8000.0F;
        data.FUEL_QtyCenter = 2000.0F;

        return data;
    }

    bool MappedEvent(const std::string& name)
    {
        return std::ranges::find(FakeSimConnectApi::mappedEventNames, name)
            != FakeSimConnectApi::mappedEventNames.end();
    }

    QStringList EventLines(const qsizetype before)
    {
        return ProbeLines(QStringLiteral("writes.log")).mid(before).filter(QStringLiteral("event "));
    }
}

class Pmdg737DataClientTest final : public QObject
{
    Q_OBJECT

    QTemporaryDir directory_;

private slots:
    void initTestCase();
    static void init();

    static void noDataBeforeFirstPacket();
    static void receivesClientDataThroughSession();
    static void exposesTypedFields();
    static void invalidPacketDoesNotLatchData();
    static void dataExpiresWhenTheBlockStopsArriving();
    static void aFreshBlockRevivesTheReading();
    static void pollNeverTransmitsAnEvent();
    static void doorEventsSkipTheTwoNumbersTheSdkReserves();
    static void groundPowerSeparatesAvailableFromPowered();
    static void aDoorToggleIsLoggedUnderItsDoorName();
};

void Pmdg737DataClientTest::initTestCase()
{
    QVERIFY(directory_.isValid());
    qputenv("GSXI_PROBE_DIR", directory_.path().toUtf8());
    probe::SetEnabled(true);
}

void Pmdg737DataClientTest::aDoorToggleIsLoggedUnderItsDoorName()
{
#ifndef NDEBUG
    Pmdg737DataClient client;
    client.Poll();
    const qsizetype before = ProbeLines(QStringLiteral("writes.log")).size();

    client.ToggleDoor(Pmdg737Door::FwdEntry);
    client.ToggleDoor(Pmdg737Door::FwdCargo);
    client.ToggleDoor(Pmdg737Door::Airstair);

    QCOMPARE(EventLines(before),
             (QStringList{
                 QStringLiteral("event DOOR_FWD_ENTRY (#83637) param=536870912 n=1"),
                 QStringLiteral("event DOOR_FWD_CARGO (#83645) param=536870912 n=1"),
                 QStringLiteral("event DOOR_AIRSTAIR (#83649) param=536870912 n=1")
             }));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void Pmdg737DataClientTest::init()
{
    FakeSimConnectApi::Reset();
}

void Pmdg737DataClientTest::noDataBeforeFirstPacket()
{
    const Pmdg737DataClient client;

    QVERIFY(!client.HasData());
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

    QVERIFY(client.GroundPowerAvailable());
    QVERIFY(client.AnyMainBusPowered());
    QVERIFY(client.BeaconOn());
    QVERIFY(client.ParkingBrakeOn());
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
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::FwdEntry), 14005U);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::AftService), 14008U);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::FwdCargo), 14013U);
    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::Airstair), 14017U);

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

    QCOMPARE(Pmdg737DataClient::DoorEventOffsetFor(Pmdg737Door::MainCargo), 14015U);
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

void Pmdg737DataClientTest::dataExpiresWhenTheBlockStopsArriving()
{
    Pmdg737DataClient client;
    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();
    const PMDG_NG3_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QVERIFY(client.HasData());

    now = 5000;
    QVERIFY(client.HasData());

    now = 20000;
    QVERIFY(!client.HasData());
    QVERIFY(!client.BeaconOn());
    QVERIFY(!client.GroundPowerAvailable());
}

void Pmdg737DataClientTest::aFreshBlockRevivesTheReading()
{
    Pmdg737DataClient client;
    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();
    const PMDG_NG3_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    now = 20000;
    QVERIFY(!client.HasData());

    FakeSimConnectApi::PushClientData(PMDG_NG3_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QVERIFY(client.HasData());
    QVERIFY(client.BeaconOn());
}

QTEST_APPLESS_MAIN(Pmdg737DataClientTest)

#include "tst_pmdg737_data_client.moc"
