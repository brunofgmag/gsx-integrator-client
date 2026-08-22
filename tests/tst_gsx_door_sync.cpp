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
}

class GsxDoorSyncTest final : public QObject
{
    Q_OBJECT

private slots:
    static void followsVehicleStateWhileCouatlKeepsRunning();
    static void distrustsVehicleStateInheritedAcrossACouatlRestart();
    static void distrustsAnInheritedJetwayDownToUnavailable();
    static void trustsTheVehicleStateAgainOnceItActuallyMoves();
    static void treatsAFirstStartAsAStartAndNotAsARestart();
};

void GsxDoorSyncTest::followsVehicleStateWhileCouatlKeepsRunning()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    sync.Sync(recorder.Writer());
    sync.Sync(recorder.Writer());

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::distrustsVehicleStateInheritedAcrossACouatlRestart()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    sync.Sync(recorder.Writer());
    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    sync.Sync(recorder.Writer());

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    sync.Sync(recorder.Writer());

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

    sync.Sync(recorder.Writer());
    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    sync.Sync(recorder.Writer());

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    sync.Sync(recorder.Writer());

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
}

void GsxDoorSyncTest::trustsTheVehicleStateAgainOnceItActuallyMoves()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    sync.Sync(recorder.Writer());
    gateway.lvars[kCouatlStarted] = kCouatlDown;
    sync.Sync(recorder.Writer());
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    sync.Sync(recorder.Writer());
    QVERIFY(recorder.Closed(GsxDoor::AftPax));

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    sync.Sync(recorder.Writer());

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    sync.Sync(recorder.Writer());

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::treatsAFirstStartAsAStartAndNotAsARestart()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlDown;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    sync.Sync(recorder.Writer());
    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kCouatlStarted] = kCouatlUp;
    sync.Sync(recorder.Writer());

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

QTEST_APPLESS_MAIN(GsxDoorSyncTest)

#include "tst_gsx_door_sync.moc"
