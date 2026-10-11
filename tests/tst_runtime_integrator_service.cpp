#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QScopeGuard>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "doubles/FakeGsxRemoteApiClient.h"
#include "doubles/FakeSimConnectApi.h"
#include "doubles/FakeTurnaroundCheckpointStore.h"
#include "RecordedWire.h"
#include "../src/application/IntegratorRuntime.h"
#include "../src/domain/turnaround/PilotTouch.h"
#include "../src/application/RuntimeIntegratorService.h"
#include "../src/infrastructure/checkpoint/JsonFileTurnaroundCheckpointStore.h"
#include "../src/infrastructure/gsx/GsxLVars.h"
#include "../src/infrastructure/probe/ProbeChannels.h"
#include "../src/infrastructure/simvars/SimVars.h"

namespace
{
    constexpr DWORD kOneSecondEvent = 1;
    constexpr DWORD kFourSecondEvent = 2;
    constexpr DWORD kPauseEvent = 6;
    constexpr DWORD kSimStateRequest = 0x0FFFFFFF;
    constexpr double kWorldMapCamera = 12.0;
    constexpr double kCockpitCamera = 2.0;
    constexpr auto kMsfs2024AppName = "SunRise";
    constexpr auto kTitleDatum = "TITLE";
    constexpr auto kAtcModelDatum = "ATC MODEL";
    constexpr auto kMd11Title = "TFDi Design MD-11 PAX";
    constexpr auto kMd11AtcModel = "MD11";
    constexpr auto kMd11ProfileId = "tfdi-md11";
    constexpr auto kRj85Title = "Just Flight RJ85";
    constexpr auto kUnsupportedTitle = "Cessna Skyhawk G1000 Asobo";
    constexpr auto kUnsupportedAtcModel = "C172";
    constexpr auto kRj85AtcModel = "RJ85";
    constexpr auto kRj85ProfileId = "justflight-rj85";
    constexpr double kRj85RecommendedFuelRateKgs = 12.0;
    constexpr auto kMd11EfbZfw = "L:MD11_EFB_PAYLOAD_ZFW";
    constexpr double kMd11EmptyWeightKg = 150000.0;
    constexpr double kMd11FuelOnBoardKg = 10000.0;
    constexpr double kJetwayInPlace = 5.0;
    constexpr double kNoJetwayAtTheStand = 2.0;
    constexpr int kFlowTickBudget = 12;
    constexpr auto kMenuToggleVerb = "menu.toggle";
    constexpr int kLoaderNoticeTickBudget = 120;
    constexpr int kReconnectWaitMs = 1000;
    constexpr std::chrono::milliseconds kFastReconnectInterval{50};
    constexpr int kPersonalPilotId = 4815162;
    constexpr auto kOpeningSimConnect = "Opening SimConnect...";
    constexpr auto kMd11SlowRuleObserved = "Rule tfdi-md11-commit-efb-targets would pass";
    constexpr auto kTakingOverFuelAndPayload = "Taking over fuel and payload insertion";
    constexpr auto kKeyLine = "GSX reports ";
    constexpr auto kUnknownPathWarning = "WARN: GSX published an unknown path";
    constexpr auto kCdk2FlightWire = "wire-key-20261003-094025.jsonl";
    constexpr auto kNewbornCouatlWire = "wire-key-20261002-184537.jsonl";
    constexpr auto kLfmnWire = "wire-key-20261002-175856.jsonl";
    constexpr auto kCdk2FlightKeyLines = 7;
    constexpr auto kMenuLine = "RemoteAPI menu:";
    constexpr auto kRetakingFuelAndPayload = "GSX automation flags reset by couatl; re-taking fuel and payload";
    constexpr auto kRj85PlannedFuelLVar = "146_SimBrief_Block_Fuel";
    constexpr auto kRj85ForwardPassengerDoorLVar = "EXT_Door_pax_1L";
    constexpr double kDoorOpen = 1.0;
    constexpr auto kRj85PlannedZfwLVar = "146_SimBrief_ZFW";
    constexpr auto kRj85PlannedPassengersLVar = "146_SimBrief_PaxQt";
    constexpr double kRj85PlannedFuelKg = 4200.0;
    constexpr double kRj85PlannedZfwKg = 27500.0;
    constexpr double kRj85PlannedPassengers = 70.0;
    constexpr double kRj85EmptyWeightKg = 24100.0;
    constexpr double kRj85FuelOnBoardKg = 3225.0;

    void PushSimRunning(const int running)
    {
        SIMCONNECT_RECV_SYSTEM_STATE state{};
        state.dwRequestID = kSimStateRequest;
        state.dwInteger = running;
        FakeSimConnectApi::Push(state, SIMCONNECT_RECV_ID_SYSTEM_STATE);
    }

    void PushOneSecondTick()
    {
        SIMCONNECT_RECV_EVENT tick{};
        tick.uEventID = kOneSecondEvent;
        FakeSimConnectApi::Push(tick, SIMCONNECT_RECV_ID_EVENT);
    }

    void PushSimOpen(const char* appName)
    {
        SIMCONNECT_RECV_OPEN open{};
        strcpy_s(open.szApplicationName, appName);
        FakeSimConnectApi::Push(open, SIMCONNECT_RECV_ID_OPEN);
    }

    void PushUnpaused()
    {
        SIMCONNECT_RECV_EVENT pause{};
        pause.uEventID = kPauseEvent;
        pause.dwData = 0;
        FakeSimConnectApi::Push(pause, SIMCONNECT_RECV_ID_EVENT);
    }

    void PushFourSecondTick()
    {
        SIMCONNECT_RECV_EVENT tick{};
        tick.uEventID = kFourSecondEvent;
        FakeSimConnectApi::Push(tick, SIMCONNECT_RECV_ID_EVENT);
    }

    bool PushDatum(const std::string& datumName, const double value)
    {
        const DWORD defineId = FakeSimConnectApi::DefineIdOf(datumName);
        if (defineId == 0)
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectDouble(defineId, value);

        return true;
    }

    bool PushLVar(const std::string& name, const double value)
    {
        return PushDatum("L:" + name, value);
    }

    bool TickAndWait(QSignalSpy& updated)
    {
        PushOneSecondTick();

        return updated.wait(2000);
    }

    bool DispatchPending()
    {
        return QTest::qWaitFor([] { return FakeSimConnectApi::pendingMessages.empty(); }, 2000);
    }

    bool WasWritten(const std::string& datumName)
    {
        const DWORD defineId = FakeSimConnectApi::DefineIdOf(datumName);

        return defineId != 0
            && std::ranges::any_of(FakeSimConnectApi::writtenSimObjectData,
                                   [defineId](const auto& write) { return write.first == defineId; });
    }

    bool PublishTheIdleGsxServices()
    {
        return std::ranges::all_of(
            std::array{gsx::lvars::kRefuelingState, gsx::lvars::kBoardingState, gsx::lvars::kDeboardingState},
            [](const char* state)
            {
                return PushLVar(state, static_cast<double>(GsxStateStatus::Callable));
            });
    }

    bool DetectWithTheGsxUp(const IntegratorRuntime& runtime, QSignalSpy& updated, const char* titleText,
                            const char* atcModelText, const char* profileId)
    {
        PushUnpaused();
        if (!TickAndWait(updated))
        {
            return false;
        }

        const DWORD title = FakeSimConnectApi::DefineIdOf(kTitleDatum);
        const DWORD atcModel = FakeSimConnectApi::DefineIdOf(kAtcModelDatum);
        if (title == 0 || atcModel == 0 || !PushLVar(gsx::lvars::kCouatlStarted, 1.0))
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectString(title, titleText);
        FakeSimConnectApi::PushSimObjectString(atcModel, atcModelText);

        return TickAndWait(updated) && runtime.GetAircraftProfileId() == profileId;
    }

    bool DetectTheMd11WithTheGsxUp(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        return DetectWithTheGsxUp(runtime, updated, kMd11Title, kMd11AtcModel, kMd11ProfileId);
    }

    bool TheCameraMovesTo(const double cameraState, QSignalSpy& updated)
    {
        const DWORD camera = FakeSimConnectApi::DefineIdOf("CAMERA STATE");
        if (camera == 0)
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectDouble(camera, cameraState);

        return TickAndWait(updated);
    }

    bool DriveTheFlowInto(const TurnaroundPhase phase, const IntegratorRuntime& runtime, QSignalSpy& updated,
                          const bool withTheServicesIdle = false)
    {
        for (int tick = 0; tick < kFlowTickBudget && runtime.GetPhase() != phase; ++tick)
        {
            if (withTheServicesIdle)
            {
                (void)PublishTheIdleGsxServices();
            }

            for (const char* engine : {simvars::kSimEng1Combustion, simvars::kSimEng2Combustion,
                                       simvars::kSimEng3Combustion})
            {
                PushDatum(engine, 0.0);
            }

            if (!TickAndWait(updated))
            {
                return false;
            }
        }

        return runtime.GetPhase() == phase;
    }

    bool TheMenuWasToggled()
    {
        return std::ranges::find(FakeGsxRemoteApi::commandVerbs, std::string(kMenuToggleVerb))
            != FakeGsxRemoteApi::commandVerbs.end();
    }

    bool TheRepositionIsAsked(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        FakeGsxRemoteApi::commandVerbs.clear();
        if (!DriveTheFlowInto(TurnaroundPhase::RepositionAircraft, runtime, updated, true))
        {
            return false;
        }

        for (int tick = 0; tick < kFlowTickBudget && !TheMenuWasToggled(); ++tick)
        {
            (void)PublishTheIdleGsxServices();
            if (!TickAndWait(updated))
            {
                return false;
            }
        }

        return TheMenuWasToggled() && runtime.GetPhase() == TurnaroundPhase::RepositionAircraft;
    }

    bool TheRepositionIsSpared(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        FakeGsxRemoteApi::commandVerbs.clear();

        return DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated, true)
            && !TheMenuWasToggled();
    }

    void SkipTheReposition(IntegratorRuntime& runtime)
    {
        AutomationSettings settings;
        settings.skipReposition = true;
        runtime.ApplySettings(settings);
    }

    bool ReachTheMd11SlowRule(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        return DetectTheMd11WithTheGsxUp(runtime, updated)
            && DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated)
            && PublishTheIdleGsxServices()
            && DriveTheFlowInto(TurnaroundPhase::CallServices, runtime, updated)
            && PushLVar(gsx::lvars::kJetway, kJetwayInPlace)
            && DriveTheFlowInto(TurnaroundPhase::WaitingFlightPlan, runtime, updated)
            && TickAndWait(updated)
            && PushDatum(simvars::kSimEmptyWeight, kMd11EmptyWeightKg)
            && PushDatum(simvars::kSimFuelTotalKg, kMd11FuelOnBoardKg)
            && TickAndWait(updated);
    }

    struct RecordingObserver final : IntegratorServiceObserver
    {
        int notifications = 0;

        void OnIntegratorStateChanged() override
        {
            ++notifications;
        }
    };

    IntegratorRuntimeOptions TheProbeActsOnTheSim()
    {
        IntegratorRuntimeOptions options;
        options.actsOnTheSim = [] { return true; };

        return options;
    }

#ifndef NDEBUG
    void TurnTheProbeOff()
    {
        probe::SetEnabled(false);
        probe::ResetForTest();
    }
#endif

    class LogCapture
    {
    public:
        LogCapture()
            : previous_(qInstallMessageHandler(Collect))
        {
            Lines().clear();
        }

        ~LogCapture()
        {
            qInstallMessageHandler(previous_);
        }

        LogCapture(const LogCapture&) = delete;
        LogCapture& operator=(const LogCapture&) = delete;
        LogCapture(LogCapture&&) = delete;
        LogCapture& operator=(LogCapture&&) = delete;

        [[nodiscard]] static qsizetype Count(const QString& fragment)
        {
            return std::ranges::count_if(Lines(),
                                         [&fragment](const QString& line) { return line.contains(fragment); });
        }

        [[nodiscard]] static bool Contains(const QString& fragment)
        {
            return Count(fragment) > 0;
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

    QJsonObject Hello()
    {
        return QJsonObject{{"type", "hello"}};
    }

    QJsonObject EmptySnapshot()
    {
        return QJsonObject{{"type", "snapshot"}};
    }

    QJsonObject Patch(const QString& path, const QJsonValue& value)
    {
        return QJsonObject{{"type", "patch"}, {"path", path}, {"value", value}};
    }

    void ReceiveWire(const std::vector<RecordedMessage>& wire)
    {
        for (const RecordedMessage& recorded : wire)
        {
            FakeGsxRemoteApi::Receive(recorded.message);
        }
    }

    qsizetype KeyLines()
    {
        return LogCapture::Count(QLatin1String(kKeyLine));
    }

    constexpr double kMd11PlannedFuelKg = 31000.0;
    constexpr double kMd11PlannedZfwKg = 180000.0;
    constexpr int kMd11PlannedPassengers = 200;
    constexpr long long kPlanEpoch = 1790000000;

    QJsonObject AMenu()
    {
        return QJsonObject{{"title", "Activate Services"}, {"entries", QJsonArray{"Call Pushback", "Cancel"}}};
    }

    void ReceiveAMenu()
    {
        FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menu"), AMenu()));
        FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), true));
    }

    constexpr auto kSavedCouatl = "2120420471";
    constexpr auto kRestartedCouatl = "2123650379";
    constexpr auto kThirdCouatl = "2129000111";
    constexpr auto kSavedAirport = "CDK2";
    constexpr auto kSavedStand = "Parking 2";
    constexpr auto kOtherStand = "Parking 5";
    constexpr auto kSimOnGroundDatum = "SIM ON GROUND";
    constexpr auto kClientVersion = "1.0.0-test";
    constexpr auto kCheckpointFile = "turnaround.json";
    constexpr int kHoldTickBudget = 30;
    constexpr int kProbeObserveIntervalMs = 1100;
    constexpr int kQuietTickBudget = 60;
    constexpr int kCabinExitTickBudget = 70;
    constexpr int kReadingRounds = 3;
    constexpr double kOnTheGround = 1.0;
    constexpr double kInTheAir = 0.0;
    constexpr auto kTransitionLine = "Transitioning:";
    constexpr auto kResumedLine = "Resumed the saved turnaround";
    constexpr auto kEraseWarning = "could not be deleted";
    constexpr auto kWriteWarning = "could not be saved";
    constexpr auto kHandshakeLine = "GSX Remote API handshake received";
    constexpr auto kMenuAnswerLine = "RemoteAPI menu: 'Activate Services'";
    constexpr auto kMenuPickVerb = "menu.pick";

    struct ResumeReading
    {
        const char* datum;
        bool isLVar;
        double value;
    };

    constexpr auto kCallable = static_cast<double>(GsxStateStatus::Callable);

    constexpr std::array<ResumeReading, 10> kResumeReadings = {{
        {.datum = gsx::lvars::kCouatlStarted, .isLVar = true, .value = 1.0},
        {.datum = gsx::lvars::kRefuelingState, .isLVar = true, .value = kCallable},
        {.datum = gsx::lvars::kBoardingState, .isLVar = true, .value = kCallable},
        {.datum = gsx::lvars::kPushbackVehicleState, .isLVar = true, .value = kCallable},
        {.datum = gsx::lvars::kDeboardingState, .isLVar = true, .value = kCallable},
        {.datum = gsx::lvars::kDeiceState, .isLVar = true, .value = kCallable},
        {.datum = gsx::lvars::kPushbackStatus, .isLVar = true, .value = 0.0},
        {.datum = kSimOnGroundDatum, .isLVar = false, .value = kOnTheGround},
        {.datum = simvars::kSimEmptyWeight, .isLVar = false, .value = kMd11EmptyWeightKg},
        {.datum = simvars::kSimFuelTotalKg, .isLVar = false, .value = kMd11FuelOnBoardKg},
    }};

    void PushTheReadingsThatWereAsked(const std::string& withheld)
    {
        for (const ResumeReading& reading : kResumeReadings)
        {
            if (withheld != reading.datum)
            {
                (void)(reading.isLVar ? PushLVar(reading.datum, reading.value) : PushDatum(reading.datum, reading.value));
            }
        }
    }

    bool TheReadingsArrive(QSignalSpy& updated, const std::string& withheld = {})
    {
        for (int round = 0; round < kReadingRounds; ++round)
        {
            PushTheReadingsThatWereAsked(withheld);
            if (!TickAndWait(updated))
            {
                return false;
            }
        }

        return true;
    }

    TurnaroundKey Md11Key(const char* couatl = kSavedCouatl, const char* parking = kSavedStand)
    {
        return TurnaroundKey{.couatlId = couatl,
                         .aircraftId = kMd11ProfileId,
                         .aircraftTitle = kMd11Title,
                         .airportIcao = kSavedAirport,
                         .parkingName = parking};
    }

    TurnaroundDocument SavedAt(const TurnaroundPhase phase)
    {
        TurnaroundDocument document;
        document.key = Md11Key();
        document.checkpoint = TurnaroundCheckpoint{.phase = phase};

        return document;
    }

    TurnaroundDocument SavedWithTheKeyOnly()
    {
        TurnaroundDocument document;
        document.key = Md11Key();

        return document;
    }

    QJsonObject AMenuTheNavigatorAnswersAlone(const char* title, const char* firstEntry, const char* secondEntry)
    {
        return QJsonObject{{"title", title}, {"entries", QJsonArray{firstEntry, secondEntry}}};
    }

    QJsonObject TheAirstairsQuestion()
    {
        return AMenuTheNavigatorAnswersAlone("Use airplane's own airstairs?", "Yes", "No");
    }

    QJsonObject TheDeIceQuestion()
    {
        return AMenuTheNavigatorAnswersAlone("Do you want de-icing?", "Yes", "No");
    }

    void ReceiveAMenuPatch(const QJsonObject& menu)
    {
        FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menu"), menu));
        FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), true));
    }

    QJsonObject GsxWire(const char* couatl, const char* parking, const bool withServices = true)
    {
        QJsonObject snapshot{{"type", "snapshot"},
                             {"startup", QJsonObject{{"sid", couatl}}},
                             {"airport", QJsonObject{{"icao", kSavedAirport}}},
                             {"parking", parking}};
        if (withServices)
        {
            snapshot.insert("services", QJsonArray{QJsonObject{{"id", "refuel"}, {"stateRaw", 1}, {"canTrigger", true}}});
        }

        return snapshot;
    }

    void TheRemoteApiReconnectsAs(const char* couatl, const char* stand)
    {
        FakeGsxRemoteApi::AnnounceConnection(false);
        FakeGsxRemoteApi::AnnounceConnection(true);
        FakeGsxRemoteApi::Receive(GsxWire(couatl, stand));
    }

    IntegratorRuntimeOptions WithTheStore(TurnaroundCheckpointStore& store)
    {
        IntegratorRuntimeOptions options;
        options.checkpointStore = &store;

        return options;
    }

    void StartTheFlowWithoutTheReposition(IntegratorRuntime& runtime, const bool autoStart = true)
    {
        AutomationSettings settings;
        settings.skipReposition = true;
        settings.autoStartFlow = autoStart;
        runtime.ApplySettings(settings);
    }

    bool DetectTheMd11WithoutTheGsx(const IntegratorRuntime& runtime, QSignalSpy& updated,
                                    const char* titleText = kMd11Title)
    {
        PushUnpaused();
        if (!TickAndWait(updated))
        {
            return false;
        }

        const DWORD title = FakeSimConnectApi::DefineIdOf(kTitleDatum);
        const DWORD atcModel = FakeSimConnectApi::DefineIdOf(kAtcModelDatum);
        if (title == 0 || atcModel == 0)
        {
            return false;
        }

        FakeSimConnectApi::PushSimObjectString(title, titleText);
        FakeSimConnectApi::PushSimObjectString(atcModel, kMd11AtcModel);

        return TickAndWait(updated) && runtime.GetAircraftProfileId() == kMd11ProfileId;
    }

    bool TheKeyIsJudged(IntegratorRuntime& runtime, QSignalSpy& updated, const char* couatl = kSavedCouatl,
                        const char* stand = kSavedStand)
    {
        if (!DetectTheMd11WithoutTheGsx(runtime, updated))
        {
            return false;
        }

        FakeGsxRemoteApi::Receive(GsxWire(couatl, stand));

        return TickAndWait(updated);
    }

    bool TheFinishedTurnaroundWaitsForTheNextOne(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        for (int tick = 0; tick < kCabinExitTickBudget && runtime.GetPhase() != TurnaroundPhase::WaitingNewFlight;
             ++tick)
        {
            if (!TickAndWait(updated))
            {
                return false;
            }
        }

        return runtime.GetPhase() == TurnaroundPhase::WaitingNewFlight && TickAndWait(updated);
    }

    bool TheSavedTurnaroundResumes(IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        for (int round = 0; round < kReadingRounds * 2; ++round)
        {
            PushTheReadingsThatWereAsked({});
            if (!TickAndWait(updated))
            {
                return false;
            }

            if (runtime.Snapshot().turnaroundHold == TurnaroundHold::None)
            {
                return true;
            }
        }

        return false;
    }

#ifndef NDEBUG
    bool TheFuelTakeoverStarts(QSignalSpy& updated)
    {
        return PushLVar(gsx::lvars::kRefuelingState, static_cast<double>(GsxStateStatus::Callable))
            && TickAndWait(updated)
            && LogCapture::Contains(QLatin1String(kTakingOverFuelAndPayload));
    }

    bool TheCouatlRaisesTheAutomationFlags()
    {
        return PushLVar(gsx::lvars::kAutomationFuel, 1.0)
            && PushLVar(gsx::lvars::kAutomationPayload, 1.0);
    }

    bool TheRj85PlanReachesWaitingPowerOn(const IntegratorRuntime& runtime, QSignalSpy& updated)
    {
        return PushDatum(simvars::kSimEmptyWeight, kRj85EmptyWeightKg)
            && PushLVar(kRj85PlannedFuelLVar, kRj85PlannedFuelKg)
            && TickAndWait(updated)
            && PushLVar(kRj85PlannedZfwLVar, kRj85PlannedZfwKg)
            && TickAndWait(updated)
            && PushLVar(kRj85PlannedPassengersLVar, kRj85PlannedPassengers)
            && TickAndWait(updated)
            && PushLVar(gsx::lvars::kSimbriefSuccess, 1.0)
            && DriveTheFlowInto(TurnaroundPhase::WaitingPowerOn, runtime, updated);
    }
#endif
}

class RuntimeIntegratorServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    static void init();

    static void freshSnapshotHasDisconnectedDefaults();
    static void commandsFailWhileOffline();
    static void fixGsxProfileWithoutConflictFails();
    static void fixPmdgOptionsWithoutConflictFails();
    static void applySettingsPushesEffectiveSettings();
    static void theAircraftRecommendedFuelRateReachesTheEffectiveSettings();
    static void theSnapshotSaysWhetherTheAutomationStartsWithTheFlight();
    static void observersAreDedupedAndNotified();
    static void automationToggleEmitsOncePerChange();
    static void runtimeGettersOnEmptyRuntime();
    static void setupConnectsThroughFakeSimConnect();
    static void setupStartsTheRemoteApiClientWithoutOpeningASocket();
    static void connectedCommandsFollowGuardOrder();
    static void subscribeFailureDisconnects();
    static void subscribeFailureKeepsRetrying();
    static void simulatorQuitRearmsTheReconnect();
    static void aTouchWithoutTheFlowRunningIsRefused();
    static void aTouchStampedWithAnotherPhaseIsRefusedAndNothingMoves();
    static void aTouchStampedWithAPhaseThatTakesNoneIsRefused();
    static void aTouchStampedWithTheCurrentPhaseReachesTheFlow();
    static void aWorldMapCameraDuringTheLoadDoesNotLeaveTheFlowOff();
    static void theGsxChipFollowsTheGsxWhileThePilotIsOnFoot();
    static void theAircraftIsDetectedOnlyOnceTheAtcModelArrives();
    static void theSnapshotCountsTheJetwayWaitDownWithTheFlow();
    static void theSnapshotCarriesTheLoaderCountdownWhileTheLoaderHoldsBoarding();
    static void theSnapshotCarriesTheDeboardingWaitOnceTheGsxTakesTheRequest();
    static void theSnapshotCarriesTheAirstairPressureWaitOnlyWhileTheAutomationDrives();
    static void theSlowTickWritesNothingWhileTheGsxIsDown();
    static void theSlowRulesAreObservedWhileTheAutomationIsOffAndTheProbeActs();
    static void theSlowTickLeavesTheTakeoversAloneWhileTheAutomationIsOff();
    static void endingTheSessionForgetsTheAircraftStrings();
    static void theSnapshotCarriesTheZfwTargetAndTheEmptyZfw();
    static void theSnapshotCarriesTheFuelOnBoardTheAircraftReportsNow();
    static void theFuelWaitsUntilTheRemoteApiAnnouncesItsConnection();
    static void openingSimConnectIsAnnouncedOncePerDisconnectedPeriod();
    static void theKeyReachesTheLogOncePerChangeAndNotPerMessage();
    static void theKeyPathsAreNoLongerWarnedAsUnknown();
    static void aBlipOnTheSameCouatlLogsNoNewKey();
    static void theCouatlRestartLogsWhatTheNewbornWireSays();
    static void theMachineDoesNotTickBetweenTheHandshakeAndTheSnapshot();
    static void theSlowTickWaitsForTheSnapshotToo();
    static void theMenuIsLeftAloneBetweenTheHandshakeAndTheSnapshot();
    static void aDropKeepsTheMachineTicking();
    static void theLoggingToggleAloneLeavesTheAircraftUntouched();
    static void theRunHeaderNamesTheRunFolderAndLeavesThePilotIdOut();

    static void aSavedTurnaroundHoldsTheMachineWhileTheRemoteApiNeverConnects();
    static void aSavedTurnaroundIsJudgedEvenWhileTheAutomationIsOff();
    static void aSavedTurnaroundFromAnotherStandIsErasedAndTheFlowStartsAsToday();
    static void aSavedTurnaroundOfAnotherAircraftIsErased();
    static void aSavedTurnaroundIsErasedUnderAnAircraftTheClientDoesNotSupport();
    static void aDifferentCouatlHoldsTheMachineAndAsksThePilot();
    static void answeringRestartErasesTheFileAndStartsTheFlow();
    static void answeringResumeRestoresTheSavedPhase();
    static void theResumeAnswerIsRefusedWhileTheSavedTurnaroundIsStillBeingJudged();
    static void theResumeAnswerIsRefusedWhenThereIsNoSavedFile();
    static void aRepeatedResumeWhileTheRestorationRunsSucceedsAndChangesNothing();
    static void theRestartIsAcceptedWhileTheSavedTurnaroundHoldsWithTheAutomationOff();
    static void theResumeAnswerIsRefusedWhileOffline();
    static void aResumedServiceThatReadsAvailableCountsAsDoneOnTheSameCouatlAndInterruptedAfterARestart();
    static void aSavedTurnaroundOfTheSameKeyResumesWithoutRepeatingAnything();
    static void aSavedFlightWaitsForTheGroundReadingBeforeTheFirstTick();
    static void eachResumeReadingMissingAloneHoldsTheMachine_data();
    static void eachResumeReadingMissingAloneHoldsTheMachine();
    static void theButtonErasesTheSavedTurnaround();
    static void aTransientSessionEndKeepsTheFileAndTheFlowResumesAgain();
    static void aMenuPatchAfterATransientSessionEndLeavesTheSavedFileIntact();
    static void withoutAPortTheFlowStillWaitsForTheButton();
    static void withAPortAndNoFileTheFlowStillWaitsForTheButton();
    static void aResumedTurnaroundRunsWhateverTheAutoStartOptionSays();
    static void aSavedPhaseTheMachineDoesNotKnowErasesTheFileAndStartsTheFlow();
    static void aKeyOnlyFileWithoutTheMarkIsIgnored();
    static void aKeyOnlyFileThatCarriesTheMarkHoldsTheMachineUntilItsKeyIsJudged();
    static void aKeyOnlyFileWithoutTheMarkOfAnotherAircraftIsNotErased();
    static void aKeyOnlyFileWithTheMarkOfAnotherAircraftIsErased();
    static void theGsxMenuOfASnapshotIsLeftAloneWhileTheSavedTurnaroundHolds();
    static void theGsxMenuOfAPatchIsLeftAloneWhileTheSavedTurnaroundHolds();
    static void theSlowRulesAreOnlyObservedWhileTheSavedTurnaroundHolds();
    static void theSimbriefReloadIsRefusedWhileTheSavedTurnaroundHolds();
    static void aRestoredPlanLeavesTheStatusReadyAndComesBackInTheNextWrite();
    static void theServiceAndMenuMemoryComeBackOnlyAfterTheServiceListArrives();
    static void nothingIsWrittenBeforeTheCouatlIdIsKnown();
    static void nothingIsWrittenInTheFirstTwoPhases();
    static void sixtyTicksWithoutAnEventDoNotWriteAgain();
    static void enteringWaitingNewFlightWritesOnlyTheKey();
    static void aFailedWriteLeavesTheLastWrittenBehindAndIsTriedAgain();
    static void aFailedEraseDoesNotHoldTheMachineAndIsTriedAgainEverySecond();
    static void aFailedEraseLetsTheMachineRunAndASuccessfulWriteSettlesIt();
    static void aDocumentWhoseErasureFailedIsNeverRestoredInThisProcess();
    static void anUnstartedFlowLeavesTheGsxMenuOfASnapshotAlone();
    static void anUnstartedFlowLeavesTheGsxMenuOfAPatchAlone();
    static void endingTheSessionForgetsTheAircraftName();
    static void theHandshakeWithoutASnapshotIsPublishedAsTheReasonToWait();
    static void theHandshakeReasonWinsOverTheSavedTurnaroundOne();
    static void aRuntimeResumesTheFileAnotherRuntimeWrote();
    static void aTitleWithBytesOutsideUtf8SurvivesTheFile();
    static void anUnreadableFileNeverRestoresAndNeverHolds();
    static void aFileOfAnotherClientVersionNeverRestoresAndNeverHolds();
    static void aSimulatorQuitWithoutASessionLeavesTheAutomationAlone();
    static void aNewCouatlLifeThatServesTheSameSimbriefGenerationIsAnnounced();
    static void thePlanIsNotAdoptedWhileTheAircraftIsStillUnreachable();
    static void aCouatlThatRestartsWhileTheReadingsAreAwaitedAsksThePilot();
    static void aParkingThatChangesWhileThePilotDecidesErasesTheFile();
    static void aCouatlThatRestartsAfterTheResumeAnswerDoesNotAskAgain();
    static void theQuestionSurvivesTheHandshakeThatComesBeforeTheSnapshot();
    static void theSimbriefReloadIsNotOfferedWhileTheSavedTurnaroundHolds();
    static void theGsxMenuAnswerIsSavedRightAfterTheNavigatorSends();
    static void aSavedTurnaroundWithoutAParkingResumesOnceTheLiveOneIsKnown();
    static void restartingWhileTheSavedTurnaroundIsBeingJudgedErasesItAndNeverRestoresIt();
    static void endingTheSessionWhileTheQuestionIsOpenKeepsTheFileAndAsksAgain();
    static void theMarkOfAKeyOnlyFileOfTheSameKeyReachesTheMachine();
    static void aKeyOnlyFileWithoutTheMarkLeavesTheRepositionToTheTurnaround();
    static void theMarkDiesWhenOnlyTheCouatlDiffersFromAKeyOnlyFile();
    static void theMarkDoesNotSpareTheRepositionWhenTheNewTurnaroundOptionIsOff();
    static void theRestartButtonKillsTheMark();
    static void theMarkOfAResumedTurnaroundIsWrittenBackWithEveryDocument();
    static void theMarkDiesWhenThePilotResumesOnAnotherCouatl();
    static void aKeyOnlyFileWithTheMarkOfAnotherStandIsErasedAndTheRepositionIsAsked();
    static void aKeyOnlyFileWaitsForTheStandAndIsErasedWhenItArrivesDifferent();
    static void aKeyOnlyFileWaitsForTheStandAndSparesTheRepositionWhenItArrivesTheSame();
    static void aStandChangedWithoutFlyingKillsTheMarkOfTheFinishedTurnaround();
    static void theSameStandKeepsTheMarkOfTheFinishedTurnaround();
    static void aStandThatGoesEmptyAndComesBackKeepsTheMarkOfTheFinishedTurnaround();
    static void aStandChangedWithTheTurnaroundUnderWayErasesItAndRepositionsAtTheNewStand();
    static void aStandThatGoesEmptyAndComesBackKeepsTheTurnaroundUnderWay();
    static void aStandChangedAfterThePushbackWasAskedKeepsTheTurnaround();
    static void enteringWaitingNewFlightWritesTheKeyAndTheMarkAndARelaunchKeepsIt();

private:
    QTemporaryDir probeDirectory_;
};

void RuntimeIntegratorServiceTest::initTestCase()
{
    qunsetenv("GSXI_PROBE");

    QVERIFY(probeDirectory_.isValid());
    qputenv("GSXI_PROBE_DIR", probeDirectory_.path().toUtf8());
}

void RuntimeIntegratorServiceTest::init()
{
    FakeSimConnectApi::Reset();
    FakeGsxRemoteApi::Reset();
}

void RuntimeIntegratorServiceTest::freshSnapshotHasDisconnectedDefaults()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    const IntegratorSnapshot snapshot = service.GetSnapshot();

    QVERIFY(!snapshot.connected);
    QVERIFY(!snapshot.sessionActive);
    QVERIFY(!snapshot.automationEnabled);
    QVERIFY(!snapshot.gsxAvailable);
    QVERIFY(!snapshot.aircraftSupported);
    QVERIFY(!snapshot.canToggleAutomation);
    QVERIFY(!snapshot.canStartLoading);
    QVERIFY(!snapshot.canReloadSimbrief);
    QVERIFY(!snapshot.refuelByGsx);
    QVERIFY(!snapshot.refuelBySelf);
    QVERIFY(!snapshot.gsxProfileConflict);
    QVERIFY(!snapshot.gsxProfileFixable);
    QVERIFY(!snapshot.pmdgOptionsConflict);
    QVERIFY(!snapshot.pmdgOptionsFixable);
    QVERIFY(!snapshot.cargoAircraft);
    QVERIFY(!snapshot.engineerPanelExternalPower);
    QVERIFY(!snapshot.efbFlightPlanOnDeparturePage);
    QCOMPARE(snapshot.aircraftName, std::string{});
    QCOMPARE(snapshot.aircraftProfileId, std::string{});
    QCOMPARE(snapshot.phase, TurnaroundPhase::WaitingSupportedAircraft);
    QCOMPARE(snapshot.flightPlanStatus, FlightPlanStatus::Idle);
    QCOMPARE(snapshot.fuelProgress, 0.0);
    QCOMPARE(snapshot.boardingProgress, 0.0);
    QCOMPARE(snapshot.deboardingProgress, 0.0);
    QCOMPARE(snapshot.plannedFuelKg, 0.0);
    QCOMPARE(snapshot.loadedFuelKg, 0.0);
    QCOMPARE(snapshot.plannedZfwKg, 0.0);
    QCOMPARE(snapshot.plannedPax, 0);
    QCOMPARE(snapshot.boardedPax, 0);
    QCOMPARE(snapshot.delayTicksRemaining, 0);
}

void RuntimeIntegratorServiceTest::commandsFailWhileOffline()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const std::string offline = "Simulator is offline.";

    const CommandResult automation = service.SetAutomationEnabled(true);

    QVERIFY(!automation.succeeded);
    QCOMPARE(automation.message, offline);

    const CommandResult loading = service.StartLoading();

    QVERIFY(!loading.succeeded);
    QCOMPARE(loading.message, offline);

    const CommandResult restart = service.RestartFlow();

    QVERIFY(!restart.succeeded);
    QCOMPARE(restart.message, offline);

    const CommandResult reload = service.ReloadSimbrief();

    QVERIFY(!reload.succeeded);
    QCOMPARE(reload.message, offline);

    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingSupportedAircraft);

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, offline);
}

void RuntimeIntegratorServiceTest::fixGsxProfileWithoutConflictFails()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const CommandResult result = service.FixGsxProfile();

    QVERIFY(!result.succeeded);
    QCOMPARE(result.message, std::string("The GSX profile does not need fixing."));
}

void RuntimeIntegratorServiceTest::fixPmdgOptionsWithoutConflictFails()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const CommandResult result = service.FixPmdgOptions();

    QVERIFY(!result.succeeded);
    QCOMPARE(result.message, std::string("The PMDG options file does not need fixing."));
}

void RuntimeIntegratorServiceTest::applySettingsPushesEffectiveSettings()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    AppSettings settings;
    settings.simbriefPilotId = 123;
    settings.fuelRateMode = FuelRateMode::Manual;
    settings.fuelRateKgs = 7.5;
    settings.callCatering = true;
    settings.callBoardingEarly = true;

    service.ApplySettings(settings);

    QCOMPARE(runtime.Settings().simbriefPilotId, 123);
    QCOMPARE(runtime.Settings().fuelRateKgs, 7.5);
    QCOMPARE(runtime.Settings().callCatering, true);
    QCOMPARE(runtime.Settings().callBoardingEarly, true);
    QCOMPARE(runtime.Snapshot().fuelRateKgs.value, 7.5);
}

void RuntimeIntegratorServiceTest::theAircraftRecommendedFuelRateReachesTheEffectiveSettings()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    AppSettings settings;
    settings.fuelRateKgs = 7.5;
    service.ApplySettings(settings);

    QCOMPARE(runtime.AircraftRecommendedFuelRateKgs(), 0.0);
    QCOMPARE(runtime.Settings().fuelRateKgs, AutomationSettings::kDefaultFuelRateKgs);

    runtime.Setup();
    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectWithTheGsxUp(runtime, updated, kRj85Title, kRj85AtcModel, kRj85ProfileId));

    QCOMPARE(runtime.AircraftRecommendedFuelRateKgs(), kRj85RecommendedFuelRateKgs);
    QCOMPARE(runtime.Settings().fuelRateKgs, kRj85RecommendedFuelRateKgs);
    QCOMPARE(runtime.Snapshot().fuelRateKgs.value, kRj85RecommendedFuelRateKgs);
}

void RuntimeIntegratorServiceTest::theSnapshotSaysWhetherTheAutomationStartsWithTheFlight()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    AppSettings settings;
    settings.autoStartFlow = false;
    service.ApplySettings(settings);

    QVERIFY(!runtime.Snapshot().automationStartsWithFlight);

    settings.autoStartFlow = true;
    service.ApplySettings(settings);

    QVERIFY(runtime.Snapshot().automationStartsWithFlight);
    QVERIFY(!runtime.Snapshot().automationEnabled);
}

void RuntimeIntegratorServiceTest::observersAreDedupedAndNotified()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    RecordingObserver first;
    RecordingObserver second;

    service.AddObserver(&first);
    service.AddObserver(&first);
    service.AddObserver(nullptr);
    service.AddObserver(&second);

    service.ApplySettings(AppSettings{});

    QCOMPARE(first.notifications, 1);
    QCOMPARE(second.notifications, 1);

    service.RemoveObserver(&second);

    service.ApplySettings(AppSettings{});

    QCOMPARE(first.notifications, 2);
    QCOMPARE(second.notifications, 1);
}

void RuntimeIntegratorServiceTest::automationToggleEmitsOncePerChange()
{
    IntegratorRuntime runtime;
    const QSignalSpy updates(&runtime, &IntegratorRuntime::Updated);

    runtime.SetAutomationEnabled(true);

    QCOMPARE(updates.count(), 1);

    runtime.SetAutomationEnabled(true);

    QCOMPARE(updates.count(), 1);

    runtime.SetAutomationEnabled(false);

    QCOMPARE(updates.count(), 2);
}

void RuntimeIntegratorServiceTest::runtimeGettersOnEmptyRuntime()
{
    IntegratorRuntime runtime;

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QVERIFY(snapshot.aircraftName.empty());
    QCOMPARE(snapshot.aircraftProfileId, std::string{});
    QVERIFY(!snapshot.refuelByGsx);
    QVERIFY(!snapshot.refuelBySelf);
    QVERIFY(!snapshot.cargoAircraft);
    QVERIFY(!snapshot.engineerPanelExternalPower);
    QVERIFY(!snapshot.efbFlightPlanOnDeparturePage);
    QVERIFY(!snapshot.gsxProfileConflict);
    QVERIFY(!snapshot.gsxProfileFixable);
    QVERIFY(!snapshot.pmdgOptionsConflict);
    QVERIFY(!snapshot.pmdgOptionsFixable);
    QCOMPARE(runtime.GetAircraftProfileId(), std::string{});
    QVERIFY(!runtime.HasGsxProfileConflict());
    QVERIFY(!runtime.FixGsxProfile());
    QVERIFY(!runtime.HasPmdgOptionsConflict());
    QVERIFY(!runtime.FixPmdgOptions());
    QVERIFY(!runtime.ReloadSimbrief());
}

void RuntimeIntegratorServiceTest::setupConnectsThroughFakeSimConnect()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    QVERIFY(runtime.IsConnected());

    const IntegratorSnapshot snapshot = service.GetSnapshot();

    QVERIFY(snapshot.connected);
    QVERIFY(snapshot.canToggleAutomation);
}

void RuntimeIntegratorServiceTest::setupStartsTheRemoteApiClientWithoutOpeningASocket()
{
    IntegratorRuntime runtime;

    QCOMPARE(FakeGsxRemoteApi::startCalls, 0);

    runtime.Setup();

    QCOMPARE(FakeGsxRemoteApi::startCalls, 1);
    QVERIFY(FakeGsxRemoteApi::commandVerbs.empty());
}

void RuntimeIntegratorServiceTest::connectedCommandsFollowGuardOrder()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    const CommandResult automation = service.SetAutomationEnabled(true);

    QVERIFY(automation.succeeded);
    QVERIFY(service.GetSnapshot().automationEnabled);

    const CommandResult loading = service.StartLoading();

    QVERIFY(!loading.succeeded);
    QCOMPARE(loading.message, std::string("The turnaround is not waiting to start loading."));

    const CommandResult reload = service.ReloadSimbrief();

    QVERIFY(!reload.succeeded);
    QCOMPARE(reload.message, std::string("Wait for an active flight session."));

    const CommandResult restart = service.RestartFlow();

    QVERIFY(restart.succeeded);
}

void RuntimeIntegratorServiceTest::subscribeFailureDisconnects()
{
    FakeSimConnectApi::subscribeSucceeds = false;

    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(!runtime.IsConnected());
}

void RuntimeIntegratorServiceTest::subscribeFailureKeepsRetrying()
{
    FakeSimConnectApi::subscribeSucceeds = false;

    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(!runtime.IsConnected());
    QVERIFY(runtime.IsReconnectPending());
}

void RuntimeIntegratorServiceTest::simulatorQuitRearmsTheReconnect()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QVERIFY(runtime.IsConnected());
    QVERIFY(!runtime.IsReconnectPending());

    QSignalSpy quits(&runtime, &IntegratorRuntime::SimulatorQuit);

    constexpr SIMCONNECT_RECV quit{};
    FakeSimConnectApi::Push(quit, SIMCONNECT_RECV_ID_QUIT);

    QVERIFY(quits.wait(2000));
    QVERIFY(!runtime.IsConnected());
    QVERIFY(runtime.IsReconnectPending());
}

void RuntimeIntegratorServiceTest::aTouchWithoutTheFlowRunningIsRefused()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    const CommandResult touch = service.AcceptPilotTouch(runtime.GetPhase());

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("Start the turnaround flow first."));
}

void RuntimeIntegratorServiceTest::aTouchStampedWithAnotherPhaseIsRefusedAndNothingMoves()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    const TurnaroundPhase before = runtime.GetPhase();
    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingNewFlight);

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("The turnaround moved on before your touch arrived."));
    QCOMPARE(runtime.GetPhase(), before);
}

void RuntimeIntegratorServiceTest::aTouchStampedWithAPhaseThatTakesNoneIsRefused()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    QVERIFY(!PilotTouch::Accepts(runtime.GetPhase()));

    const CommandResult touch = service.AcceptPilotTouch(runtime.GetPhase());

    QVERIFY(!touch.succeeded);
    QCOMPARE(touch.message, std::string("This step does not wait on the pilot."));
}

void RuntimeIntegratorServiceTest::aTouchStampedWithTheCurrentPhaseReachesTheFlow()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    runtime.Setup();
    QVERIFY(service.SetAutomationEnabled(true).succeeded);

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::WaitingNewFlight));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingNewFlight);

    const CommandResult touch = service.AcceptPilotTouch(TurnaroundPhase::WaitingNewFlight);

    QVERIFY(touch.succeeded);
    QVERIFY(touch.message.empty());
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::aWorldMapCameraDuringTheLoadDoesNotLeaveTheFlowOff()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    AutomationSettings settings;
    settings.autoStartFlow = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushSimRunning(1);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(runtime.IsSessionActive());
    QVERIFY(service.GetSnapshot().automationEnabled);

    const DWORD camera = FakeSimConnectApi::DefineIdOf("CAMERA STATE");
    QVERIFY(camera != 0);

    FakeSimConnectApi::PushSimObjectDouble(camera, kWorldMapCamera);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(!service.GetSnapshot().sessionReady);

    FakeSimConnectApi::PushSimObjectDouble(camera, kCockpitCamera);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(runtime.IsSessionActive());
    QVERIFY(service.GetSnapshot().sessionReady);
    QVERIFY(service.GetSnapshot().automationEnabled);
}

void RuntimeIntegratorServiceTest::theGsxChipFollowsTheGsxWhileThePilotIsOnFoot()
{
    IntegratorRuntime runtime;
    const RuntimeIntegratorService service(&runtime);

    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushSimOpen(kMsfs2024AppName);
    PushUnpaused();
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    const DWORD isAircraft = FakeSimConnectApi::DefineIdOf("IS AIRCRAFT");
    const DWORD isAvatar = FakeSimConnectApi::DefineIdOf("IS AVATAR");
    QVERIFY(isAircraft != 0);
    QVERIFY(isAvatar != 0);

    FakeSimConnectApi::PushSimObjectDouble(isAircraft, 1.0);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(service.GetSnapshot().sessionReady);

    const DWORD couatlStarted = FakeSimConnectApi::DefineIdOf("L:FSDT_GSX_COUATL_STARTED");
    QVERIFY(couatlStarted != 0);
    QVERIFY(!service.GetSnapshot().gsxAvailable);

    FakeSimConnectApi::PushSimObjectDouble(isAvatar, 1.0);
    FakeSimConnectApi::PushSimObjectDouble(couatlStarted, 1.0);
    PushOneSecondTick();
    QVERIFY(updated.wait(2000));

    QVERIFY(!service.GetSnapshot().sessionReady);
    QVERIFY(service.GetSnapshot().pilotOnFoot);
    QVERIFY(service.GetSnapshot().gsxAvailable);
}

void RuntimeIntegratorServiceTest::theAircraftIsDetectedOnlyOnceTheAtcModelArrives()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));

    const DWORD title = FakeSimConnectApi::DefineIdOf(kTitleDatum);
    const DWORD atcModel = FakeSimConnectApi::DefineIdOf(kAtcModelDatum);

    QVERIFY(title != 0);
    QVERIFY(atcModel != 0);
    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));

    FakeSimConnectApi::PushSimObjectString(title, kMd11Title);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetAircraftProfileId(), std::string{});

    FakeSimConnectApi::PushSimObjectString(atcModel, kMd11AtcModel);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetAircraftProfileId(), std::string{kMd11ProfileId});
}

void RuntimeIntegratorServiceTest::theSnapshotCountsTheJetwayWaitDownWithTheFlow()
{
    IntegratorRuntime runtime;

    AutomationSettings settings;
    settings.skipReposition = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(PublishTheIdleGsxServices());
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::CallServices, runtime, updated));

    QVERIFY(PushLVar(gsx::lvars::kJetway, static_cast<double>(GsxStateStatus::Requested)));
    QVERIFY(TickAndWait(updated));

    const int firstWaitSeconds = runtime.Snapshot().servicesWaitSeconds;
    QVERIFY(firstWaitSeconds > 0);

    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().servicesWaitSeconds, firstWaitSeconds - 1);
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheLoaderCountdownWhileTheLoaderHoldsBoarding()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::Loading) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::Loading);

    QVERIFY(PushLVar(gsx::lvars::kBoardingState, static_cast<double>(GsxStateStatus::Active)));
    QVERIFY(TickAndWait(updated));

    QVERIFY(PushLVar(gsx::lvars::kBaggageLoaderMainState, gsx::states::kLoaderWaitingForDoor));
    for (int tick = 0; tick < kLoaderNoticeTickBudget
         && runtime.Snapshot().loaderHoldingBoarding == CargoLoader::None; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QCOMPARE(snapshot.loaderHoldingBoarding, CargoLoader::MainDeck);
    QVERIFY(snapshot.loaderDoorWaitSeconds > 0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheAirstairPressureWaitOnlyWhileTheAutomationDrives()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectWithTheGsxUp(runtime, updated, kRj85Title, kRj85AtcModel, kRj85ProfileId));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::CallServices) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::CallServices);

    QVERIFY(!runtime.Snapshot().ownStairsWaitingForPressure);

    QVERIFY(PushLVar(gsx::lvars::kJetway, kNoJetwayAtTheStand));
    QVERIFY(TickAndWait(updated));
    QVERIFY(PushLVar(kRj85ForwardPassengerDoorLVar, kDoorOpen));
    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.Snapshot().ownStairsWaitingForPressure);

    runtime.SetAutomationEnabled(false);

    QVERIFY(!runtime.Snapshot().ownStairsWaitingForPressure);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheDeboardingWaitOnceTheGsxTakesTheRequest()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::RequestDeboarding)
                           - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::RequestDeboarding);

    QVERIFY(PushLVar(gsx::lvars::kDeboardingState, static_cast<double>(GsxStateStatus::Unavailable)));
    QVERIFY(TickAndWait(updated));

    QVERIFY(!runtime.Snapshot().deboardingAwaitsGsx);

    QVERIFY(PushLVar(gsx::lvars::kDeboardingState, static_cast<double>(GsxStateStatus::Requested)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::RequestDeboarding);
    QVERIFY(runtime.Snapshot().deboardingAwaitsGsx);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theSlowTickWritesNothingWhileTheGsxIsDown()
{
    IntegratorRuntime runtime;
    SkipTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(ReachTheMd11SlowRule(runtime, updated));

    FakeSimConnectApi::writtenSimObjectData.clear();

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 0.0));
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(WasWritten(kMd11EfbZfw));
}

void RuntimeIntegratorServiceTest::theSlowRulesAreObservedWhileTheAutomationIsOffAndTheProbeActs()
{
    const LogCapture log;
    IntegratorRuntime runtime(TheProbeActsOnTheSim());

    AutomationSettings settings;
    settings.autoStartFlow = false;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(!runtime.Snapshot().automationEnabled);
    QVERIFY(!LogCapture::Contains(QLatin1String(kMd11SlowRuleObserved)));

    FakeSimConnectApi::writtenSimObjectData.clear();

    PushFourSecondTick();

    QVERIFY(QTest::qWaitFor([] { return LogCapture::Contains(QLatin1String(kMd11SlowRuleObserved)); }, 2000));
    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());
}

void RuntimeIntegratorServiceTest::theSlowTickLeavesTheTakeoversAloneWhileTheAutomationIsOff()
{
#ifndef NDEBUG
    const LogCapture log;
    IntegratorRuntime runtime(TheProbeActsOnTheSim());
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::RequestFuel) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::RequestFuel);

    QVERIFY(TheFuelTakeoverStarts(updated));

    runtime.SetAutomationEnabled(false);
    QVERIFY(TheCouatlRaisesTheAutomationFlags());

    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(!LogCapture::Contains(QLatin1String(kRetakingFuelAndPayload)));

    runtime.SetAutomationEnabled(true);
    PushFourSecondTick();

    QVERIFY(QTest::qWaitFor([] { return LogCapture::Contains(QLatin1String(kRetakingFuelAndPayload)); }, 2000));
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::endingTheSessionForgetsTheAircraftStrings()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));

    QVERIFY(!runtime.IsSessionActive());

    QVERIFY(TheCameraMovesTo(kCockpitCamera, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.IsSessionActive());
    QCOMPARE(runtime.GetAircraftProfileId(), std::string{});

    QVERIFY(DetectWithTheGsxUp(runtime, updated, kRj85Title, kRj85AtcModel, kRj85ProfileId));
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheZfwTargetAndTheEmptyZfw()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectWithTheGsxUp(runtime, updated, kRj85Title, kRj85AtcModel, kRj85ProfileId));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::WaitingFlightPlan)
                           - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingFlightPlan);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().targetZfwKg, 0.0);
    QCOMPARE(runtime.Snapshot().emptyZfwKg, 0.0);

    QVERIFY(TheRj85PlanReachesWaitingPowerOn(runtime, updated));

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QCOMPARE(snapshot.targetZfwKg, kRj85PlannedZfwKg);
    QCOMPARE(snapshot.emptyZfwKg, kRj85EmptyWeightKg);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theSnapshotCarriesTheFuelOnBoardTheAircraftReportsNow()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectWithTheGsxUp(runtime, updated, kRj85Title, kRj85AtcModel, kRj85ProfileId));

    QCOMPARE(runtime.Snapshot().fuelOnBoardKg, 0.0);

    QVERIFY(PushDatum(simvars::kSimFuelTotalKg, kRj85FuelOnBoardKg));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelOnBoardKg, kRj85FuelOnBoardKg);

    QVERIFY(PushDatum(simvars::kSimFuelTotalKg, kRj85FuelOnBoardKg - 100.0));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelOnBoardKg, kRj85FuelOnBoardKg - 100.0);
}

void RuntimeIntegratorServiceTest::theFuelWaitsUntilTheRemoteApiAnnouncesItsConnection()
{
#ifndef NDEBUG
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    runtime.DebugSkipPhase(static_cast<int>(TurnaroundPhase::Loading) - static_cast<int>(runtime.GetPhase()));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::Loading);

    QVERIFY(PushLVar(gsx::lvars::kRefuelingState, static_cast<double>(GsxStateStatus::Completed)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelProgress, 0.0);

    FakeGsxRemoteApi::Receive(EmptySnapshot());
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().fuelProgress, 100.0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::openingSimConnectIsAnnouncedOncePerDisconnectedPeriod()
{
    const LogCapture log;
    IntegratorRuntimeOptions options;
    options.reconnectInterval = kFastReconnectInterval;
    IntegratorRuntime runtime(options);
    runtime.Setup();

    QCOMPARE(LogCapture::Count(QLatin1String(kOpeningSimConnect)), 1);

    const QSignalSpy quits(&runtime, &IntegratorRuntime::SimulatorQuit);
    FakeSimConnectApi::openSucceeds = false;
    constexpr SIMCONNECT_RECV quit{};
    FakeSimConnectApi::Push(quit, SIMCONNECT_RECV_ID_QUIT);

    QVERIFY(QTest::qWaitFor([&quits] { return quits.count() > 0; }, 2000));

    QSignalSpy retries(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(retries.wait(kReconnectWaitMs));
    QVERIFY(!runtime.IsConnected());
    QCOMPARE(LogCapture::Count(QLatin1String(kOpeningSimConnect)), 2);

    QVERIFY(retries.wait(kReconnectWaitMs));
    QVERIFY(!runtime.IsConnected());
    QCOMPARE(LogCapture::Count(QLatin1String(kOpeningSimConnect)), 2);
}

void RuntimeIntegratorServiceTest::theKeyReachesTheLogOncePerChangeAndNotPerMessage()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    const std::vector<RecordedMessage> wire = LoadRecordedWire(QLatin1String(kCdk2FlightWire));

    QVERIFY(!wire.empty());
    QCOMPARE(KeyLines(), qsizetype{0});

    ReceiveWire(wire);

    QCOMPARE(KeyLines(), qsizetype{kCdk2FlightKeyLines});
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 29495244, airport CDK2, parking Parking 2")));
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 29495244, airport CDK2, no parking")));
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 29495244, no airport, no parking")));
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 29495244, airport CYEG, parking Gate 8")));

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QStringLiteral("Gate 8")));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), false));

    QCOMPARE(KeyLines(), qsizetype{kCdk2FlightKeyLines});
}

void RuntimeIntegratorServiceTest::theKeyPathsAreNoLongerWarnedAsUnknown()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    const std::vector<RecordedMessage> newborn = LoadRecordedWire(QLatin1String(kNewbornCouatlWire));
    const std::vector<RecordedMessage> flight = LoadRecordedWire(QLatin1String(kCdk2FlightWire));

    QVERIFY(!newborn.empty());
    QVERIFY(!flight.empty());

    ReceiveWire(newborn);
    FakeGsxRemoteApi::AnnounceConnection(false);
    ReceiveWire(flight);

    QVERIFY(!LogCapture::Contains(QLatin1String(kUnknownPathWarning)));

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/somethingNobodyHasSeen"), QJsonValue()));

    QCOMPARE(LogCapture::Count(QLatin1String(kUnknownPathWarning)), qsizetype{1});
}

void RuntimeIntegratorServiceTest::aBlipOnTheSameCouatlLogsNoNewKey()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    const std::vector<RecordedMessage> wire = LoadRecordedWire(QLatin1String(kLfmnWire));

    QVERIFY(!wire.empty());

    ReceiveWire(wire);

    QCOMPARE(KeyLines(), qsizetype{1});

    FakeGsxRemoteApi::AnnounceConnection(false);

    QCOMPARE(KeyLines(), qsizetype{1});

    ReceiveWire(wire);

    QCOMPARE(KeyLines(), qsizetype{1});
}

void RuntimeIntegratorServiceTest::theCouatlRestartLogsWhatTheNewbornWireSays()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    const std::vector<RecordedMessage> lfmn = LoadRecordedWire(QLatin1String(kLfmnWire));
    const std::vector<RecordedMessage> newborn = LoadRecordedWire(QLatin1String(kNewbornCouatlWire));

    QVERIFY(!lfmn.empty());
    QVERIFY(!newborn.empty());

    ReceiveWire(lfmn);

    QCOMPARE(KeyLines(), qsizetype{1});
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 2120420471, airport LFMN, parking Terminal 1 | Gate C14")));

    FakeGsxRemoteApi::AnnounceConnection(false);
    ReceiveWire(newborn);

    QCOMPARE(KeyLines(), qsizetype{5});
    QVERIFY(LogCapture::Contains(QStringLiteral("GSX reports no couatl, no airport, no parking")));
    QVERIFY(LogCapture::Contains(QStringLiteral("GSX reports no couatl, airport LFMN, no parking")));
    QVERIFY(LogCapture::Contains(QStringLiteral("GSX reports couatl 2123650379, airport LFMN, no parking")));
    QVERIFY(LogCapture::Contains(
        QStringLiteral("GSX reports couatl 2123650379, airport LFMN, parking Terminal 1 | Gate C14")));
}

void RuntimeIntegratorServiceTest::theMachineDoesNotTickBetweenTheHandshakeAndTheSnapshot()
{
    IntegratorRuntime runtime;
    SkipTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    const TurnaroundPhase before = runtime.GetPhase();

    FakeGsxRemoteApi::Receive(Hello());

    QVERIFY(!DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QCOMPARE(runtime.GetPhase(), before);

    FakeGsxRemoteApi::Receive(EmptySnapshot());

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::theSlowTickWaitsForTheSnapshotToo()
{
    IntegratorRuntime runtime;
    SkipTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(ReachTheMd11SlowRule(runtime, updated));

    FakeSimConnectApi::writtenSimObjectData.clear();

    FakeGsxRemoteApi::Receive(Hello());
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());

    FakeGsxRemoteApi::Receive(EmptySnapshot());
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QVERIFY(WasWritten(kMd11EfbZfw));
}

void RuntimeIntegratorServiceTest::theMenuIsLeftAloneBetweenTheHandshakeAndTheSnapshot()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    FakeGsxRemoteApi::Receive(Hello());
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menu"),
                                    QJsonObject{{"title", "Activate Services"},
                                                {"entries", QJsonArray{"Call Pushback", "Cancel"}}}));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), true));

    QVERIFY(!LogCapture::Contains(QLatin1String(kMenuLine)));

    FakeGsxRemoteApi::Receive(EmptySnapshot());

    QVERIFY(LogCapture::Contains(QStringLiteral("RemoteAPI menu: 'Activate Services' -> [Call Pushback | Cancel]")));
}

void RuntimeIntegratorServiceTest::aDropKeepsTheMachineTicking()
{
    IntegratorRuntime runtime;
    SkipTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    ReceiveWire(LoadRecordedWire(QLatin1String(kLfmnWire)));
    FakeGsxRemoteApi::AnnounceConnection(false);

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::theLoggingToggleAloneLeavesTheAircraftUntouched()
{
#ifndef NDEBUG
    probe::SetEnabled(true);
    const auto probeOff = qScopeGuard(TurnTheProbeOff);

    IntegratorRuntime runtime;

    AutomationSettings settings;
    settings.autoStartFlow = false;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(!DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(!runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.GetAircraftProfileId(), std::string{});
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::theRunHeaderNamesTheRunFolderAndLeavesThePilotIdOut()
{
#ifndef NDEBUG
    probe::SetEnabled(true);
    const auto probeOff = qScopeGuard(TurnTheProbeOff);

    const LogCapture log;
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    AppSettings settings;
    settings.simbriefPilotId = kPersonalPilotId;
    settings.callCatering = true;
    service.ApplySettings(settings);

    runtime.Setup();

    QVERIFY(LogCapture::Contains(QStringLiteral("Logging run: folder=") + probe::RunLocation()));
    QVERIFY(LogCapture::Contains(QStringLiteral("build=debug")));
    QVERIFY(LogCapture::Contains(QStringLiteral("simbriefPilotId=set")));
    QVERIFY(LogCapture::Contains(QStringLiteral("callCatering=1")));
    QVERIFY(!LogCapture::Contains(QString::number(kPersonalPilotId)));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundHoldsTheMachineWhileTheRemoteApiNeverConnects()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    for (int tick = 0; tick < kHoldTickBudget; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);
    QVERIFY(!LogCapture::Contains(QLatin1String(kTransitionLine)));
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundIsJudgedEvenWhileTheAutomationIsOff()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    QVERIFY(!runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundFromAnotherStandIsErasedAndTheFlowStartsAsToday()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kSavedCouatl, kOtherStand));

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(!LogCapture::Contains(QLatin1String(kResumedLine)));
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundOfAnotherAircraftIsErased()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    store.stored->key.aircraftId = kRj85ProfileId;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    QCOMPARE(store.eraseCalls, 1);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundIsErasedUnderAnAircraftTheClientDoesNotSupport()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    const DWORD title = FakeSimConnectApi::DefineIdOf(kTitleDatum);
    const DWORD atcModel = FakeSimConnectApi::DefineIdOf(kAtcModelDatum);
    QVERIFY(title != 0);
    QVERIFY(atcModel != 0);
    FakeSimConnectApi::PushSimObjectString(title, kUnsupportedTitle);
    FakeSimConnectApi::PushSimObjectString(atcModel, kUnsupportedAtcModel);
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.GetAircraftProfileId().empty());
    QCOMPARE(store.eraseCalls, 1);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::aDifferentCouatlHoldsTheMachineAndAsksThePilot()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));

    for (int tick = 0; tick < kHoldTickBudget; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
    QVERIFY(!LogCapture::Contains(QLatin1String(kTransitionLine)));
    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(store.stored.has_value());
}

void RuntimeIntegratorServiceTest::answeringRestartErasesTheFileAndStartsTheFlow()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    runtime.RestartFlow();

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::answeringResumeRestoresTheSavedPhase()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    QVERIFY(service.ResumeSavedTurnaround().succeeded);
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(runtime.Snapshot().automationEnabled);
    QVERIFY(LogCapture::Contains(QLatin1String(kResumedLine)));
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::theResumeAnswerIsRefusedWhileTheSavedTurnaroundIsStillBeingJudged()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    const CommandResult refused = service.ResumeSavedTurnaround();

    QVERIFY(!refused.succeeded);
    QVERIFY(!refused.message.empty());
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);
}

void RuntimeIntegratorServiceTest::theResumeAnswerIsRefusedWhenThereIsNoSavedFile()
{
    FakeTurnaroundCheckpointStore store;
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::None);

    const CommandResult refused = service.ResumeSavedTurnaround();

    QVERIFY(!refused.succeeded);
    QVERIFY(!refused.message.empty());
}

void RuntimeIntegratorServiceTest::aRepeatedResumeWhileTheRestorationRunsSucceedsAndChangesNothing()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    QVERIFY(service.ResumeSavedTurnaround().succeeded);
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    const CommandResult again = service.ResumeSavedTurnaround();

    QVERIFY(again.succeeded);
    QVERIFY(again.message.empty());
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);

    const CommandResult afterwards = service.ResumeSavedTurnaround();

    QVERIFY(!afterwards.succeeded);
}

void RuntimeIntegratorServiceTest::theRestartIsAcceptedWhileTheSavedTurnaroundHoldsWithTheAutomationOff()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    AppSettings appSettings;
    appSettings.autoStartFlow = false;
    service.ApplySettings(appSettings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);
    QVERIFY(!service.GetSnapshot().automationEnabled);

    QVERIFY(service.RestartFlow().succeeded);

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::theResumeAnswerIsRefusedWhileOffline()
{
    IntegratorRuntime runtime;
    RuntimeIntegratorService service(&runtime);

    const CommandResult refused = service.ResumeSavedTurnaround();

    QVERIFY(!refused.succeeded);
    QCOMPARE(refused.message, std::string("Simulator is offline."));
}

void RuntimeIntegratorServiceTest::aResumedServiceThatReadsAvailableCountsAsDoneOnTheSameCouatlAndInterruptedAfterARestart()
{
    constexpr auto kRefuelingStatusEntry = "gsx.service.refueling.status";
    constexpr auto kRefuelingStarted = static_cast<double>(GsxStateStatus::Active);

    for (const bool couatlRestarted : {false, true})
    {
        FakeSimConnectApi::Reset();
        FakeGsxRemoteApi::Reset();

        FakeTurnaroundCheckpointStore store;
        store.stored = SavedAt(TurnaroundPhase::Loading);
        store.stored->checkpoint->data.refuelBaselined = true;
        MemoryBag service;
        service.PutNumber(kRefuelingStatusEntry, kRefuelingStarted);
        store.stored->memoryByOwner["gsxService"] = service;
        IntegratorRuntime runtime(WithTheStore(store));
        runtime.Setup();

        QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

        QVERIFY(TheKeyIsJudged(runtime, updated, couatlRestarted ? kRestartedCouatl : kSavedCouatl));

        if (couatlRestarted)
        {
            runtime.ResumeSavedTurnaround();
        }

        QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
        QVERIFY(TickAndWait(updated));

        const IntegratorSnapshot snapshot = runtime.Snapshot();

        QCOMPARE(snapshot.serviceInterrupted, couatlRestarted);
        QCOMPARE(snapshot.fuelProgress == 100.0, !couatlRestarted);
    }
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundOfTheSameKeyResumesWithoutRepeatingAnything()
{
    constexpr auto kMd11EfbFuel = "L:MD11_EFB_PAYLOAD_FUEL";

    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->checkpoint->data.boardingFinished = true;
    store.stored->checkpoint->data.refuelFinished = true;
    store.stored->checkpoint->data.loadingStartNotified = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    FakeGsxRemoteApi::commandVerbs.clear();
    FakeSimConnectApi::writtenSimObjectData.clear();

    QVERIFY(TickAndWait(updated));
    PushFourSecondTick();
    QVERIFY(DispatchPending());

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);

    QVERIFY(TickAndWait(updated));
    PushFourSecondTick();
    QVERIFY(DispatchPending());
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(FakeGsxRemoteApi::commandVerbs.empty());
    QVERIFY(!WasWritten(kMd11EfbZfw));
    QVERIFY(!WasWritten(kMd11EfbFuel));
    QVERIFY(FakeSimConnectApi::writtenSimObjectData.empty());
}

void RuntimeIntegratorServiceTest::aSavedFlightWaitsForTheGroundReadingBeforeTheFirstTick()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::OnFlight);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheReadingsArrive(updated, kSimOnGroundDatum));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    QVERIFY(PushDatum(kSimOnGroundDatum, kInTheAir));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::OnFlight);

    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::OnFlight);
}

void RuntimeIntegratorServiceTest::eachResumeReadingMissingAloneHoldsTheMachine_data()
{
    QTest::addColumn<QString>("withheld");
    QTest::addColumn<bool>("withServices");
    QTest::addColumn<bool>("aircraftIsTheOneMissing");

    for (const ResumeReading& reading : kResumeReadings)
    {
        const bool aircraftReading = std::string_view(reading.datum) == simvars::kSimEmptyWeight
            || std::string_view(reading.datum) == simvars::kSimFuelTotalKg;
        QTest::newRow(reading.datum) << QString::fromLatin1(reading.datum) << true << aircraftReading;
    }

    QTest::newRow("the service list") << QString() << false << false;
}

void RuntimeIntegratorServiceTest::eachResumeReadingMissingAloneHoldsTheMachine()
{
    QFETCH(QString, withheld);
    QFETCH(bool, withServices);
    QFETCH(bool, aircraftIsTheOneMissing);

    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand, withServices));

    QVERIFY(TheReadingsArrive(updated, withheld.toStdString()));

    QCOMPARE(runtime.Snapshot().turnaroundHold,
             aircraftIsTheOneMissing ? TurnaroundHold::AwaitingAircraft : TurnaroundHold::AwaitingGsxReadings);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    if (!withServices)
    {
        FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    }

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
}

void RuntimeIntegratorServiceTest::theButtonErasesTheSavedTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(service.RestartFlow().succeeded);

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::aTransientSessionEndKeepsTheFileAndTheFlowResumesAgain()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);

    QVERIFY(TickAndWait(updated));
    QVERIFY(store.writeCalls > 0);

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));

    QVERIFY(!runtime.IsSessionActive());
    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(store.stored.has_value());

    QVERIFY(TheCameraMovesTo(kCockpitCamera, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.IsSessionActive());
    QCOMPARE(store.eraseCalls, 0);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::aMenuPatchAfterATransientSessionEndLeavesTheSavedFileIntact()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(TickAndWait(updated));

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));

    const std::optional<TurnaroundDocument> kept = store.stored;
    const int writesBefore = store.writeCalls;

    QVERIFY(kept.has_value());
    QVERIFY(kept->checkpoint.has_value());

    ReceiveAMenu();

    QVERIFY(TheCameraMovesTo(kCockpitCamera, updated));
    QVERIFY(TickAndWait(updated));

    ReceiveAMenu();

    QVERIFY(TheKeyIsJudged(runtime, updated));

    ReceiveAMenu();

    QCOMPARE(store.writeCalls, writesBefore);
    QVERIFY(store.stored == kept);
}

void RuntimeIntegratorServiceTest::withoutAPortTheFlowStillWaitsForTheButton()
{
    IntegratorRuntime runtime;
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(!runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
}

void RuntimeIntegratorServiceTest::withAPortAndNoFileTheFlowStillWaitsForTheButton()
{
    FakeTurnaroundCheckpointStore store;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(!runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
    QCOMPARE(store.readCalls, 1);
    QCOMPARE(store.writeCalls, 0);
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::aResumedTurnaroundRunsWhateverTheAutoStartOptionSays()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(!runtime.Snapshot().automationEnabled);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QVERIFY(runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
}

void RuntimeIntegratorServiceTest::aSavedPhaseTheMachineDoesNotKnowErasesTheFileAndStartsTheFlow()
{
#ifndef NDEBUG
    probe::SetEnabled(true);
    const auto probeOff = qScopeGuard(TurnTheProbeOff);
#endif

    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingNewFlight);
    IntegratorRuntimeOptions options = WithTheStore(store);
    options.actsOnTheSim = [] { return true; };
    IntegratorRuntime runtime(options);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    QTest::qWait(kProbeObserveIntervalMs);

    QVERIFY(TheReadingsArrive(updated));

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QVERIFY(!LogCapture::Contains(QLatin1String(kResumedLine)));
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWithoutTheMarkIsIgnored()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    const std::optional<TurnaroundDocument> original = store.stored;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    FakeGsxRemoteApi::Receive(GsxWire(kRestartedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(store.stored == original);

    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWithoutTheMarkOfAnotherAircraftIsNotErased()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->key.aircraftId = kRj85ProfileId;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(store.stored.has_value());
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWithTheMarkOfAnotherAircraftIsErased()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->key.aircraftId = kRj85ProfileId;
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
}

void RuntimeIntegratorServiceTest::theGsxMenuOfASnapshotIsLeftAloneWhileTheSavedTurnaroundHolds()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    QJsonObject snapshot = GsxWire(kSavedCouatl, kSavedStand);
    snapshot.insert("menu", AMenu());
    snapshot.insert("menuShown", true);
    FakeGsxRemoteApi::Receive(snapshot);

    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.Snapshot().turnaroundHold != TurnaroundHold::None);
    QVERIFY(!LogCapture::Contains(QLatin1String(kMenuAnswerLine)));
}

void RuntimeIntegratorServiceTest::theGsxMenuOfAPatchIsLeftAloneWhileTheSavedTurnaroundHolds()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menu"), AMenu()));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), true));

    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.Snapshot().turnaroundHold != TurnaroundHold::None);
    QVERIFY(!LogCapture::Contains(QLatin1String(kMenuAnswerLine)));
}

void RuntimeIntegratorServiceTest::theSlowRulesAreOnlyObservedWhileTheSavedTurnaroundHolds()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(TickAndWait(updated));
    QVERIFY(runtime.Snapshot().automationEnabled);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QVERIFY(!LogCapture::Contains(QLatin1String(kMd11SlowRuleObserved)));

    PushFourSecondTick();

    QVERIFY(QTest::qWaitFor([] { return LogCapture::Contains(QLatin1String(kMd11SlowRuleObserved)); }, 2000));
}

void RuntimeIntegratorServiceTest::theSimbriefReloadIsRefusedWhileTheSavedTurnaroundHolds()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));

    AutomationSettings settings;
    settings.simbriefPilotId = kPersonalPilotId;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    QVERIFY(!runtime.ReloadSimbrief());
}

void RuntimeIntegratorServiceTest::aRestoredPlanLeavesTheStatusReadyAndComesBackInTheNextWrite()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    FlightPlan plan;
    plan.fuelKg = kMd11PlannedFuelKg;
    plan.zfwKg = kMd11PlannedZfwKg;
    plan.passengers = kMd11PlannedPassengers;
    plan.origin = "CDK2";
    plan.destination = "CYEG";
    plan.generatedEpoch = kPlanEpoch;
    plan.operatingEmptyKg = kMd11EmptyWeightKg;
    store.stored->plan = plan;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().flightPlanStatus, FlightPlanStatus::Idle);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.Snapshot().flightPlanStatus, FlightPlanStatus::Ready);

    QVERIFY(TickAndWait(updated));

    QVERIFY(store.stored.has_value());
    QVERIFY(store.stored->plan.has_value());
    QCOMPARE(store.stored->plan->fuelKg, kMd11PlannedFuelKg);
    QCOMPARE(store.stored->plan->destination, std::string("CYEG"));
}

void RuntimeIntegratorServiceTest::theServiceAndMenuMemoryComeBackOnlyAfterTheServiceListArrives()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    MemoryBag service;
    service.PutFlag("gsx.service.refueling.completed", true);
    store.stored->memoryByOwner["gsxService"] = service;
    MemoryBag menu;
    menu.PutFlag("deIceYesSpent", true);
    store.stored->memoryByOwner["gsxMenu"] = menu;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand, false));

    QVERIFY(TheReadingsArrive(updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QVERIFY(!store.stored->memoryByOwner.empty());

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.stored->memoryByOwner.at("gsxService").Flag("gsx.service.refueling.completed", false));
    QVERIFY(store.stored->memoryByOwner.at("gsxMenu").Flag("deIceYesSpent", false));
}

void RuntimeIntegratorServiceTest::nothingIsWrittenBeforeTheCouatlIdIsKnown()
{
    FakeTurnaroundCheckpointStore store;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.writeCalls, 0);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.writeCalls > 0);
    QVERIFY(store.stored.has_value());
    QCOMPARE(store.stored->key.couatlId, std::string(kSavedCouatl));
}

void RuntimeIntegratorServiceTest::nothingIsWrittenInTheFirstTwoPhases()
{
    FakeTurnaroundCheckpointStore store;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::WaitingAircraftReady, runtime, updated));
    QCOMPARE(store.writeCalls, 0);

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::RepositionAircraft, runtime, updated));

    QVERIFY(store.writeCalls > 0);
    QVERIFY(store.stored->checkpoint.has_value());
    QCOMPARE(store.stored->checkpoint->phase, TurnaroundPhase::RepositionAircraft);
}

void RuntimeIntegratorServiceTest::sixtyTicksWithoutAnEventDoNotWriteAgain()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(TickAndWait(updated));

    const int writes = store.writeCalls;

    QVERIFY(writes > 0);

    for (int tick = 0; tick < kQuietTickBudget; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(store.writeCalls, writes);
}

void RuntimeIntegratorServiceTest::enteringWaitingNewFlightWritesOnlyTheKey()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::CabinServices);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::CabinServices);
    QVERIFY(store.stored->checkpoint.has_value());

    for (int tick = 0; tick < kCabinExitTickBudget && runtime.GetPhase() != TurnaroundPhase::WaitingNewFlight; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingNewFlight);
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.stored.has_value());
    QVERIFY(!store.stored->checkpoint.has_value());
    QVERIFY(!store.stored->plan.has_value());
    QVERIFY(store.stored->memoryByOwner.empty());
    QVERIFY(store.stored->key == Md11Key());
    QVERIFY(!store.stored->repositioned);
}

void RuntimeIntegratorServiceTest::aFailedWriteLeavesTheLastWrittenBehindAndIsTriedAgain()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.writeResult = false;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));

    const int failures = store.writeCalls;

    QVERIFY(failures > 0);
    QVERIFY(!store.stored.has_value());

    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.writeCalls > failures);
    QCOMPARE(LogCapture::Count(QLatin1String(kWriteWarning)), qsizetype{1});

    store.writeResult = true;
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.stored.has_value());
}

void RuntimeIntegratorServiceTest::aFailedEraseDoesNotHoldTheMachineAndIsTriedAgainEverySecond()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    store.eraseResult = false;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kSavedCouatl, kOtherStand));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(store.eraseCalls, 1);

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(TickAndWait(updated));
    }

    QCOMPARE(store.eraseCalls, 6);
    QCOMPARE(LogCapture::Count(QLatin1String(kEraseWarning)), qsizetype{1});

    store.eraseResult = true;
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, 7);
    QVERIFY(!store.stored.has_value());
}

void RuntimeIntegratorServiceTest::aFailedEraseLetsTheMachineRunAndASuccessfulWriteSettlesIt()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    store.eraseResult = false;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kSavedCouatl, kOtherStand));
    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.writeCalls > 0);

    const int erases = store.eraseCalls;

    QVERIFY(erases > 1);

    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, erases);
}

void RuntimeIntegratorServiceTest::aDocumentWhoseErasureFailedIsNeverRestoredInThisProcess()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.eraseResult = false;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kSavedCouatl, kOtherStand));
    QCOMPARE(store.readCalls, 1);

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));
    QVERIFY(TheCameraMovesTo(kCockpitCamera, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(TheKeyIsJudged(runtime, updated, kSavedCouatl, kSavedStand));
    QVERIFY(TheReadingsArrive(updated));

    QCOMPARE(store.readCalls, 1);
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QVERIFY(!LogCapture::Contains(QLatin1String(kResumedLine)));
}

void RuntimeIntegratorServiceTest::anUnstartedFlowLeavesTheGsxMenuOfASnapshotAlone()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));
    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(TickAndWait(updated));
    QVERIFY(!runtime.Snapshot().automationEnabled);
    QVERIFY(runtime.Snapshot().gsxAvailable);

    QJsonObject snapshot = GsxWire(kSavedCouatl, kSavedStand);
    snapshot.insert("menu", TheAirstairsQuestion());
    snapshot.insert("menuShown", true);
    FakeGsxRemoteApi::Receive(snapshot);

    QVERIFY(FakeGsxRemoteApi::commandVerbs.empty());

    runtime.SetAutomationEnabled(true);
    FakeGsxRemoteApi::Receive(snapshot);

    QCOMPARE(FakeGsxRemoteApi::commandVerbs, std::vector<std::string>{kMenuPickVerb});
}

void RuntimeIntegratorServiceTest::anUnstartedFlowLeavesTheGsxMenuOfAPatchAlone()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    StartTheFlowWithoutTheReposition(runtime, false);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    PushUnpaused();
    QVERIFY(TickAndWait(updated));
    QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
    QVERIFY(TickAndWait(updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    ReceiveAMenuPatch(TheAirstairsQuestion());

    QVERIFY(FakeGsxRemoteApi::commandVerbs.empty());

    runtime.SetAutomationEnabled(true);
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), false));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/menuShown"), true));

    QCOMPARE(FakeGsxRemoteApi::commandVerbs, std::vector<std::string>{kMenuPickVerb});
}

void RuntimeIntegratorServiceTest::endingTheSessionForgetsTheAircraftName()
{
    IntegratorRuntime runtime;
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(!runtime.Snapshot().aircraftName.empty());

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));

    QVERIFY(runtime.Snapshot().aircraftName.empty());
}

void RuntimeIntegratorServiceTest::theHandshakeWithoutASnapshotIsPublishedAsTheReasonToWait()
{
    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    FakeGsxRemoteApi::Receive(Hello());

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxSnapshot);
    QCOMPARE(LogCapture::Count(QLatin1String(kHandshakeLine)), qsizetype{1});

    FakeGsxRemoteApi::Receive(EmptySnapshot());

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::theHandshakeReasonWinsOverTheSavedTurnaroundOne()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    FakeGsxRemoteApi::Receive(Hello());

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxSnapshot);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
}

void RuntimeIntegratorServiceTest::aRuntimeResumesTheFileAnotherRuntimeWrote()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    TurnaroundPhase written = TurnaroundPhase::WaitingSupportedAircraft;
    {
        JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
        IntegratorRuntime runtime(WithTheStore(store));
        StartTheFlowWithoutTheReposition(runtime);
        runtime.Setup();

        QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

        FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

        QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
        QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
        QVERIFY(TickAndWait(updated));

        written = runtime.GetPhase();

        QVERIFY(QFile::exists(QDir(directory.path()).filePath(QString(kCheckpointFile))));
    }

    FakeSimConnectApi::Reset();
    FakeGsxRemoteApi::Reset();

    JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QVERIFY(written >= TurnaroundPhase::PlaceGroundEquipment);
    QCOMPARE(runtime.GetPhase(), written);
}

void RuntimeIntegratorServiceTest::aTitleWithBytesOutsideUtf8SurvivesTheFile()
{
    constexpr auto kStrayByteTitle = "TFDi Design MD-11 PAX \xE9 livery";

    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    {
        JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
        IntegratorRuntime runtime(WithTheStore(store));
        StartTheFlowWithoutTheReposition(runtime);
        runtime.Setup();

        QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

        FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

        QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated, kStrayByteTitle));
        QVERIFY(PushLVar(gsx::lvars::kCouatlStarted, 1.0));
        QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
        QVERIFY(TickAndWait(updated));
    }

    FakeSimConnectApi::Reset();
    FakeGsxRemoteApi::Reset();

    JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
    QVERIFY(store.Read().has_value());

    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithoutTheGsx(runtime, updated, kStrayByteTitle));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QVERIFY(store.Read().has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
}

void RuntimeIntegratorServiceTest::anUnreadableFileNeverRestoresAndNeverHolds()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QFile garbage(QDir(directory.path()).filePath(QString(kCheckpointFile)));
    QVERIFY(garbage.open(QIODevice::WriteOnly));
    QVERIFY(garbage.write("{ \"clientVersion\": \"1.0.0-test\", \"key\": ") > 0);
    garbage.close();

    JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(store.Read().has_value());
}

void RuntimeIntegratorServiceTest::aFileOfAnotherClientVersionNeverRestoresAndNeverHolds()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    {
        JsonFileTurnaroundCheckpointStore older(directory.path(), QStringLiteral("0.0.1-test"));
        QVERIFY(older.Write(SavedAt(TurnaroundPhase::WaitingReadyToPush)));
    }

    JsonFileTurnaroundCheckpointStore store(directory.path(), kClientVersion);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::aSimulatorQuitWithoutASessionLeavesTheAutomationAlone()
{
    const LogCapture log;
    IntegratorRuntimeOptions options;
    options.reconnectInterval = kFastReconnectInterval;
    IntegratorRuntime runtime(options);
    runtime.Setup();
    runtime.ApplySettings(AutomationSettings{});

    QVERIFY(runtime.Snapshot().automationEnabled);
    QVERIFY(!runtime.IsSessionActive());

    const QSignalSpy quits(&runtime, &IntegratorRuntime::SimulatorQuit);
    constexpr SIMCONNECT_RECV quit{};
    FakeSimConnectApi::Push(quit, SIMCONNECT_RECV_ID_QUIT);

    QVERIFY(QTest::qWaitFor([&quits] { return quits.count() > 0; }, 2000));

    QVERIFY(runtime.Snapshot().automationEnabled);
    QVERIFY(!LogCapture::Contains(QStringLiteral("Session ended")));
}

void RuntimeIntegratorServiceTest::aNewCouatlLifeThatServesTheSameSimbriefGenerationIsAnnounced()
{
    constexpr auto kGenerationLine = "GSX served SimBrief generation 3";

    const LogCapture log;
    IntegratorRuntime runtime;
    runtime.Setup();

    QJsonObject snapshot = GsxWire(kSavedCouatl, kSavedStand);
    snapshot.insert("simbrief", QJsonObject{{"status", "ok"}, {"gen", 3}});

    FakeGsxRemoteApi::Receive(snapshot);

    QCOMPARE(LogCapture::Count(QLatin1String(kGenerationLine)), qsizetype{1});

    FakeGsxRemoteApi::AnnounceConnection(false);
    snapshot.insert("startup", QJsonObject{{"sid", kRestartedCouatl}});
    FakeGsxRemoteApi::Receive(snapshot);

    QCOMPARE(LogCapture::Count(QLatin1String(kGenerationLine)), qsizetype{2});
}

void RuntimeIntegratorServiceTest::thePlanIsNotAdoptedWhileTheAircraftIsStillUnreachable()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->plan = FlightPlan{.fuelKg = kMd11PlannedFuelKg, .zfwKg = kMd11PlannedZfwKg};
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheReadingsArrive(updated, simvars::kSimFuelTotalKg));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingAircraft);
    QCOMPARE(runtime.Snapshot().flightPlanStatus, FlightPlanStatus::Idle);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.Snapshot().flightPlanStatus, FlightPlanStatus::Ready);
}

void RuntimeIntegratorServiceTest::aCouatlThatRestartsWhileTheReadingsAreAwaitedAsksThePilot()
{
    constexpr auto kRefuelingStatusEntry = "gsx.service.refueling.status";
    constexpr auto kRefuelingStarted = static_cast<double>(GsxStateStatus::Active);

    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    store.stored->checkpoint->data.refuelBaselined = true;
    MemoryBag service;
    service.PutNumber(kRefuelingStatusEntry, kRefuelingStarted);
    store.stored->memoryByOwner["gsxService"] = service;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    TheRemoteApiReconnectsAs(kRestartedCouatl, kSavedStand);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);
    QCOMPARE(store.eraseCalls, 0);

    runtime.ResumeSavedTurnaround();

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));

    const IntegratorSnapshot snapshot = runtime.Snapshot();

    QVERIFY(snapshot.serviceInterrupted);
    QVERIFY(snapshot.fuelProgress != 100.0);
}

void RuntimeIntegratorServiceTest::aParkingThatChangesWhileThePilotDecidesErasesTheFile()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::Loading);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    TheRemoteApiReconnectsAs(kRestartedCouatl, kOtherStand);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::aCouatlThatRestartsAfterTheResumeAnswerDoesNotAskAgain()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    runtime.ResumeSavedTurnaround();

    TheRemoteApiReconnectsAs(kThirdCouatl, kSavedStand);
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(LogCapture::Contains(QLatin1String(kResumedLine)));
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::theQuestionSurvivesTheHandshakeThatComesBeforeTheSnapshot()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    RuntimeIntegratorService service(&runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    FakeGsxRemoteApi::AnnounceConnection(false);
    FakeGsxRemoteApi::AnnounceConnection(true);

    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);
    QVERIFY(service.ResumeSavedTurnaround().succeeded);
    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingGsxSnapshot);

    FakeGsxRemoteApi::Receive(GsxWire(kRestartedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(service.GetSnapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
}

void RuntimeIntegratorServiceTest::theSimbriefReloadIsNotOfferedWhileTheSavedTurnaroundHolds()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingFlightPlan);
    IntegratorRuntime runtime(WithTheStore(store));

    AutomationSettings settings;
    settings.simbriefPilotId = kPersonalPilotId;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);
    QVERIFY(!runtime.Snapshot().canReloadSimbrief);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingFlightPlan);
    QVERIFY(runtime.Snapshot().canReloadSimbrief);
}

void RuntimeIntegratorServiceTest::theGsxMenuAnswerIsSavedRightAfterTheNavigatorSends()
{
    FakeTurnaroundCheckpointStore store;
    IntegratorRuntime runtime(WithTheStore(store));

    AutomationSettings settings;
    settings.skipReposition = true;
    settings.autoDeice = true;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(store.stored.has_value());
    QVERIFY(store.stored->memoryByOwner.contains("gsxMenu"));
    QVERIFY(!store.stored->memoryByOwner.at("gsxMenu").Flag("deIceYesSpent", true));

    const int writes = store.writeCalls;

    ReceiveAMenuPatch(TheDeIceQuestion());

    QCOMPARE(FakeGsxRemoteApi::commandVerbs.back(), std::string(kMenuPickVerb));
    QCOMPARE(store.writeCalls, writes + 1);
    QVERIFY(store.stored->memoryByOwner.at("gsxMenu").Flag("deIceYesSpent", false));
}

void RuntimeIntegratorServiceTest::aSavedTurnaroundWithoutAParkingResumesOnceTheLiveOneIsKnown()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->key.parkingName.clear();
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingGsxReadings);

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(LogCapture::Contains(QLatin1String(kResumedLine)));
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::restartingWhileTheSavedTurnaroundIsBeingJudgedErasesItAndNeverRestoresIt()
{
    const LogCapture log;
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    runtime.RestartFlow();

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TheReadingsArrive(updated));

    QCOMPARE(store.readCalls, 1);
    QVERIFY(!LogCapture::Contains(QLatin1String(kResumedLine)));
}

void RuntimeIntegratorServiceTest::endingTheSessionWhileTheQuestionIsOpenKeepsTheFileAndAsksAgain()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    QVERIFY(TheCameraMovesTo(kWorldMapCamera, updated));

    QVERIFY(!runtime.IsSessionActive());
    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(store.stored.has_value());

    QVERIFY(TheCameraMovesTo(kCockpitCamera, updated));
    QVERIFY(TickAndWait(updated));

    QVERIFY(runtime.IsSessionActive());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);
    QCOMPARE(store.eraseCalls, 0);
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileThatCarriesTheMarkHoldsTheMachineUntilItsKeyIsJudged()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    StartTheFlowWithoutTheReposition(runtime);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
}

void RuntimeIntegratorServiceTest::theMarkOfAKeyOnlyFileOfTheSameKeyReachesTheMachine()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TheRepositionIsSpared(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(store.stored->checkpoint.has_value());
    QVERIFY(store.stored->repositioned);
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWithoutTheMarkLeavesTheRepositionToTheTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TheRepositionIsAsked(runtime, updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::RepositionAircraft);
}

void RuntimeIntegratorServiceTest::theMarkDiesWhenOnlyTheCouatlDiffersFromAKeyOnlyFile()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kRestartedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TheRepositionIsAsked(runtime, updated));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
}

void RuntimeIntegratorServiceTest::theMarkDoesNotSpareTheRepositionWhenTheNewTurnaroundOptionIsOff()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    AutomationSettings settings;
    settings.skipRepositionOnNewTurnaround = false;
    runtime.ApplySettings(settings);
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TheRepositionIsAsked(runtime, updated));
}

void RuntimeIntegratorServiceTest::theRestartButtonKillsTheMark()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(store.stored->repositioned);

    runtime.RestartFlow();

    QVERIFY(!store.stored.has_value());

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TheRepositionIsAsked(runtime, updated));
    QVERIFY(store.stored.has_value());
    QVERIFY(!store.stored->repositioned);
}

void RuntimeIntegratorServiceTest::theMarkOfAResumedTurnaroundIsWrittenBackWithEveryDocument()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(store.writeCalls > 0);
    QVERIFY(store.stored->checkpoint.has_value());
    QVERIFY(store.stored->repositioned);
}

void RuntimeIntegratorServiceTest::theMarkDiesWhenThePilotResumesOnAnotherCouatl()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated, kRestartedCouatl));
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::AwaitingResumeDecision);

    runtime.ResumeSavedTurnaround();

    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(store.stored->checkpoint.has_value());
    QVERIFY(!store.stored->repositioned);
}

void RuntimeIntegratorServiceTest::enteringWaitingNewFlightWritesTheKeyAndTheMarkAndARelaunchKeepsIt()
{
    FakeTurnaroundCheckpointStore store;

    {
        store.stored = SavedAt(TurnaroundPhase::CabinServices);
        store.stored->repositioned = true;
        IntegratorRuntime runtime(WithTheStore(store));
        runtime.Setup();

        QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

        QVERIFY(TheKeyIsJudged(runtime, updated));
        QVERIFY(TheSavedTurnaroundResumes(runtime, updated));

        for (int tick = 0; tick < kCabinExitTickBudget && runtime.GetPhase() != TurnaroundPhase::WaitingNewFlight;
             ++tick)
        {
            QVERIFY(TickAndWait(updated));
        }

        QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingNewFlight);
        QVERIFY(TickAndWait(updated));

        QVERIFY(!store.stored->checkpoint.has_value());
        QVERIFY(store.stored->key == Md11Key());
        QVERIFY(store.stored->repositioned);
    }

    FakeSimConnectApi::Reset();
    FakeGsxRemoteApi::Reset();

    IntegratorRuntime relaunched(WithTheStore(store));
    relaunched.Setup();

    QSignalSpy updated(&relaunched, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kSavedStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(relaunched, updated));
    QVERIFY(DriveTheFlowInto(TurnaroundPhase::PlaceGroundEquipment, relaunched, updated));
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWithTheMarkOfAnotherStandIsErasedAndTheRepositionIsAsked()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    FakeGsxRemoteApi::Receive(GsxWire(kSavedCouatl, kOtherStand));

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QVERIFY(TheRepositionIsAsked(runtime, updated));
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWaitsForTheStandAndIsErasedWhenItArrivesDifferent()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QJsonObject snapshot = GsxWire(kSavedCouatl, kSavedStand);
    snapshot.remove("parking");
    FakeGsxRemoteApi::Receive(snapshot);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);
    QCOMPARE(store.eraseCalls, 0);

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kOtherStand)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(store.eraseCalls, 1);
    QVERIFY(!store.stored.has_value());
    QVERIFY(TheRepositionIsAsked(runtime, updated));
}

void RuntimeIntegratorServiceTest::aKeyOnlyFileWaitsForTheStandAndSparesTheRepositionWhenItArrivesTheSame()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedWithTheKeyOnly();
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QJsonObject snapshot = GsxWire(kSavedCouatl, kSavedStand);
    snapshot.remove("parking");
    FakeGsxRemoteApi::Receive(snapshot);

    QVERIFY(DetectTheMd11WithTheGsxUp(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::JudgingSavedTurnaround);

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kSavedStand)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(runtime.Snapshot().turnaroundHold, TurnaroundHold::None);
    QCOMPARE(store.eraseCalls, 0);
    QVERIFY(TheRepositionIsSpared(runtime, updated));
}

void RuntimeIntegratorServiceTest::aStandChangedWithoutFlyingKillsTheMarkOfTheFinishedTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::CabinServices);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TheFinishedTurnaroundWaitsForTheNextOne(runtime, updated));

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kOtherStand)));
    QVERIFY(TickAndWait(updated));

    runtime.AcceptPilotTouch();

    QVERIFY(TheRepositionIsAsked(runtime, updated));
}

void RuntimeIntegratorServiceTest::theSameStandKeepsTheMarkOfTheFinishedTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::CabinServices);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TheFinishedTurnaroundWaitsForTheNextOne(runtime, updated));
    QVERIFY(TickAndWait(updated));

    runtime.AcceptPilotTouch();

    QVERIFY(TheRepositionIsSpared(runtime, updated));
}

void RuntimeIntegratorServiceTest::aStandThatGoesEmptyAndComesBackKeepsTheMarkOfTheFinishedTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::CabinServices);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TheFinishedTurnaroundWaitsForTheNextOne(runtime, updated));

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(QString())));
    QVERIFY(TickAndWait(updated));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kSavedStand)));
    QVERIFY(TickAndWait(updated));

    runtime.AcceptPilotTouch();

    QVERIFY(TheRepositionIsSpared(runtime, updated));
}

void RuntimeIntegratorServiceTest::aStandChangedWithTheTurnaroundUnderWayErasesItAndRepositionsAtTheNewStand()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);

    const int erasesBefore = store.eraseCalls;

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kOtherStand)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, erasesBefore + 1);
    QVERIFY(runtime.GetPhase() < TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(TheRepositionIsAsked(runtime, updated));
}

void RuntimeIntegratorServiceTest::aStandThatGoesEmptyAndComesBackKeepsTheTurnaroundUnderWay()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingReadyToPush);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));

    const int erasesBefore = store.eraseCalls;

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(QString())));
    QVERIFY(TickAndWait(updated));
    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kSavedStand)));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, erasesBefore);
    QCOMPARE(runtime.GetPhase(), TurnaroundPhase::WaitingReadyToPush);
}

void RuntimeIntegratorServiceTest::aStandChangedAfterThePushbackWasAskedKeepsTheTurnaround()
{
    FakeTurnaroundCheckpointStore store;
    store.stored = SavedAt(TurnaroundPhase::WaitingDeparture);
    store.stored->repositioned = true;
    IntegratorRuntime runtime(WithTheStore(store));
    runtime.Setup();

    QSignalSpy updated(&runtime, &IntegratorRuntime::Updated);

    QVERIFY(TheKeyIsJudged(runtime, updated));
    QVERIFY(TheSavedTurnaroundResumes(runtime, updated));
    QVERIFY(TickAndWait(updated));

    const int erasesBefore = store.eraseCalls;

    FakeGsxRemoteApi::Receive(Patch(QStringLiteral("/parking"), QJsonValue(kOtherStand)));
    QVERIFY(TickAndWait(updated));
    QVERIFY(TickAndWait(updated));

    QCOMPARE(store.eraseCalls, erasesBefore);
    QVERIFY(runtime.GetPhase() >= TurnaroundPhase::RequestPushback);
}

QTEST_GUILESS_MAIN(RuntimeIntegratorServiceTest)

#include "tst_runtime_integrator_service.moc"
