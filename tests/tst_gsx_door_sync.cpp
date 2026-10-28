#include <QtTest/QTest>

#include <algorithm>
#include <map>
#include <vector>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "doubles/FakeVariableGateway.h"
#include "../src/infrastructure/gsx/GsxDoorSync.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    using namespace gsx::lvars;

    constexpr double kCouatlUp = 1.0;
    constexpr double kCouatlDown = 0.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kNoJetway = 2.0;
    constexpr double kStairsParked = 1.0;
    constexpr double kStairsFinalPosition = gsx::states::kStairsFinalPosition;
    constexpr int kTransitBoundTicks = 30;
    constexpr int kTicksWellPastTheBound = 60;

    class LogCapture
    {
    public:
        LogCapture() : previous_(qInstallMessageHandler(Collect)) { Lines().clear(); }
        ~LogCapture() { qInstallMessageHandler(previous_); }

        [[nodiscard]] static long long Count(const char* fragment)
        {
            return std::ranges::count_if(Lines(), [fragment](const QString& line)
                                         { return line.contains(QLatin1String(fragment)); });
        }

    private:
        static QStringList& Lines()
        {
            static QStringList lines;

            return lines;
        }

        static void Collect(QtMsgType, const QMessageLogContext&, const QString& message)
        {
            Lines().append(message);
        }

        QtMessageHandler previous_;
    };

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

    struct Positions
    {
        std::map<GsxDoor, DoorPosition> byDoor;

        GsxDoorSync::PositionReader Reader()
        {
            return [this](const GsxDoor door)
            {
                const auto it = byDoor.find(door);

                return it != byDoor.end() ? it->second : DoorPosition::NotRead;
            };
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
    static void closesTheCargoDoorOnceTheLoaderStartsFinishing();
    static void distrustsVehicleStateInheritedAcrossACouatlRestart();
    static void distrustsAnInheritedJetwayDownToUnavailable();
    static void trustsTheVehicleStateAgainOnceItActuallyMoves();
    static void treatsAFirstStartAsAStartAndNotAsARestart();
    static void seesTheRestartThoughSyncNeverRunsWhileGsxIsDown();
    static void holdsTheCloseWhileTheWatchedExitIsStillOpening();
    static void holdsTheReopenWhileTheWatchedExitIsStillClosing();
    static void ignoresAnExitNobodyWatches();
    static void needsTwoSamplesBeforeCallingTheExitMoving();
    static void closesOnlyThePassengerDoorsWhenThePassengersAreAboard();
    static void keepsThePassengerDoorsShutWhenStairsArriveWhileTheyAreHeld();
    static void releasesThePassengerHoldWhenTheDepartureHoldIsReleased();
    static void stillClosesTheCargoDoorsForDepartureAfterThePassengerHold();
    static void closesADoorTheLastProcessOpenedOnceItsVehicleIsGone();
    static void leavesADoorAloneWhenTheLastProcessSavedNothingAboutIt();
    static void waitsForTheVehicleStateBeforeClosingARestoredDoor();
    static void aRestoredForwardPassengerDoorWaitsForTheJetwayAndTheFrontStairs();
    static void aHeldCloseDoesNotWaitForTheVehicleState();
    static void aRestoredProcessDistrustsTheVehicleStateAGsxRestartLeftBehind();
    static void aRestoredProcessDistrustsAVehicleStateThatArrivesAfterTheRestore();
    static void aRestoredProcessTrustsTheVehicleStateWhenGsxDidNotRestart();
    static void theDoorTargetsSurviveTheMemory();
    static void theMemoryOmitsTheDoorsWithAnUnknownTarget();
    static void theMemoryIgnoresNamesThatAreNotTheDoorSyncs();
    static void theMemoryIgnoresAValueThatIsNeitherOpenNorClosed();
    static void anEmptyMemoryLeavesEveryTargetUnknown();
    static void aRestoredForwardPassengerDoorWaitsForTheJetwayWhenOnlyTheFrontStairsArrived();
    static void aDoorThisProcessOpenedClosesWithoutWaitingForTheOtherVehicle();
    static void aRestoredDoorClosedByCloseAllAndReopenedByTheJetwayClosesAgain();
    static void aRestoredDoorIsClosedWhenItsVehicleLeavesAfterTheyArrived();
    static void distrustsAVehicleStateThatHadNotArrivedAtALiveRestart();
    static void aSecondLiveRestartDistrustsTheStateGsxLeftBehindThen();
    static void restoringWithoutAGsxRestartForgetsTheStatesAnEarlierRestoreDistrusted();
    static void restoringWithoutAGsxRestartForgetsTheStatesAnEarlierRestoreWasWaitingFor();
    static void aDoorTheReaderCannotReadIsWrittenAsWithoutAReader();
    static void holdsTheWriteWhileTheDoorPositionIsPending();
    static void holdsTheCloseWhileTheDoorPositionIsPending();
    static void doesNotWriteADoorReadAlreadyOpenAndJudgesTheNextChangeFromIt();
    static void doesNotWriteADoorReadAlreadyClosedAfterItsOwnOpen();
    static void writesADoorReadDifferentFromTheTarget();
    static void neverClosesADoorReadOpenWithNoOpenOfItsOwn();
    static void closeAllSkipsADoorReadClosedAndWritesThePendingOnes();
    static void closeAllWritesTheDoorsReadOpenAndTheOnesNobodyReads();
    static void aDoorSkippedByCloseAllCanOpenAgain();
    static void aCloseAllThenAReadingStillOpenWhileTheVehicleServesIsATransit();
    static void anOwnOpenThenAReadingStillClosedWhenTheVehicleLeavesIsATransit();
    static void aDoorThatNeverObeysIsBelievedAfterTheBoundAndLoggedOnce();
    static void aDoorThatObeysInTimeEndsTheTransitWithoutALog();
    static void aWriteTheDoorHasNotObeyedYetDoesNotLetCloseAllSkipIt();
    static void aRecordMadeWithoutWritingStartsNoTransit();
    static void aRestoredTargetStartsNoTransit();
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

void GsxDoorSyncTest::closesTheCargoDoorOnceTheLoaderStartsFinishing()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderLoading;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::FwdCargo));

    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderFinishing;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdCargo));
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

void GsxDoorSyncTest::holdsTheCloseWhileTheWatchedExitIsStillOpening()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    gateway.avars["EXIT OPEN:3"] = 0.0;

    GsxDoorSync sync(&gateway);
    sync.WatchExit(GsxDoor::AftPax, 3);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    gateway.avars["EXIT OPEN:3"] = 30.0;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);
    QCOMPARE(recorder.writes, 1);

    gateway.avars["EXIT OPEN:3"] = 100.0;
    Tick(sync, gateway, recorder);
    QCOMPARE(recorder.writes, 1);

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::holdsTheReopenWhileTheWatchedExitIsStillClosing()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    gateway.avars["EXIT OPEN:3"] = 100.0;

    GsxDoorSync sync(&gateway);
    sync.WatchExit(GsxDoor::AftPax, 3);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);

    gateway.avars["EXIT OPEN:3"] = 60.0;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);
    QCOMPARE(recorder.writes, 2);

    gateway.avars["EXIT OPEN:3"] = 0.0;
    Tick(sync, gateway, recorder);
    QCOMPARE(recorder.writes, 2);

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 3);
}

void GsxDoorSyncTest::ignoresAnExitNobodyWatches()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsFrontState] = kStairsFinalPosition;
    gateway.avars["EXIT OPEN:0"] = 0.0;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.avars["EXIT OPEN:0"] = 45.0;
    gateway.lvars[kPassengerStairsFrontState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::needsTwoSamplesBeforeCallingTheExitMoving()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsFrontState] = kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    sync.WatchExit(GsxDoor::FwdPax, 0);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.avars["EXIT OPEN:0"] = 40.0;
    gateway.lvars[kPassengerStairsFrontState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::closesOnlyThePassengerDoorsWhenThePassengersAreAboard()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kJetway] = kJetwayDocked;
    gateway.lvars[kBaggageLoaderRearState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::FwdPax));
    QVERIFY(recorder.Opened(GsxDoor::AftCargo));

    const int writesBefore = recorder.writes;
    sync.HoldPassengerDoorsClosed(true);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QVERIFY(recorder.Opened(GsxDoor::AftCargo));
    QCOMPARE(recorder.writes, writesBefore + 1);
}

void GsxDoorSyncTest::keepsThePassengerDoorsShutWhenStairsArriveWhileTheyAreHeld()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync sync(&gateway);
    Recorder recorder;
    sync.HoldPassengerDoorsClosed(true);

    gateway.lvars[kPassengerStairsMiddleState] = gsx::states::kStairsFinalPosition;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    gateway.lvars[kCateringRearState] = gsx::states::kVehicleApproaching;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QVERIFY(!recorder.Opened(GsxDoor::MidPax));
    QVERIFY(!recorder.Opened(GsxDoor::AftPax));
    QVERIFY(recorder.Opened(GsxDoor::AftCatering));
}

void GsxDoorSyncTest::releasesThePassengerHoldWhenTheDepartureHoldIsReleased()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kJetway] = kJetwayDocked;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    sync.HoldPassengerDoorsClosed(true);
    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Closed(GsxDoor::FwdPax));

    sync.HoldClosedForDeparture(false);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::FwdPax));
}

void GsxDoorSyncTest::stillClosesTheCargoDoorsForDepartureAfterThePassengerHold()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;
    gateway.lvars[kBaggageLoaderRearState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    sync.HoldPassengerDoorsClosed(true);
    Tick(sync, gateway, recorder);
    QVERIFY(recorder.Opened(GsxDoor::FwdCargo));
    QVERIFY(recorder.Opened(GsxDoor::AftCargo));

    sync.HoldClosedForDeparture(true);
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdCargo));
    QVERIFY(recorder.Closed(GsxDoor::AftCargo));
}

void GsxDoorSyncTest::closesADoorTheLastProcessOpenedOnceItsVehicleIsGone()
{
    MemoryBag memory;
    {
        FakeVariableGateway previousGateway;
        previousGateway.lvars[kCouatlStarted] = kCouatlUp;
        previousGateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

        GsxDoorSync previous(&previousGateway);
        Recorder previousRecorder;

        Tick(previous, previousGateway, previousRecorder);
        QVERIFY(previousRecorder.Opened(GsxDoor::FwdCargo));

        previous.AppendMemory(memory);
    }

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = 0.0;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdCargo));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::leavesADoorAloneWhenTheLastProcessSavedNothingAboutIt()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = 0.0;

    GsxDoorSync newborn(&gateway);
    Recorder recorder;

    Tick(newborn, gateway, recorder);
    Tick(newborn, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
}

void GsxDoorSyncTest::waitsForTheVehicleStateBeforeClosingARestoredDoor()
{
    MemoryBag memory;
    memory.PutText("doorSync.FwdCargo", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kBaggageLoaderFrontState] = 0.0;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdCargo));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aRestoredForwardPassengerDoorWaitsForTheJetwayAndTheFrontStairs()
{
    MemoryBag memory;
    memory.PutText("doorSync.FwdPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kJetway] = kNoJetway;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsFrontState] = 0.0;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aHeldCloseDoesNotWaitForTheVehicleState()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    restored.HoldPassengerDoorsClosed(true);
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
}

void GsxDoorSyncTest::aRestoredProcessDistrustsTheVehicleStateAGsxRestartLeftBehind()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, true);
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(restored.VehicleState(kPassengerStairsRearState, 0.0), 0.0);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(restored, gateway, recorder);

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::aRestoredProcessDistrustsAVehicleStateThatArrivesAfterTheRestore()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, true);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aRestoredProcessTrustsTheVehicleStateWhenGsxDidNotRestart()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(restored.VehicleState(kPassengerStairsRearState, 0.0), kStairsFinalPosition);
}

void GsxDoorSyncTest::theDoorTargetsSurviveTheMemory()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsFinalPosition;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync previous(&gateway);
    Recorder recorder;

    Tick(previous, gateway, recorder);
    previous.HoldPassengerDoorsClosed(true);
    Tick(previous, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QVERIFY(recorder.Opened(GsxDoor::FwdCargo));

    MemoryBag memory;
    previous.AppendMemory(memory);

    GsxDoorSync restored(&gateway);
    restored.RestoreMemory(memory, false);

    MemoryBag again;
    restored.AppendMemory(again);

    QVERIFY(again == memory);
}

void GsxDoorSyncTest::theMemoryOmitsTheDoorsWithAnUnknownTarget()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    MemoryBag untouched;
    sync.AppendMemory(untouched);

    QVERIFY(untouched.Entries().empty());

    Tick(sync, gateway, recorder);

    MemoryBag memory;
    sync.AppendMemory(memory);

    QCOMPARE(memory.Entries().size(), std::size_t{1});
}

void GsxDoorSyncTest::theMemoryIgnoresNamesThatAreNotTheDoorSyncs()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync previous(&gateway);
    Recorder recorder;

    Tick(previous, gateway, recorder);

    MemoryBag own;
    previous.AppendMemory(own);

    std::vector<MemoryBag::Entry> entries = own.Entries();
    entries.emplace_back("gsx.service.boarding.completed", "1");
    entries.emplace_back("FwdCargo", "open");
    entries.emplace_back("closeRequested", "1");

    GsxDoorSync restored(&gateway);
    restored.RestoreMemory(MemoryBag(entries), false);

    MemoryBag again;
    restored.AppendMemory(again);

    QVERIFY(again == own);

    const std::vector<MemoryBag::Entry> foreign = {
        {"gsx.service.boarding.completed", "1"},
        {"FwdCargo", "open"},
    };

    GsxDoorSync foreignOnly(&gateway);
    foreignOnly.RestoreMemory(MemoryBag(foreign), false);

    MemoryBag nothing;
    foreignOnly.AppendMemory(nothing);

    QVERIFY(nothing.Entries().empty());
}

void GsxDoorSyncTest::theMemoryIgnoresAValueThatIsNeitherOpenNorClosed()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync previous(&gateway);
    Recorder recorder;

    Tick(previous, gateway, recorder);

    MemoryBag own;
    previous.AppendMemory(own);

    std::vector<MemoryBag::Entry> entries = own.Entries();
    for (MemoryBag::Entry& entry : entries)
    {
        entry.second = "ajar";
    }

    GsxDoorSync restored(&gateway);
    restored.RestoreMemory(MemoryBag(entries), false);

    MemoryBag again;
    restored.AppendMemory(again);

    QVERIFY(again.Entries().empty());
}

void GsxDoorSyncTest::anEmptyMemoryLeavesEveryTargetUnknown()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);

    MemoryBag before;
    sync.AppendMemory(before);

    QVERIFY(!before.Entries().empty());

    sync.RestoreMemory(MemoryBag{}, false);

    MemoryBag after;
    sync.AppendMemory(after);

    QVERIFY(after.Entries().empty());

    gateway.lvars[kBaggageLoaderFrontState] = 0.0;
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aRestoredForwardPassengerDoorWaitsForTheJetwayWhenOnlyTheFrontStairsArrived()
{
    MemoryBag memory;
    memory.PutText("doorSync.FwdPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsFrontState] = 0.0;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kJetway] = kNoJetway;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aDoorThisProcessOpenedClosesWithoutWaitingForTheOtherVehicle()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kJetway] = kJetwayDocked;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.lvars[kJetway] = kNoJetway;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
}

void GsxDoorSyncTest::aRestoredDoorClosedByCloseAllAndReopenedByTheJetwayClosesAgain()
{
    MemoryBag memory;
    memory.PutText("doorSync.FwdPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    restored.CloseAll(recorder.Writer());

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));

    gateway.lvars[kJetway] = kJetwayDocked;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::FwdPax));

    gateway.lvars[kJetway] = kNoJetway;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
}

void GsxDoorSyncTest::aRestoredDoorIsClosedWhenItsVehicleLeavesAfterTheyArrived()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsRearState] = 0.0;
    Tick(restored, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::distrustsAVehicleStateThatHadNotArrivedAtALiveRestart()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlDown;
    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(sync.VehicleState(kPassengerStairsRearState, 0.0), 0.0);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
}

void GsxDoorSyncTest::aSecondLiveRestartDistrustsTheStateGsxLeftBehindThen()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    GsxDoorSync sync(&gateway);
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);

    gateway.lvars[kCouatlStarted] = kCouatlDown;
    gateway.lvars[kPassengerStairsRearState] = gsx::states::kStairsWaitingForDoor;
    Tick(sync, gateway, recorder);
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 2);
    QVERIFY(recorder.Closed(GsxDoor::AftPax));
}

void GsxDoorSyncTest::restoringWithoutAGsxRestartForgetsTheStatesAnEarlierRestoreDistrusted()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, true);
    restored.RestoreMemory(memory, false);
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(restored.VehicleState(kPassengerStairsRearState, 0.0), kStairsFinalPosition);
}

void GsxDoorSyncTest::restoringWithoutAGsxRestartForgetsTheStatesAnEarlierRestoreWasWaitingFor()
{
    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    GsxDoorSync restored(&gateway);
    Recorder recorder;

    restored.RestoreMemory(memory, true);
    restored.RestoreMemory(memory, false);

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(restored, gateway, recorder);
    Tick(restored, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(restored.VehicleState(kPassengerStairsRearState, 0.0), kStairsFinalPosition);
}

void GsxDoorSyncTest::aDoorTheReaderCannotReadIsWrittenAsWithoutAReader()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);

    recorder.writes = 0;
    sync.CloseAll(recorder.Writer());

    QCOMPARE(recorder.writes, static_cast<int>(GsxDoor::Count));
}

void GsxDoorSyncTest::holdsTheWriteWhileTheDoorPositionIsPending()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Pending;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::holdsTheCloseWhileTheDoorPositionIsPending()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Pending;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::doesNotWriteADoorReadAlreadyOpenAndJudgesTheNextChangeFromIt()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::doesNotWriteADoorReadAlreadyClosedAfterItsOwnOpen()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    Tick(sync, gateway, recorder);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);
    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::writesADoorReadDifferentFromTheTarget()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::neverClosesADoorReadOpenWithNoOpenOfItsOwn()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    Positions positions;
    for (const GsxDoor door : {GsxDoor::FwdPax, GsxDoor::AftPax, GsxDoor::FwdCargo, GsxDoor::AftCargo})
    {
        positions.byDoor[door] = DoorPosition::Open;
    }

    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
}

void GsxDoorSyncTest::closeAllSkipsADoorReadClosedAndWritesThePendingOnes()
{
    FakeVariableGateway gateway;

    Positions positions;
    positions.byDoor[GsxDoor::FwdPax] = DoorPosition::Closed;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Pending;
    positions.byDoor[GsxDoor::FwdCargo] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    sync.CloseAll(recorder.Writer());

    QCOMPARE(recorder.writes, static_cast<int>(GsxDoor::Count) - 2);
    QVERIFY(!recorder.last.contains(GsxDoor::FwdPax));
    QVERIFY(!recorder.last.contains(GsxDoor::FwdCargo));
    QVERIFY(recorder.Closed(GsxDoor::AftPax));
}

void GsxDoorSyncTest::closeAllWritesTheDoorsReadOpenAndTheOnesNobodyReads()
{
    FakeVariableGateway gateway;

    Positions positions;
    positions.byDoor[GsxDoor::MidPax] = DoorPosition::Open;
    positions.byDoor[GsxDoor::AftCargo] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    sync.CloseAll(recorder.Writer());

    QVERIFY(recorder.Closed(GsxDoor::MidPax));
    QVERIFY(recorder.Closed(GsxDoor::FwdPax));
    QVERIFY(recorder.Closed(GsxDoor::AftCatering));
    QVERIFY(!recorder.last.contains(GsxDoor::AftCargo));
}

void GsxDoorSyncTest::aDoorSkippedByCloseAllCanOpenAgain()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    sync.CloseAll(recorder.Writer());
    recorder.writes = 0;
    recorder.last.clear();

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aCloseAllThenAReadingStillOpenWhileTheVehicleServesIsATransit()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    sync.CloseAll(recorder.Writer());

    QVERIFY(recorder.Closed(GsxDoor::AftPax));

    recorder.writes = 0;
    recorder.last.clear();
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::anOwnOpenThenAReadingStillClosedWhenTheVehicleLeavesIsATransit()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 2);

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 2);
}

void GsxDoorSyncTest::aDoorThatNeverObeysIsBelievedAfterTheBoundAndLoggedOnce()
{
    const LogCapture log;
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    sync.CloseAll(recorder.Writer());
    recorder.writes = 0;
    recorder.last.clear();

    for (int tick = 0; tick < kTransitBoundTicks; ++tick)
    {
        Tick(sync, gateway, recorder);
    }

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(LogCapture::Count("believed"), 0LL);

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(LogCapture::Count("believed"), 1LL);

    for (int tick = 0; tick < kTicksWellPastTheBound; ++tick)
    {
        Tick(sync, gateway, recorder);
    }

    QCOMPARE(recorder.writes, 0);
    QCOMPARE(LogCapture::Count("believed"), 1LL);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aDoorThatObeysInTimeEndsTheTransitWithoutALog()
{
    const LogCapture log;
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    Tick(sync, gateway, recorder);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    for (int tick = 0; tick < kTicksWellPastTheBound; ++tick)
    {
        Tick(sync, gateway, recorder);
    }

    QCOMPARE(recorder.writes, 1);
    QCOMPARE(LogCapture::Count("believed"), 0LL);

    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 1);
}

void GsxDoorSyncTest::aWriteTheDoorHasNotObeyedYetDoesNotLetCloseAllSkipIt()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));

    sync.CloseAll(recorder.Writer());

    QVERIFY(recorder.Closed(GsxDoor::AftPax));
}

void GsxDoorSyncTest::aRecordMadeWithoutWritingStartsNoTransit()
{
    const LogCapture log;
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Open;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;
    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
    QCOMPARE(LogCapture::Count("believed"), 0LL);
}

void GsxDoorSyncTest::aRestoredTargetStartsNoTransit()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = kCouatlUp;
    gateway.lvars[kPassengerStairsRearState] = kStairsParked;

    MemoryBag memory;
    memory.PutText("doorSync.AftPax", "open");

    Positions positions;
    positions.byDoor[GsxDoor::AftPax] = DoorPosition::Closed;
    GsxDoorSync sync(&gateway);
    sync.ReadPositionsWith(positions.Reader());
    sync.RestoreMemory(memory, false);
    Recorder recorder;

    Tick(sync, gateway, recorder);

    QCOMPARE(recorder.writes, 0);

    gateway.lvars[kPassengerStairsRearState] = kStairsFinalPosition;
    Tick(sync, gateway, recorder);

    QVERIFY(recorder.Opened(GsxDoor::AftPax));
    QCOMPARE(recorder.writes, 1);
}

QTEST_APPLESS_MAIN(GsxDoorSyncTest)

#include "tst_gsx_door_sync.moc"
