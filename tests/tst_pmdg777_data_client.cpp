#include <QtTest/QTest>

#include <algorithm>
#include <string>
#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/pmdg/Pmdg777DataClient.h"
#include "../src/infrastructure/pmdg/Pmdg777SdkData.h"

namespace
{
    PMDG_777X_Data MakeSampleData()
    {
        PMDG_777X_Data data{};
        data.AircraftModel = 6;
        data.ELEC_annunExtPowr_ON[0] = true;
        data.LTS_Beacon_Sw_ON = true;
        data.BRAKES_ParkingBrakeLeverOn = true;
        data.APURunning = true;
        data.WheelChocksSet = true;
        data.FUEL_QtyLeft = 20000.0f;
        data.FUEL_QtyRight = 20000.0f;
        data.FUEL_QtyCenter = 10000.0f;
        data.DOOR_state[0] = 0;
        data.FMC_CruiseAlt = 32000;

        return data;
    }

    bool MappedEvent(const std::string& name)
    {
        return std::ranges::find(FakeSimConnectApi::mappedEventNames, name)
            != FakeSimConnectApi::mappedEventNames.end();
    }
}

class Pmdg777DataClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void init();

    static void noDataBeforeFirstPacket();
    static void receivesClientDataThroughSession();
    static void exposesTypedFields();
    static void kickFiredWhenStale();
    static void kickReleasesSwitchOnNextPoll();
    static void kickSuppressedInFlight();
    static void kickStopsAfterFirstData();
    static void invalidPacketDoesNotLatchData();
    static void toggleDoorTransmitsMappedEvent();
    static void fmcFlightPlanFromCruiseAltOrFlightNumber();
};

void Pmdg777DataClientTest::init()
{
    FakeSimConnectApi::Reset();
}

void Pmdg777DataClientTest::noDataBeforeFirstPacket()
{
    const Pmdg777DataClient client;

    QVERIFY(!client.HasData());
    QCOMPARE(client.AircraftModel(), 0);
}

void Pmdg777DataClientTest::receivesClientDataThroughSession()
{
    Pmdg777DataClient client;

    client.SetClockForTest([] { return 0LL; });

    client.Poll();

    QVERIFY(std::ranges::find(FakeSimConnectApi::mappedClientDataAreas, std::string(PMDG_777X_DATA_NAME))
        != FakeSimConnectApi::mappedClientDataAreas.end());

    const PMDG_777X_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));

    client.Poll();

    QVERIFY(client.HasData());
}

void Pmdg777DataClientTest::exposesTypedFields()
{
    Pmdg777DataClient client;

    client.SetClockForTest([] { return 0LL; });

    client.Poll();

    const PMDG_777X_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QCOMPARE(client.AircraftModel(), 6);
    QVERIFY(client.ExtPowerConnected());
    QVERIFY(client.BeaconOn());
    QVERIFY(client.ParkingBrakeOn());
    QVERIFY(client.ApuRunning());
    QVERIFY(client.WheelChocksSet());
    QCOMPARE(client.TotalFuelLbs(), 50000.0);
    QCOMPARE(client.DoorState(0), 0);
}

void Pmdg777DataClientTest::kickFiredWhenStale()
{
    Pmdg777DataClient client;

    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();

    QVERIFY(MappedEvent("#69750"));
    QVERIFY(FakeSimConnectApi::transmittedEvents > 0);

    now = 1000;
    client.Poll();
    const int afterRelease = FakeSimConnectApi::transmittedEvents;

    now = 2000;
    client.Poll();
    QCOMPARE(FakeSimConnectApi::transmittedEvents, afterRelease);

    now = 6000;
    client.Poll();
    QVERIFY(FakeSimConnectApi::transmittedEvents > afterRelease);
}

void Pmdg777DataClientTest::kickReleasesSwitchOnNextPoll()
{
    Pmdg777DataClient client;
    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();
    const int afterKick = FakeSimConnectApi::transmittedEvents;

    now = 1000;
    client.Poll();
    QCOMPARE(FakeSimConnectApi::transmittedEvents, afterKick + 1);

    now = 2000;
    client.Poll();
    QCOMPARE(FakeSimConnectApi::transmittedEvents, afterKick + 1);
}

void Pmdg777DataClientTest::kickSuppressedInFlight()
{
    Pmdg777DataClient client;
    client.SetClockForTest([] { return 100000LL; });
    client.SetInFlight(true);

    client.Poll();

    QVERIFY(!MappedEvent("#69750"));
}

void Pmdg777DataClientTest::kickStopsAfterFirstData()
{
    Pmdg777DataClient client;
    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();
    const PMDG_777X_Data sample = MakeSampleData();
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();

    QVERIFY(client.HasData());
    FakeSimConnectApi::mappedEventNames.clear();

    now = 100000;
    client.Poll();

    QVERIFY(!MappedEvent("#69750"));
}

void Pmdg777DataClientTest::invalidPacketDoesNotLatchData()
{
    Pmdg777DataClient client;
    long long now = 0;
    client.SetClockForTest([&now] { return now; });

    client.Poll();
    constexpr PMDG_777X_Data empty{};
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &empty, sizeof(empty));
    client.Poll();

    QVERIFY(!client.HasData());

    const int beforeRetry = FakeSimConnectApi::transmittedEvents;
    now = 6000;
    client.Poll();
    QVERIFY(FakeSimConnectApi::transmittedEvents > beforeRetry);
}

void Pmdg777DataClientTest::toggleDoorTransmitsMappedEvent()
{
    Pmdg777DataClient client;
    client.SetClockForTest([] { return 0LL; });
    client.Poll();

    FakeSimConnectApi::mappedEventNames.clear();
    client.ToggleDoor(0);
    QVERIFY(MappedEvent("#83643"));

    client.ToggleDoor(12);
    QVERIFY(MappedEvent("#83655"));
}

void Pmdg777DataClientTest::fmcFlightPlanFromCruiseAltOrFlightNumber()
{
    Pmdg777DataClient client;
    client.SetClockForTest([] { return 0LL; });
    client.Poll();

    QVERIFY(!client.HasFmcFlightPlan());

    PMDG_777X_Data sample = MakeSampleData();
    sample.FMC_CruiseAlt = 0;
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();
    QVERIFY(!client.HasFmcFlightPlan());

    sample.FMC_CruiseAlt = 32000;
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();
    QVERIFY(client.HasFmcFlightPlan());

    sample.FMC_CruiseAlt = 0;
    std::strncpy(sample.FMC_flightNumber, "DHL611", sizeof(sample.FMC_flightNumber));
    FakeSimConnectApi::PushClientData(PMDG_777X_DATA_DEFINITION, &sample, sizeof(sample));
    client.Poll();
    QVERIFY(client.HasFmcFlightPlan());
}

QTEST_APPLESS_MAIN(Pmdg777DataClientTest)

#include "tst_pmdg777_data_client.moc"
