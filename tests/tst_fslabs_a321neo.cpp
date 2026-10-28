#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtLogging>
#include "AircraftTicks.h"
#include "FslabsFixtures.h"
#include "TestDoubles.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/infrastructure/aircraft/fslabs/FslabsA321neo.h"
#include "../src/infrastructure/aircraft/fslabs/FslabsAoc.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimBeaconLight = "LIGHT BEACON";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";

    constexpr auto kSmartSwitch = "VC_PED_DRAIMS_1_INT_RAD_Switch";
    constexpr auto kParkingBrake = "FSLA320_ParkBrake";
    constexpr auto kChocks = "FSLA320_Wheel_Chocks";

    constexpr std::array kDoors = {
        "FSLA320_pax_door1", "FSLA320_pax_door2", "FSLA320_pax_door3", "FSLA320_pax_door4",
        "FSLA320_upper_cargo_door", "FSLA320_lower_cargo_door"
    };

    constexpr int kTicksAScreenChangeTakes = 2;
    constexpr int kTicksBeforeTheFirstScreen = 1;
    constexpr int kIdleProbeTicks = 7;
    constexpr int kProbeTicks = kTicksBeforeTheFirstScreen + kIdleProbeTicks;
    constexpr int kTicksBetweenProbes = 10;
    constexpr int kTicksBeforeTheAircraftDeboards = 75;
    constexpr int kTicksARepeatIsHeldBack = 60;
    constexpr int kTicksAnAskCounts = 5;
    constexpr int kStopsBeforeGivingUp = 3;
    constexpr int kTicksAToggleGetsToShowUp = 20;
    constexpr int kTogglesBeforeGivingUp = 3;
    constexpr double kEmptyWeightKg = 45000.0;
    constexpr double kFuelKg = 6100.0;
    constexpr double kPayloadKg = 9300.0;

    const QString kProbeKeys = QStringLiteral("96 117 96");
    const QString kInitCheckKeys = QStringLiteral("96 117 121 112 96");
    const QString kSendOfpKeys = QStringLiteral("96 117 121 113 8 122 125 96");
    const QString kBoardKeys = QStringLiteral("96 117 121 123 125 114 125 96");
    const QString kBoardAlreadyCheckedKeys = QStringLiteral("96 117 121 123 125 125 96");
    const QString kPushbackKeys = QStringLiteral("96 117 121 124 113 125 96");
    const QString kKeysUpToTheWrongPage = QStringLiteral("96 117 121 113");
    const QString kKeysToTheBoardingPage = QStringLiteral("96 117 121 123");
    const QString kKeysUpToBoardNow = QStringLiteral("96 117 121 123 125 114");
    const QString kKeysToTheAtsuPage = QStringLiteral("96 117");
    const QString kKeysToTheAocMenu = QStringLiteral("96 117 121");
    const QString kKeysUpToTheInitPage = QStringLiteral("96 117 121 112");

    QString Joined(const std::vector<int>& keys)
    {
        QStringList parts;
        for (const int key : keys)
        {
            parts.append(QString::number(key));
        }

        return parts.join(QLatin1Char(' '));
    }

    int KeyCountOf(const QString& keys)
    {
        return static_cast<int>(keys.split(QLatin1Char(' ')).size());
    }

    int TicksToPress(const QString& keys)
    {
        return kTicksAScreenChangeTakes * (KeyCountOf(keys) - 1) + 1;
    }

    int TicksForAScriptOf(const QString& keys)
    {
        return TicksToPress(keys) + kTicksAScreenChangeTakes;
    }

    class LogCapture
    {
    public:
        LogCapture() : previous_(qInstallMessageHandler(Collect)) { Lines().clear(); }
        ~LogCapture() { qInstallMessageHandler(previous_); }

        [[nodiscard]] static int CountContaining(const QString& text)
        {
            return static_cast<int>(Lines().filter(text).size());
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

    struct AocFixture
    {
        FakeFslabsMcdu mcdu;
        FakeFslabsMcdu leftMcdu;
        FslabsAoc aoc{mcdu, leftMcdu};
        bool powerIsAsked = true;

        void TickTimes(const int ticks, const bool initIsAwaited = false)
        {
            for (int tick = 0; tick < ticks; ++tick)
            {
                mcdu.Poll();
                leftMcdu.Poll();
                aoc.Advance(initIsAwaited, powerIsAsked);
            }
        }

        void SettleTheProbe()
        {
            TickTimes(kProbeTicks);

            QCOMPARE(Pressed(), kProbeKeys);
            QVERIFY(aoc.IsAlive());

            mcdu.ForgetThePresses();
        }

        void RunTheScript(const AocRequest request, const QString& expectedKeys)
        {
            QVERIFY(aoc.Request(request));

            TickTimes(TicksForAScriptOf(expectedKeys));

            QCOMPARE(Pressed(), expectedKeys);
            QCOMPARE(mcdu.Page(), std::string("mcdu-menu"));
        }

        [[nodiscard]] QString Pressed() const { return Joined(mcdu.keys); }
    };

    struct FslabsFixture
    {
        FslabsFixture()
            : aircraft{&gateway, &status, std::move(mcduOwner), std::move(leftMcduOwner), std::move(groundOwner)}
        {
        }

        FakeVariableGateway gateway;
        AutomationStatus status;
        std::unique_ptr<FakeFslabsMcdu> mcduOwner = std::make_unique<FakeFslabsMcdu>();
        FakeFslabsMcdu& mcdu = *mcduOwner;
        std::unique_ptr<FakeFslabsMcdu> leftMcduOwner = std::make_unique<FakeFslabsMcdu>();
        FakeFslabsMcdu& leftMcdu = *leftMcduOwner;
        std::unique_ptr<FakeFslabsGround> groundOwner = std::make_unique<FakeFslabsGround>();
        FakeFslabsGround& ground = *groundOwner;
        FslabsA321neo aircraft;

        void Tick(const int ticks = 1)
        {
            for (int tick = 0; tick < ticks; ++tick)
            {
                TickAircraft(aircraft, gateway);
            }
        }

        void SetLoaders(const double front, const double rear)
        {
            gateway.lvars[gsx::lvars::kBaggageLoaderFrontState] = front;
            gateway.lvars[gsx::lvars::kBaggageLoaderRearState] = rear;
        }

        void PowerUp()
        {
            static_cast<void>(aircraft.IsPowered());
            Tick(kProbeTicks);

            QVERIFY(aircraft.IsPowered());

            mcdu.ForgetThePresses();
        }

        void ShutEnginesAndBeaconDown()
        {
            gateway.avars[kSimEng1Combustion] = 0.0;
            gateway.avars[kSimEng2Combustion] = 0.0;
            gateway.avars[kSimBeaconLight] = 0.0;
        }

        void SeedMassAndBalance()
        {
            gateway.avars[kSimEmptyWeight] = kEmptyWeightKg;
            gateway.avars[kSimFuelTotalKg] = kFuelKg;
            gateway.avars[kSimTotalWeight] = kEmptyWeightKg + kPayloadKg + kFuelKg;
        }
    };
}

class FslabsA321neoTest final : public QObject
{
    Q_OBJECT

private slots:
    static void theProbeProvesLifeWithTwoPageChanges();
    static void aFrozenFrameIsNeverAlive();
    static void aProofHoldsUntilAKeyFailsToChangeThePage();
    static void aStopOnTheSamePageKeepsTheProof();
    static void aBlankScreenIsNotAliveAndPressesNothing();
    static void anUnreachableMcduPressesNothing();
    static void aBlankScreenOrALostEndpointNeedsAFreshProof();
    static void theOfpScriptPressesTheMeasuredKeysInOrder();
    static void theBoardingScriptPressesBoardNowWhileItIsUnchecked();
    static void theBoardingScriptSkipsBoardNowWhenItIsAlreadyChecked();
    static void theBoardingScriptTakesABlankBoxAsChecked();
    static void theBoardingScriptSkipsBoardNowWhenTheBoxIsAlreadyBlank();
    static void thePushbackScriptPressesTheMeasuredKeysInOrder();
    static void aWrongPageAbortsAfterOneStepAndLeavesTheMenu();
    static void aRepeatedAbortLogsOnce();
    static void anAbortAfterASuccessLogsAgain();
    static void aKeyIsSeenFreshTwoTicksAfterItIsPressed();
    static void anIdleMcduWithAProofIsNeverProbedAgain();
    static void aProbeWithEveryReplyOneTickLateStillProvesTheLife();
    static void anIdleMcduNobodyAsksAboutIsNeverProbed();
    static void aQueuedRequestWithNoAskStillGetsAProbeAndRuns();
    static void anAwaitedInitIsCheckedOnceAndThatProvesTheLife();
    static void aCheckOnAnMcduThatDoesNotAnswerRunsAgain();
    static void theAocLogsWhenTheMcduComesAndGoes();
    static void theAocStaysQuietWhenNothingWantsTheMcdu();
    static void aRunningScriptWaitsForAScreenFetchedAfterItsKey();
    static void theInitIsDoneOnlyWithTheDepartureFilled();
    static void aFlightNumberAloneOnEitherMcduIsNotTheInit();
    static void theInitIsCheckedOnlyWhileTheAdapterAsks();
    static void theRightMcduIsLeftAloneWhileTheInitIsAwaited();
    static void theInitOnTheLeftMcduIsSeenWithoutAKey();
    static void theInitOnTheRightMcduIsSeenWithoutAKey();
    static void anInterruptedProbeLeavesThePilotsPage();
    static void anInterruptedInitCheckLeavesThePilotsPage();
    static void aNewTurnaroundAllowsAnotherInitCheck();
    static void aNewTurnaroundDuringTheFirstCheckLetsItRunAgain();
    static void aRequestIsTakenOnceWhileItIsQueuedOrRunning();
    static void requestsQueueWhileNotAliveAndRunInOrderOnceAlive();
    static void aBlankScreenMidScriptDropsTheScript();
    static void aNewTurnaroundDropsTheQueuedRequestsAndTheInit();
    static void aNewTurnaroundAbortsARunningScriptAndLeavesTheMenu();
    static void aCompletedRequestIsNotQueuedAgainForSixtyTicks();
    static void aRepeatHeldBackLogsOncePerCompletion();
    static void anAbortedRequestIsQueuedAgainAtOnce();
    static void aRequestStoppedThreeTimesIsNoLongerTaken();
    static void aCompletionResetsTheStopCount();
    static void aNewTurnaroundForgetsTheCompletedRequests();

    static void theAdapterDeclaresWhatItDoesThroughTheAocAndTheRightMcdu();
    static void observeDrivesTheMcduEveryTick();
    static void powerFollowsTheAocBeingAlive();
    static void noOneAskingForPowerMeansNoProbe();
    static void askingForPowerRunsTheProbeAndPowersTheAircraft();
    static void askingForPowerEveryTickProbesOnlyUntilTheProof();
    static void aSingleAskProbesOnlyWithinItsWindow();
    static void anAskCountsForFiveObserveTicks();
    static void askingIfTheAircraftIsReadyToPushAsksForPower();
    static void reachabilityFollowsTheMcdu();
    static void eachRequestMemberQueuesItsOwnScriptAndIsAlwaysTaken();
    static void theFlightPlanNeedsTheOfpAndTheInit();
    static void aFlightPlanThatIsNotReadyNeverAsksForTheInit();
    static void plannedValuesComeFromTheSimbriefStatus();
    static void passengersItBoardsComeFromTheGsxLVar();
    static void thePassengerCountIsWatchedFromTheFirstTick();
    static void noPassengerCountIsReportedBeforeTheLVarArrives();
    static void deboardReadinessNeedsSeventyFiveQuietTicks();
    static void readyToPushNeedsPowerEnginesOffAndBeacon();
    static void theAircraftIsHeldByBrakeOrChocks();
    static void chocksStatusIsUnknownUntilTheLVarArrives();
    static void doorsAreUnknownUntilDataArrivesThenFollowTheLVars();
    static void fuelAndZfwComeFromTheSimulatorWeights();
    static void theAdapterNeverWritesFuelOrPayload();
    static void aTouchOnTheRadSideIsTheSmartSwitch();
    static void aNewTurnaroundClearsTheInit();

    static void observePollsTheGroundGatewayEveryTick();
    static void theGsxMenuIsDrivenOnlyWhileTheRemoteControlIsOn();
    static void theForwardHoldClosesWhenTheFrontLoaderLeaves();
    static void aHoldStaysOpenWhileItsLoaderLowersTheBelt();
    static void theAftHoldClosesWhenTheRearLoaderLeaves();
    static void aHoldIsLeftAloneWhileItsLoaderServes();
    static void aHoldIsOnlyToggledWhileItReadsOpen();
    static void aHoldStillOpeningWhenTheLoaderLeavesIsClosedOnceItIsOpen();
    static void aLoaderThatNeverServedClosesNothing();
    static void aLoaderStateThatHasNotArrivedClosesNothing();
    static void aHoldThatNeverReadClosesNothing();
    static void aHoldStillOpenAfterTheWaitIsToggledAgain();
    static void aHoldThatStaysOpenGivesUpAfterThreeTogglesAndLogsOnce();
    static void aHoldThatClosesIsNotToggledAgain();
    static void aLoaderThatComesBackCancelsTheClose();
    static void anUnreachableGroundEndpointTogglesNothing();
    static void aNewTurnaroundForgetsTheServingLoader();
    static void aNewTurnaroundDropsTheCloseThatIsDue();
};

void FslabsA321neoTest::theProbeProvesLifeWithTwoPageChanges()
{
    AocFixture fixture;

    QVERIFY(!fixture.aoc.IsAlive());

    fixture.TickTimes(kProbeTicks);

    QCOMPARE(fixture.Pressed(), kProbeKeys);
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aFrozenFrameIsNeverAlive()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.mcdu.isFrozen = true;

    for (int tick = 0; tick < 40; ++tick)
    {
        fixture.TickTimes(1);

        QVERIFY(!fixture.aoc.IsAlive());
    }

    QVERIFY(fixture.mcdu.keys.size() <= 12);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step")), 0);
}

void FslabsA321neoTest::aProofHoldsUntilAKeyFailsToChangeThePage()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.isFrozen = true;

    fixture.TickTimes(60);

    QVERIFY(fixture.aoc.IsAlive());
    QVERIFY(fixture.mcdu.keys.empty());

    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));
    fixture.TickTimes(TicksToPress(kKeysToTheAtsuPage));

    QCOMPARE(fixture.Pressed(), kKeysToTheAtsuPage);
    QVERIFY(fixture.aoc.IsAlive());

    fixture.TickTimes(kTicksAScreenChangeTakes);

    QVERIFY(!fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aStopOnTheSamePageKeepsTheProof()
{
    AocFixture fixture;
    fixture.mcdu.Route("boarding-2", 114, "boarding-2");
    fixture.SettleTheProbe();

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToBoardNow));

    QCOMPARE(fixture.Pressed(), kKeysUpToBoardNow + QStringLiteral(" 96"));
    QVERIFY(fixture.aoc.IsAlive());

    fixture.mcdu.ForgetThePresses();
    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToBoardNow));

    QCOMPARE(fixture.Pressed(), kKeysUpToBoardNow + QStringLiteral(" 96"));
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aBlankScreenIsNotAliveAndPressesNothing()
{
    AocFixture fixture;
    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);

    fixture.TickTimes(30);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(!fixture.aoc.IsAlive());
}

void FslabsA321neoTest::anUnreachableMcduPressesNothing()
{
    AocFixture fixture;
    fixture.mcdu.reachable = false;

    fixture.TickTimes(30);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(!fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aBlankScreenOrALostEndpointNeedsAFreshProof()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    fixture.TickTimes(kTicksAScreenChangeTakes + 1);

    QVERIFY(!fixture.aoc.IsAlive());

    fixture.mcdu.Show("mcdu-menu");
    fixture.TickTimes(kProbeTicks + kTicksAScreenChangeTakes);

    QCOMPARE(fixture.Pressed(), kProbeKeys);
    QVERIFY(fixture.aoc.IsAlive());

    fixture.mcdu.ForgetThePresses();
    fixture.mcdu.reachable = false;
    fixture.TickTimes(kTicksAScreenChangeTakes + 1);

    QVERIFY(!fixture.aoc.IsAlive());

    fixture.mcdu.reachable = true;
    fixture.TickTimes(kProbeTicks + kTicksAScreenChangeTakes);

    QCOMPARE(fixture.Pressed(), kProbeKeys);
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::theOfpScriptPressesTheMeasuredKeysInOrder()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::SendOfp, kSendOfpKeys);
}

void FslabsA321neoTest::theBoardingScriptPressesBoardNowWhileItIsUnchecked()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::Board, kBoardKeys);
}

void FslabsA321neoTest::theBoardingScriptSkipsBoardNowWhenItIsAlreadyChecked()
{
    AocFixture fixture;
    fixture.mcdu.Route("boarding-1", 125, "boarding-2-checked");
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::Board, kBoardAlreadyCheckedKeys);
}

void FslabsA321neoTest::theBoardingScriptTakesABlankBoxAsChecked()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.mcdu.Route("boarding-2", 114, "boarding-2-checked-blank");
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::Board, kBoardKeys);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step")), 0);
}

void FslabsA321neoTest::theBoardingScriptSkipsBoardNowWhenTheBoxIsAlreadyBlank()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.mcdu.Route("boarding-1", 125, "boarding-2-checked-blank");
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::Board, kBoardAlreadyCheckedKeys);

    QCOMPARE(fixture.mcdu.PressesOf(114), 0);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step")), 0);
}

void FslabsA321neoTest::thePushbackScriptPressesTheMeasuredKeysInOrder()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    fixture.RunTheScript(AocRequest::Pushback, kPushbackKeys);
}

void FslabsA321neoTest::aWrongPageAbortsAfterOneStepAndLeavesTheMenu()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 113, "departure");
    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));

    fixture.TickTimes(TicksForAScriptOf(kKeysUpToTheWrongPage));

    QCOMPARE(fixture.Pressed(), QStringLiteral("96 117 121 113 96"));
    QCOMPARE(fixture.mcdu.Page(), std::string("mcdu-menu"));
    QCOMPARE(fixture.mcdu.PressesOf(125), 0);
}

void FslabsA321neoTest::aRepeatedAbortLogsOnce()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 113, "departure");

    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToTheWrongPage));
    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToTheWrongPage));

    QCOMPARE(fixture.mcdu.PressesOf(113), 2);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step 3")), 1);
}

void FslabsA321neoTest::anAbortAfterASuccessLogsAgain()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 113, "departure");

    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToTheWrongPage));

    QCOMPARE(fixture.Pressed(), QStringLiteral("96 117 121 113 96"));

    fixture.mcdu.Route("aoc-menu", 113, "ofp-data");
    fixture.mcdu.ForgetThePresses();
    fixture.RunTheScript(AocRequest::SendOfp, kSendOfpKeys);

    fixture.powerIsAsked = false;
    fixture.TickTimes(kTicksARepeatIsHeldBack + 1);
    fixture.mcdu.Route("aoc-menu", 113, "departure");
    QVERIFY(fixture.aoc.Request(AocRequest::SendOfp));
    fixture.TickTimes(TicksForAScriptOf(kKeysUpToTheWrongPage));

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step 3")), 2);
}

void FslabsA321neoTest::aKeyIsSeenFreshTwoTicksAfterItIsPressed()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    QVERIFY(fixture.aoc.Request(AocRequest::Board));

    fixture.TickTimes(1);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96"));

    fixture.TickTimes(kTicksAScreenChangeTakes - 1);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96"));

    fixture.TickTimes(1);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96 117"));
}

void FslabsA321neoTest::anIdleMcduWithAProofIsNeverProbedAgain()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    fixture.TickTimes(120);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aProbeWithEveryReplyOneTickLateStillProvesTheLife()
{
    AocFixture fixture;
    fixture.mcdu.replyDelayTicks = 1;

    fixture.TickTimes(60);

    QVERIFY2(fixture.mcdu.pollsAtKey[2] - fixture.mcdu.pollsAtKey[1] > kTicksAScreenChangeTakes,
             "the replies were not late");
    QCOMPARE(fixture.Pressed(), kProbeKeys);
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::anIdleMcduNobodyAsksAboutIsNeverProbed()
{
    AocFixture fixture;
    fixture.powerIsAsked = false;

    fixture.TickTimes(60);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(!fixture.aoc.IsAlive());
}

void FslabsA321neoTest::aQueuedRequestWithNoAskStillGetsAProbeAndRuns()
{
    AocFixture fixture;
    fixture.powerIsAsked = false;

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(kProbeTicks + TicksForAScriptOf(kBoardKeys));

    QCOMPARE(fixture.Pressed(), kProbeKeys + QLatin1Char(' ') + kBoardKeys);
    QCOMPARE(fixture.mcdu.Page(), std::string("mcdu-menu"));
}

void FslabsA321neoTest::anAwaitedInitIsCheckedOnceAndThatProvesTheLife()
{
    AocFixture fixture;
    fixture.powerIsAsked = false;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");

    fixture.TickTimes(kTicksBeforeTheFirstScreen + TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(fixture.aoc.IsAlive());
    QVERIFY(!fixture.aoc.IsInitDone());

    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(120, true);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(!fixture.aoc.IsInitDone());
}

void FslabsA321neoTest::aCheckOnAnMcduThatDoesNotAnswerRunsAgain()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.powerIsAsked = false;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");
    fixture.mcdu.isFrozen = true;

    fixture.TickTimes(kTicksBeforeTheFirstScreen + TicksForAScriptOf(kKeysToTheAtsuPage), true);

    QCOMPARE(fixture.Pressed(), kKeysToTheAtsuPage);
    QVERIFY(!fixture.aoc.IsAlive());

    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(kTicksBetweenProbes, true);

    QCOMPARE(fixture.Pressed(), kKeysToTheAtsuPage);
    QVERIFY(!fixture.aoc.IsAlive());
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step")), 0);

    fixture.mcdu.ForgetThePresses();
    fixture.mcdu.isFrozen = false;
    fixture.TickTimes(kTicksBetweenProbes + TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(fixture.aoc.IsAlive());

    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(120, true);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(fixture.aoc.IsAlive());
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step")), 0);
}

void FslabsA321neoTest::theAocLogsWhenTheMcduComesAndGoes()
{
    const LogCapture log;
    AocFixture fixture;

    fixture.SettleTheProbe();
    fixture.TickTimes(5);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("answers its keys")), 1);

    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    fixture.TickTimes(5);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("off or unpowered")), 1);
}

void FslabsA321neoTest::theAocStaysQuietWhenNothingWantsTheMcdu()
{
    const LogCapture log;
    AocFixture fixture;

    fixture.SettleTheProbe();
    fixture.powerIsAsked = false;
    fixture.TickTimes(40);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("off or unpowered")), 0);

    fixture.powerIsAsked = true;
    fixture.TickTimes(30);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("off or unpowered")), 0);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("answers its keys")), 1);
}

void FslabsA321neoTest::aRunningScriptWaitsForAScreenFetchedAfterItsKey()
{
    AocFixture fixture;
    fixture.TickTimes(kTicksBeforeTheFirstScreen + 1);
    QCOMPARE(fixture.Pressed(), QStringLiteral("96"));

    fixture.mcdu.holdsTheScreen = true;
    fixture.TickTimes(5);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96"));

    fixture.mcdu.holdsTheScreen = false;
    fixture.TickTimes(1);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96"));

    fixture.TickTimes(1);

    QCOMPARE(fixture.Pressed(), QStringLiteral("96 117"));
}

void FslabsA321neoTest::theInitIsDoneOnlyWithTheDepartureFilled()
{
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");
    fixture.SettleTheProbe();

    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(!fixture.aoc.IsInitDone());

    fixture.mcdu.Show("aoc-init");
    fixture.TickTimes(kTicksAScreenChangeTakes, true);

    QVERIFY(fixture.aoc.IsInitDone());

    fixture.mcdu.Show("aoc-init-before-init");
    fixture.TickTimes(60, true);

    QVERIFY(fixture.aoc.IsInitDone());
}

void FslabsA321neoTest::aFlightNumberAloneOnEitherMcduIsNotTheInit()
{
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-flight-number-only");
    fixture.leftMcdu.Show("aoc-init-flight-number-only");
    fixture.SettleTheProbe();

    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(!fixture.aoc.IsInitDone());

    fixture.mcdu.Show("aoc-init-flight-number-only");
    fixture.TickTimes(60, true);

    QVERIFY(!fixture.aoc.IsInitDone());
}

void FslabsA321neoTest::theInitIsCheckedOnlyWhileTheAdapterAsks()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    fixture.TickTimes(60, false);

    QCOMPARE(fixture.mcdu.PressesOf(112), 0);

    fixture.TickTimes(20, true);

    QVERIFY(fixture.mcdu.PressesOf(112) > 0);
}

void FslabsA321neoTest::theRightMcduIsLeftAloneWhileTheInitIsAwaited()
{
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");

    fixture.TickTimes(200, true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(!fixture.aoc.IsInitDone());
    QVERIFY(fixture.aoc.IsAlive());
}

void FslabsA321neoTest::theInitOnTheLeftMcduIsSeenWithoutAKey()
{
    AocFixture fixture;
    fixture.powerIsAsked = false;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");
    fixture.leftMcdu.Show("aoc-init");

    fixture.TickTimes(60, true);

    QVERIFY(fixture.aoc.IsInitDone());
    QVERIFY(fixture.leftMcdu.keys.empty());
    QVERIFY(fixture.mcdu.keys.empty());
}

void FslabsA321neoTest::theInitOnTheRightMcduIsSeenWithoutAKey()
{
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");
    fixture.SettleTheProbe();
    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
    QVERIFY(!fixture.aoc.IsInitDone());

    fixture.mcdu.ForgetThePresses();
    fixture.mcdu.Show("aoc-init");
    fixture.TickTimes(60, true);

    QVERIFY(fixture.aoc.IsInitDone());
    QVERIFY(fixture.mcdu.keys.empty());
}

void FslabsA321neoTest::anInterruptedProbeLeavesThePilotsPage()
{
    AocFixture fixture;
    fixture.mcdu.Route("mcdu-menu", 117, "departure");

    fixture.TickTimes(kProbeTicks);

    QCOMPARE(fixture.Pressed(), kKeysToTheAtsuPage);
    QCOMPARE(fixture.mcdu.Page(), std::string("departure"));
    QVERIFY(!fixture.aoc.IsAlive());
}

void FslabsA321neoTest::anInterruptedInitCheckLeavesThePilotsPage()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "departure");
    fixture.SettleTheProbe();

    fixture.TickTimes(200, true);

    QCOMPARE(fixture.Pressed(), kKeysUpToTheInitPage);
    QCOMPARE(fixture.mcdu.Page(), std::string("departure"));
    QVERIFY(fixture.aoc.IsAlive());
    QVERIFY(!fixture.aoc.IsInitDone());
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped at step 3")), 1);
}

void FslabsA321neoTest::aNewTurnaroundAllowsAnotherInitCheck()
{
    AocFixture fixture;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");
    fixture.SettleTheProbe();
    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.mcdu.PressesOf(112), 1);

    fixture.TickTimes(60, true);

    QCOMPARE(fixture.mcdu.PressesOf(112), 1);

    fixture.aoc.ResetForNewTurnaround();
    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.mcdu.PressesOf(112), 2);
}

void FslabsA321neoTest::aNewTurnaroundDuringTheFirstCheckLetsItRunAgain()
{
    AocFixture fixture;
    fixture.powerIsAsked = false;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");

    fixture.TickTimes(kTicksBeforeTheFirstScreen + TicksToPress(kKeysToTheAocMenu), true);

    QCOMPARE(fixture.Pressed(), kKeysToTheAocMenu);
    QVERIFY(fixture.aoc.IsAlive());

    fixture.aoc.ResetForNewTurnaround();
    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);

    QCOMPARE(fixture.Pressed(), kInitCheckKeys);
}

void FslabsA321neoTest::aRequestIsTakenOnceWhileItIsQueuedOrRunning()
{
    AocFixture fixture;
    fixture.SettleTheProbe();

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));

    fixture.TickTimes(3);

    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    QVERIFY(fixture.aoc.Request(AocRequest::Pushback));

    fixture.TickTimes(30);

    QCOMPARE(fixture.mcdu.PressesOf(123), 1);

    fixture.TickTimes(kTicksARepeatIsHeldBack + 1);

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
}

void FslabsA321neoTest::requestsQueueWhileNotAliveAndRunInOrderOnceAlive()
{
    AocFixture fixture;
    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    QVERIFY(fixture.aoc.Request(AocRequest::Pushback));
    fixture.TickTimes(15);

    QVERIFY(fixture.mcdu.keys.empty());

    fixture.mcdu.Show("mcdu-menu");
    fixture.TickTimes(kProbeTicks + TicksForAScriptOf(kBoardKeys) + TicksForAScriptOf(kPushbackKeys));

    QCOMPARE(fixture.Pressed(), kProbeKeys + QLatin1Char(' ') + kBoardKeys + QLatin1Char(' ') + kPushbackKeys);
}

void FslabsA321neoTest::aBlankScreenMidScriptDropsTheScript()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(3);
    const QString pressedSoFar = fixture.Pressed();

    QCOMPARE(pressedSoFar, QStringLiteral("96 117"));

    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    fixture.TickTimes(10);

    QCOMPARE(fixture.Pressed(), pressedSoFar);

    fixture.mcdu.Show("mcdu-menu");
    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(kProbeTicks + TicksToPress(kKeysToTheBoardingPage));

    QCOMPARE(fixture.mcdu.PressesOf(123), 0);
}

void FslabsA321neoTest::aNewTurnaroundDropsTheQueuedRequestsAndTheInit()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.TickTimes(TicksForAScriptOf(kInitCheckKeys), true);
    QVERIFY(fixture.aoc.IsInitDone());
    QVERIFY(fixture.aoc.Request(AocRequest::Board));

    fixture.aoc.ResetForNewTurnaround();
    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(TicksToPress(kKeysToTheBoardingPage));

    QVERIFY(!fixture.aoc.IsInitDone());
    QCOMPARE(fixture.mcdu.PressesOf(123), 0);
    QVERIFY(fixture.aoc.Request(AocRequest::Board));
}

void FslabsA321neoTest::aNewTurnaroundAbortsARunningScriptAndLeavesTheMenu()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(TicksToPress(kKeysToTheBoardingPage));
    QCOMPARE(fixture.Pressed(), kKeysToTheBoardingPage);

    fixture.aoc.ResetForNewTurnaround();

    QCOMPARE(fixture.Pressed(), kKeysToTheBoardingPage + QStringLiteral(" 96"));
    QCOMPARE(fixture.mcdu.Page(), std::string("mcdu-menu"));

    fixture.TickTimes(5);

    QCOMPARE(fixture.mcdu.PressesOf(125), 0);
}

void FslabsA321neoTest::aCompletedRequestIsNotQueuedAgainForSixtyTicks()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.powerIsAsked = false;
    fixture.RunTheScript(AocRequest::Board, kBoardKeys);
    fixture.mcdu.ForgetThePresses();

    fixture.TickTimes(10);

    QVERIFY(!fixture.aoc.Request(AocRequest::Board));

    fixture.TickTimes(5);

    QVERIFY(fixture.mcdu.keys.empty());

    fixture.TickTimes(kTicksARepeatIsHeldBack - 15);

    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    QVERIFY(fixture.mcdu.keys.empty());

    fixture.TickTimes(1);

    QVERIFY(fixture.aoc.Request(AocRequest::Board));

    fixture.TickTimes(TicksForAScriptOf(kBoardKeys));

    QCOMPARE(fixture.Pressed(), kBoardKeys);
}

void FslabsA321neoTest::aRepeatHeldBackLogsOncePerCompletion()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.powerIsAsked = false;
    fixture.RunTheScript(AocRequest::Board, kBoardKeys);

    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(10);
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(10);
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("repeat request is not sent")), 1);

    fixture.TickTimes(kTicksARepeatIsHeldBack);
    fixture.mcdu.ForgetThePresses();

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
    fixture.TickTimes(TicksForAScriptOf(kBoardKeys));

    QCOMPARE(fixture.Pressed(), kBoardKeys);
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("repeat request is not sent")), 2);
}

void FslabsA321neoTest::anAbortedRequestIsQueuedAgainAtOnce()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 123, "departure");
    QVERIFY(fixture.aoc.Request(AocRequest::Board));

    fixture.TickTimes(TicksForAScriptOf(kKeysToTheBoardingPage));

    QCOMPARE(fixture.Pressed(), kKeysToTheBoardingPage + QStringLiteral(" 96"));
    QCOMPARE(fixture.mcdu.PressesOf(125), 0);

    fixture.mcdu.Route("aoc-menu", 123, "boarding-1");
    fixture.mcdu.ForgetThePresses();

    fixture.RunTheScript(AocRequest::Board, kBoardKeys);
}

void FslabsA321neoTest::aRequestStoppedThreeTimesIsNoLongerTaken()
{
    const LogCapture log;
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 123, "departure");

    for (int run = 0; run < kStopsBeforeGivingUp; ++run)
    {
        QVERIFY(fixture.aoc.Request(AocRequest::Board));
        fixture.TickTimes(TicksForAScriptOf(kKeysToTheBoardingPage));
    }

    QCOMPARE(fixture.mcdu.PressesOf(123), kStopsBeforeGivingUp);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped three times in a row")), 1);
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));

    fixture.mcdu.ForgetThePresses();
    fixture.TickTimes(200);

    QVERIFY(fixture.mcdu.keys.empty());
    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    QVERIFY(fixture.aoc.Request(AocRequest::Pushback));
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("stopped three times in a row")), 1);

    fixture.aoc.ResetForNewTurnaround();

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
}

void FslabsA321neoTest::aCompletionResetsTheStopCount()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.mcdu.Route("aoc-menu", 123, "departure");

    for (int run = 0; run < kStopsBeforeGivingUp - 1; ++run)
    {
        QVERIFY(fixture.aoc.Request(AocRequest::Board));
        fixture.TickTimes(TicksForAScriptOf(kKeysToTheBoardingPage));
    }

    fixture.mcdu.Route("aoc-menu", 123, "boarding-1");
    fixture.mcdu.ForgetThePresses();
    fixture.RunTheScript(AocRequest::Board, kBoardKeys);
    fixture.TickTimes(kTicksARepeatIsHeldBack + 1);
    fixture.mcdu.Route("aoc-menu", 123, "departure");

    for (int run = 0; run < kStopsBeforeGivingUp - 1; ++run)
    {
        QVERIFY(fixture.aoc.Request(AocRequest::Board));
        fixture.TickTimes(TicksForAScriptOf(kKeysToTheBoardingPage));
    }

    QVERIFY(fixture.aoc.Request(AocRequest::Board));
}

void FslabsA321neoTest::aNewTurnaroundForgetsTheCompletedRequests()
{
    AocFixture fixture;
    fixture.SettleTheProbe();
    fixture.RunTheScript(AocRequest::Board, kBoardKeys);

    QVERIFY(!fixture.aoc.Request(AocRequest::Board));
    QVERIFY(fixture.aoc.Request(AocRequest::Pushback));

    fixture.aoc.ResetForNewTurnaround();
    fixture.mcdu.ForgetThePresses();

    fixture.RunTheScript(AocRequest::Board, kBoardKeys);
}

void FslabsA321neoTest::theAdapterDeclaresWhatItDoesThroughTheAocAndTheRightMcdu()
{
    const FslabsFixture fixture;

    QVERIFY(fixture.aircraft.LoadsItsFlightPlanThroughTheAoc());
    QVERIFY(fixture.aircraft.WorksThroughTheRightMcdu());
    QCOMPARE(fixture.aircraft.GetRefuelMethod(), RefuelBy::Gsx);
    QCOMPARE(fixture.aircraft.GetBoardMethod(), BoardBy::Self);
    QVERIFY(fixture.aircraft.SupportsStairsOrJetways());
    QVERIFY(!fixture.aircraft.CompletesPushbackViaInterruptMenu());
    QVERIFY(!fixture.aircraft.IsCargoVariant());
    QVERIFY(fixture.aircraft.Rules().empty());
}

void FslabsA321neoTest::observeDrivesTheMcduEveryTick()
{
    FslabsFixture fixture;

    fixture.Tick(7);

    QCOMPARE(fixture.mcdu.polls, 7);
    QCOMPARE(fixture.leftMcdu.polls, 7);
}

void FslabsA321neoTest::powerFollowsTheAocBeingAlive()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsPowered());

    fixture.Tick(kProbeTicks);

    QVERIFY(fixture.aircraft.IsPowered());

    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    fixture.Tick(kTicksAScreenChangeTakes);

    QVERIFY(!fixture.aircraft.IsPowered());
}

void FslabsA321neoTest::noOneAskingForPowerMeansNoProbe()
{
    FslabsFixture fixture;

    fixture.Tick(60);

    QVERIFY(fixture.mcdu.keys.empty());
    QCOMPARE(fixture.mcdu.polls, 60);
}

void FslabsA321neoTest::askingForPowerRunsTheProbeAndPowersTheAircraft()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsPowered());

    fixture.Tick(kProbeTicks);

    QCOMPARE(Joined(fixture.mcdu.keys), kProbeKeys);
    QVERIFY(fixture.aircraft.IsPowered());
}

void FslabsA321neoTest::askingForPowerEveryTickProbesOnlyUntilTheProof()
{
    FslabsFixture fixture;

    for (int tick = 0; tick < 120; ++tick)
    {
        static_cast<void>(fixture.aircraft.IsPowered());
        fixture.Tick(1);

        if (tick >= kProbeTicks)
        {
            QVERIFY(fixture.aircraft.IsPowered());
        }
    }

    QCOMPARE(Joined(fixture.mcdu.keys), kProbeKeys);
}

void FslabsA321neoTest::aSingleAskProbesOnlyWithinItsWindow()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsPowered());

    fixture.Tick(60);

    QCOMPARE(Joined(fixture.mcdu.keys), kProbeKeys);
    QVERIFY(fixture.aircraft.IsPowered());
}

void FslabsA321neoTest::anAskCountsForFiveObserveTicks()
{
    FslabsFixture withinTheWindow;
    withinTheWindow.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    static_cast<void>(withinTheWindow.aircraft.IsPowered());
    withinTheWindow.Tick(kTicksAnAskCounts - kTicksAScreenChangeTakes);
    withinTheWindow.mcdu.Show("mcdu-menu");
    withinTheWindow.Tick(kTicksAScreenChangeTakes);

    QCOMPARE(Joined(withinTheWindow.mcdu.keys), QStringLiteral("96"));

    FslabsFixture pastTheWindow;
    pastTheWindow.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    static_cast<void>(pastTheWindow.aircraft.IsPowered());
    pastTheWindow.Tick(kTicksAnAskCounts - kTicksAScreenChangeTakes + 1);
    pastTheWindow.mcdu.Show("mcdu-menu");
    pastTheWindow.Tick(10);

    QVERIFY(pastTheWindow.mcdu.keys.empty());
}

void FslabsA321neoTest::askingIfTheAircraftIsReadyToPushAsksForPower()
{
    FslabsFixture fixture;
    fixture.ShutEnginesAndBeaconDown();
    fixture.gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(!fixture.aircraft.IsReadyToPush());

    fixture.Tick(kProbeTicks);

    QCOMPARE(Joined(fixture.mcdu.keys), kProbeKeys);
    QVERIFY(fixture.aircraft.IsReadyToPush());
}

void FslabsA321neoTest::reachabilityFollowsTheMcdu()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsReachable());

    fixture.Tick(kTicksAScreenChangeTakes);

    QVERIFY(fixture.aircraft.IsReachable());

    fixture.leftMcdu.reachable = false;
    fixture.Tick(kTicksAScreenChangeTakes);

    QVERIFY(fixture.aircraft.IsReachable());

    fixture.mcdu.reachable = false;
    fixture.Tick(1);

    QVERIFY(!fixture.aircraft.IsReachable());
}

void FslabsA321neoTest::eachRequestMemberQueuesItsOwnScriptAndIsAlwaysTaken()
{
    FslabsFixture refuel;
    refuel.PowerUp();
    QVERIFY(refuel.aircraft.RequestRefuelingItself());
    QVERIFY(refuel.aircraft.RequestRefuelingItself());
    refuel.Tick(TicksForAScriptOf(kSendOfpKeys));

    QCOMPARE(Joined(refuel.mcdu.keys), kSendOfpKeys);

    FslabsFixture boarding;
    boarding.PowerUp();
    QVERIFY(boarding.aircraft.RequestBoardingItself());
    boarding.Tick(TicksForAScriptOf(kBoardKeys));

    QCOMPARE(Joined(boarding.mcdu.keys), kBoardKeys);

    FslabsFixture pushback;
    pushback.PowerUp();
    QVERIFY(pushback.aircraft.RequestPushbackItself());
    pushback.Tick(TicksForAScriptOf(kPushbackKeys));

    QCOMPARE(Joined(pushback.mcdu.keys), kPushbackKeys);
}

void FslabsA321neoTest::theFlightPlanNeedsTheOfpAndTheInit()
{
    FslabsFixture fixture;
    fixture.status.flightPlanStatus = FlightPlanStatus::Ready;
    fixture.mcdu.Route("aoc-menu", 112, "aoc-init-before-init");

    fixture.Tick(40);

    QCOMPARE(fixture.mcdu.PressesOf(112), 1);
    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());

    fixture.leftMcdu.Show("aoc-init");
    fixture.Tick(kTicksAScreenChangeTakes);

    QVERIFY(fixture.aircraft.IsFlightPlanLoaded());
    QCOMPARE(fixture.mcdu.PressesOf(112), 1);
}

void FslabsA321neoTest::aFlightPlanThatIsNotReadyNeverAsksForTheInit()
{
    FslabsFixture fixture;
    fixture.status.flightPlanStatus = FlightPlanStatus::Idle;

    fixture.Tick(60);

    QCOMPARE(fixture.mcdu.PressesOf(112), 0);
    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());
}

void FslabsA321neoTest::plannedValuesComeFromTheSimbriefStatus()
{
    FslabsFixture fixture;
    fixture.status.plannedFuelKg = 6100.0;
    fixture.status.plannedZfwKg = 67400.0;
    fixture.status.plannedPassengers = 110;

    QCOMPARE(fixture.aircraft.GetPlannedFuelKg(), 6100.0);
    QCOMPARE(fixture.aircraft.GetPlannedZfwKg(), 67400.0);
    QCOMPARE(fixture.aircraft.GetPlannedPassengers(), 110);
}

void FslabsA321neoTest::passengersItBoardsComeFromTheGsxLVar()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.GetPassengersItBoards().has_value());

    fixture.gateway.lvars[gsx::lvars::kNumPassengers] = 0.0;

    QVERIFY(!fixture.aircraft.GetPassengersItBoards().has_value());

    fixture.gateway.lvars[gsx::lvars::kNumPassengers] = 105.0;

    QCOMPARE(fixture.aircraft.GetPassengersItBoards().value_or(-1), 105);
}

void FslabsA321neoTest::thePassengerCountIsWatchedFromTheFirstTick()
{
    FslabsFixture fixture;
    fixture.gateway.arrivesATickAfterItIsAsked = true;
    fixture.gateway.lvars[gsx::lvars::kNumPassengers] = 105.0;

    fixture.Tick();
    fixture.gateway.DeliverWhatWasAsked();

    QCOMPARE(fixture.aircraft.GetPassengersItBoards().value_or(-1), 105);
}

void FslabsA321neoTest::noPassengerCountIsReportedBeforeTheLVarArrives()
{
    FslabsFixture fixture;
    fixture.gateway.arrivesATickAfterItIsAsked = true;
    fixture.gateway.lvars[gsx::lvars::kNumPassengers] = 105.0;

    QVERIFY(!fixture.aircraft.GetPassengersItBoards().has_value());

    fixture.gateway.DeliverWhatWasAsked();

    QCOMPARE(fixture.aircraft.GetPassengersItBoards().value_or(-1), 105);
}

void FslabsA321neoTest::deboardReadinessNeedsSeventyFiveQuietTicks()
{
    FslabsFixture fixture;
    fixture.ShutEnginesAndBeaconDown();

    fixture.Tick(kTicksBeforeTheAircraftDeboards - 1);

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());

    fixture.Tick(1);

    QVERIFY(fixture.aircraft.IsReadyToDeboard());

    fixture.gateway.avars[kSimBeaconLight] = 1.0;
    fixture.Tick(1);

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());

    fixture.gateway.avars[kSimBeaconLight] = 0.0;
    fixture.Tick(kTicksBeforeTheAircraftDeboards - 1);

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());

    fixture.gateway.avars[kSimEng2Combustion] = 1.0;
    fixture.Tick(1);
    fixture.gateway.avars[kSimEng2Combustion] = 0.0;
    fixture.Tick(kTicksBeforeTheAircraftDeboards - 1);

    QVERIFY(!fixture.aircraft.IsReadyToDeboard());

    fixture.Tick(1);

    QVERIFY(fixture.aircraft.IsReadyToDeboard());
}

void FslabsA321neoTest::readyToPushNeedsPowerEnginesOffAndBeacon()
{
    FslabsFixture fixture;
    fixture.ShutEnginesAndBeaconDown();
    fixture.PowerUp();

    QVERIFY(!fixture.aircraft.IsReadyToPush());

    fixture.gateway.avars[kSimBeaconLight] = 1.0;

    QVERIFY(fixture.aircraft.IsReadyToPush());

    fixture.gateway.avars[kSimEng1Combustion] = 1.0;

    QVERIFY(!fixture.aircraft.IsReadyToPush());
    QVERIFY(fixture.aircraft.IsEngineRunning());

    fixture.gateway.avars[kSimEng1Combustion] = 0.0;
    fixture.mcdu.Show(FakeFslabsMcdu::kBlankPage);
    fixture.Tick(kTicksAScreenChangeTakes);

    QVERIFY(!fixture.aircraft.IsReadyToPush());
}

void FslabsA321neoTest::theAircraftIsHeldByBrakeOrChocks()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsHeldInPlace());
    QVERIFY(!fixture.aircraft.IsParkingBrakeSet());

    fixture.gateway.lvars[kChocks] = 1.0;

    QVERIFY(fixture.aircraft.IsHeldInPlace());
    QVERIFY(!fixture.aircraft.IsParkingBrakeSet());

    fixture.gateway.lvars[kChocks] = 0.0;
    fixture.gateway.lvars[kParkingBrake] = 1.0;

    QVERIFY(fixture.aircraft.IsHeldInPlace());
    QVERIFY(fixture.aircraft.IsParkingBrakeSet());
}

void FslabsA321neoTest::chocksStatusIsUnknownUntilTheLVarArrives()
{
    FslabsFixture fixture;

    QCOMPARE(fixture.aircraft.GetChocksStatus(), std::optional(ChocksStatus::Unknown));

    fixture.gateway.lvars[kChocks] = 0.0;

    QCOMPARE(fixture.aircraft.GetChocksStatus(), std::optional(ChocksStatus::Removed));

    fixture.gateway.lvars[kChocks] = 1.0;

    QCOMPARE(fixture.aircraft.GetChocksStatus(), std::optional(ChocksStatus::Placed));
}

void FslabsA321neoTest::doorsAreUnknownUntilDataArrivesThenFollowTheLVars()
{
    FslabsFixture fixture;

    QCOMPARE(fixture.aircraft.GetDoorStatus(), DoorStatus::Unknown);

    for (const char* const door : kDoors)
    {
        fixture.gateway.lvars[door] = 0.0;
    }

    QCOMPARE(fixture.aircraft.GetDoorStatus(), DoorStatus::AllClosed);

    for (const char* const door : kDoors)
    {
        fixture.gateway.lvars[door] = 1.0;

        QCOMPARE(fixture.aircraft.GetDoorStatus(), DoorStatus::AnyOpen);

        fixture.gateway.lvars[door] = 0.0;
    }

    fixture.gateway.lvars.erase(kDoors.back());

    QCOMPARE(fixture.aircraft.GetDoorStatus(), DoorStatus::Unknown);
}

void FslabsA321neoTest::fuelAndZfwComeFromTheSimulatorWeights()
{
    FslabsFixture fixture;

    QCOMPARE(fixture.aircraft.GetCurrentZfwKg(), 0.0);

    fixture.SeedMassAndBalance();

    QCOMPARE(fixture.aircraft.GetEmptyZfwKg(), kEmptyWeightKg);
    QCOMPARE(fixture.aircraft.GetCurrentFuelKg(), kFuelKg);
    QCOMPARE(fixture.aircraft.GetCurrentZfwKg(), kEmptyWeightKg + kPayloadKg);
}

void FslabsA321neoTest::theAdapterNeverWritesFuelOrPayload()
{
    FslabsFixture fixture;
    fixture.SeedMassAndBalance();

    fixture.aircraft.SetCurrentFuelKg(9000.0);
    fixture.aircraft.SetCurrentZfwKg(70000.0);
    fixture.Tick(5);

    QCOMPARE(fixture.gateway.setAVarCalls, 0);
    QCOMPARE(fixture.gateway.setLVarCalls, 0);
}

void FslabsA321neoTest::aTouchOnTheRadSideIsTheSmartSwitch()
{
    FslabsFixture fixture;

    QVERIFY(std::ranges::find(fixture.gateway.fastRefreshNames, std::string(kSmartSwitch))
            != fixture.gateway.fastRefreshNames.end());
    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitch] = {10.0, 10.0, true};

    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitch] = {20.0, 20.0, true};

    QVERIFY(!fixture.aircraft.ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitch] = {0.0, 10.0, true};

    QVERIFY(fixture.aircraft.ConsumeSmartSwitch());
    QCOMPARE(fixture.gateway.setLVarCalls, 0);
}

void FslabsA321neoTest::aNewTurnaroundClearsTheInit()
{
    FslabsFixture fixture;
    fixture.status.flightPlanStatus = FlightPlanStatus::Ready;
    fixture.Tick(40);

    QVERIFY(fixture.aircraft.IsFlightPlanLoaded());

    fixture.aircraft.OnTurnaroundStarted();

    QVERIFY(!fixture.aircraft.IsFlightPlanLoaded());
}

void FslabsA321neoTest::theGsxMenuIsDrivenOnlyWhileTheRemoteControlIsOn()
{
    FslabsFixture fixture;

    QVERIFY(!fixture.aircraft.IsDrivingTheGsxMenu());

    fixture.gateway.lvars[gsx::lvars::kSetRemoteControl] = 0.0;

    QVERIFY(!fixture.aircraft.IsDrivingTheGsxMenu());

    fixture.gateway.lvars[gsx::lvars::kSetRemoteControl] = 1.0;

    QVERIFY(fixture.aircraft.IsDrivingTheGsxMenu());

    fixture.gateway.lvars[gsx::lvars::kSetRemoteControl] = 0.0;

    QVERIFY(!fixture.aircraft.IsDrivingTheGsxMenu());
}

void FslabsA321neoTest::observePollsTheGroundGatewayEveryTick()
{
    FslabsFixture fixture;

    fixture.Tick(3);

    QCOMPARE(fixture.ground.polls, 3);
}

void FslabsA321neoTest::theForwardHoldClosesWhenTheFrontLoaderLeaves()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick();

    QCOMPARE(fixture.ground.toggles, std::vector{FslabsHold::Forward});
}

void FslabsA321neoTest::aHoldStaysOpenWhileItsLoaderLowersTheBelt()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, gsx::states::kLoaderLoading);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderFinishing, gsx::states::kLoaderFinishing);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRetracting, gsx::states::kLoaderRetracting);
    fixture.Tick(3);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.SetLoaders(gsx::states::kLoaderRemoving, gsx::states::kLoaderRetracting);
    fixture.Tick();

    QCOMPARE(fixture.ground.toggles, std::vector{FslabsHold::Forward});
}

void FslabsA321neoTest::theAftHoldClosesWhenTheRearLoaderLeaves()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;
    fixture.SetLoaders(0.0, gsx::states::kLoaderLoading);
    fixture.Tick(3);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.SetLoaders(0.0, gsx::states::kLoaderRemoving);
    fixture.Tick();

    QCOMPARE(fixture.ground.toggles, std::vector{FslabsHold::Aft});
}

void FslabsA321neoTest::aHoldIsLeftAloneWhileItsLoaderServes()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;

    for (const double serving : {gsx::states::kLoaderWaitingForDoor, gsx::states::kLoaderUnloading,
                                 gsx::states::kLoaderInPosition, gsx::states::kLoaderLoading})
    {
        fixture.SetLoaders(serving, serving);
        fixture.Tick(5);
    }

    QVERIFY(fixture.ground.toggles.empty());
}

void FslabsA321neoTest::aHoldIsOnlyToggledWhileItReadsOpen()
{
    for (const FslabsHoldState state : {FslabsHoldState::Closed, FslabsHoldState::Opening, FslabsHoldState::Closing})
    {
        FslabsFixture fixture;
        fixture.ground.forwardHold = state;
        fixture.ground.aftHold = state;
        fixture.SetLoaders(gsx::states::kLoaderLoading, gsx::states::kLoaderLoading);
        fixture.Tick(3);
        fixture.SetLoaders(gsx::states::kLoaderRemoving, gsx::states::kLoaderRemoving);
        fixture.Tick(kTicksAToggleGetsToShowUp * 2);

        QVERIFY2(fixture.ground.toggles.empty(), qPrintable(QString::number(static_cast<int>(state))));
    }
}

void FslabsA321neoTest::aHoldStillOpeningWhenTheLoaderLeavesIsClosedOnceItIsOpen()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Opening;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(5);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.Tick();

    QCOMPARE(fixture.ground.toggles, std::vector{FslabsHold::Forward});
}

void FslabsA321neoTest::aLoaderThatNeverServedClosesNothing()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;

    for (const double state : {0.0, gsx::states::kVehicleApproaching, gsx::states::kLoaderFinishing,
                               gsx::states::kLoaderRetracting, gsx::states::kLoaderRemoving})
    {
        fixture.SetLoaders(state, state);
        fixture.Tick(5);
    }

    QVERIFY(fixture.ground.toggles.empty());
}

void FslabsA321neoTest::aLoaderStateThatHasNotArrivedClosesNothing()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.ground.aftHold = FslabsHoldState::Open;

    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());
}

void FslabsA321neoTest::aHoldThatNeverReadClosesNothing()
{
    FslabsFixture fixture;
    fixture.SetLoaders(gsx::states::kLoaderLoading, gsx::states::kLoaderLoading);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, gsx::states::kLoaderRemoving);
    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());
}

void FslabsA321neoTest::aHoldStillOpenAfterTheWaitIsToggledAgain()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick();

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);

    fixture.Tick(kTicksAToggleGetsToShowUp - 1);

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);

    fixture.Tick();

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 2);
    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Aft), 0);
}

void FslabsA321neoTest::aHoldThatStaysOpenGivesUpAfterThreeTogglesAndLogsOnce()
{
    const LogCapture log;
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(kTicksAToggleGetsToShowUp * (kTogglesBeforeGivingUp - 1) + 1);

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), kTogglesBeforeGivingUp);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("still open after")), 0);

    fixture.Tick(kTicksAToggleGetsToShowUp - 1);

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("still open after")), 0);

    fixture.Tick();

    QCOMPARE(LogCapture::CountContaining(QStringLiteral("still open after")), 1);

    fixture.Tick(kTicksAToggleGetsToShowUp * 5);

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), kTogglesBeforeGivingUp);
    QCOMPARE(LogCapture::CountContaining(QStringLiteral("still open after")), 1);
}

void FslabsA321neoTest::aHoldThatClosesIsNotToggledAgain()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick();

    fixture.ground.forwardHold = FslabsHoldState::Closing;
    fixture.Tick(5);
    fixture.ground.forwardHold = FslabsHoldState::Closed;
    fixture.Tick(kTicksAToggleGetsToShowUp * 3);

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);

    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.Tick(kTicksAToggleGetsToShowUp * 3);

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);
}

void FslabsA321neoTest::aLoaderThatComesBackCancelsTheClose()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Opening;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick();

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);
}

void FslabsA321neoTest::anUnreachableGroundEndpointTogglesNothing()
{
    FslabsFixture fixture;
    fixture.ground.available = false;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());

    fixture.ground.available = true;
    fixture.Tick();

    QCOMPARE(fixture.ground.TogglesOf(FslabsHold::Forward), 1);
}

void FslabsA321neoTest::aNewTurnaroundForgetsTheServingLoader()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);

    fixture.aircraft.OnTurnaroundStarted();
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());
}

void FslabsA321neoTest::aNewTurnaroundDropsTheCloseThatIsDue()
{
    FslabsFixture fixture;
    fixture.ground.forwardHold = FslabsHoldState::Opening;
    fixture.SetLoaders(gsx::states::kLoaderLoading, 0.0);
    fixture.Tick(3);
    fixture.SetLoaders(gsx::states::kLoaderRemoving, 0.0);
    fixture.Tick(3);

    fixture.aircraft.OnTurnaroundStarted();
    fixture.ground.forwardHold = FslabsHoldState::Open;
    fixture.Tick(kTicksAToggleGetsToShowUp);

    QVERIFY(fixture.ground.toggles.empty());
}

QTEST_APPLESS_MAIN(FslabsA321neoTest)

#include "tst_fslabs_a321neo.moc"
