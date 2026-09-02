#include <QtTest/QTest>

#include <map>
#include "doubles/FakeVariableGateway.h"
#include "../src/infrastructure/gsx/GsxDoorSync.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    using namespace gsx::lvars;

    constexpr double kCouatlUp = 1.0;
    constexpr double kCouatlDown = 0.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kStairsParked = 1.0;

    struct Recorder;

    void Tick(GsxDoorSync& sync, FakeVariableGateway& gateway, Recorder& recorder);

    struct Recorder
    {
        std::map<GsxDoor, bool> last;
        int writes = 0;

        GsxDoorSync::DoorWriter Writer()
        {
            return [this](const GsxDoor door, const bool open)
            {
                last[door] = open;
                ++writes;
            };
        }

        [[nodiscard]] bool Opened(const GsxDoor door) const
        {
            const auto it = last.find(door);

            return it != last.end() && it->second;
        }

        [[nodiscard]] bool Closed(const GsxDoor door) const
        {
            const auto it = last.find(door);

            return it != last.end() && !it->second;
        }
    };

    void Tick(GsxDoorSync& sync, FakeVariableGateway& gateway, Recorder& recorder)
    {
        sync.Observe();

        if (gateway.lvars[kCouatlStarted] >= kCouatlUp)
        {
            sync.Sync(recorder.Writer());
        }
    }
}

class GsxDoorSyncTest final : public QObject
{
    Q_OBJECT

private slots:
    static void followsVehicleStateWhileCouatlKeepsRunning();
    static void opensThePaxDoorWhileTheStairsAreStillApproaching();
    static void keepsThePaxDoorShutWhileTheStairsAreMerelyDispatched();
    static void keepsTheCargoDoorShutWhileTheLoaderIsMerelyDispatched();
    static void opensTheCargoDoorOnceTheLoaderWaitsAtIt();
    static void distrustsVehicleStateInheritedAcrossACouatlRestart();
    static void distrustsAnInheritedJetwayDownToUnavailable();
    static void trustsTheVehicleStateAgainOnceItActuallyMoves();
    static void treatsAFirstStartAsAStartAndNotAsARestart();
    static void seesTheRestartThoughSyncNeverRunsWhileGsxIsDown();
};

void GsxDoorSyncTest::followsVehicleStateWhileCouatlKeepsRunning()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::opensThePaxDoorWhileTheStairsAreStillApproaching()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kVehicleApproaching;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::keepsThePaxDoorShutWhileTheStairsAreMerelyDispatched()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kVehicleDispatched;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(!recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::keepsTheCargoDoorShutWhileTheLoaderIsMerelyDispatched()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kVehicleDispatched;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(!recorder.Opened(GsxDoor::FwdCargo));
}

void GsxDoorSyncTest::opensTheCargoDoorOnceTheLoaderWaitsAtIt()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::FwdCargo));
}

void GsxDoorSyncTest::distrustsVehicleStateInheritedAcrossACouatlRestart()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    Tick(sync, gateway, recorder);

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(sync.VehicleState(kPassengerStairsRearState, 0.0), 0.0);
}

void GsxDoorSyncTest::distrustsAnInheritedJetwayDownToUnavailable()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kJetway] = kJetwayDocked;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    Tick(sync, gateway, recorder);

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
}

void GsxDoorSyncTest::trustsTheVehicleStateAgainOnceItActuallyMoves()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlDown;
    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Closed(GsxDoor::AftPax));

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::treatsAFirstStartAsAStartAndNotAsARestart()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlDown;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::seesTheRestartThoughSyncNeverRunsWhileGsxIsDown()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    for (int tick = 0; tick < 5; ++tick)
    {
        sync.Observe();
    }
    QCOMPARE(recorder.writes, 1);

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
}

QTEST_APPLESS_MAIN(GsxDoorSyncTest)

#include "tst_gsx_door_sync.moc"
