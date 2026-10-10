#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include "AircraftTicks.h"
#include "doubles/FakePmdg737DataGateway.h"
#include "doubles/FakePmdgTabletGateway.h"
#include "doubles/FakeVariableGateway.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/infrastructure/aircraft/pmdg/Pmdg737.h"
#include "../src/infrastructure/gsx/GsxLVars.h"
#include "../src/infrastructure/pmdg/PmdgRouteFile.h"

namespace
{
    constexpr auto kChocksLVar = "NGXWheelChocks";
    constexpr auto kSmartSwitchLVar = "switch_752_73X";
    constexpr double kSmartSwitchNeutral = 50.0;
    constexpr double kSmartSwitchDown = 0.0;
    constexpr double kSmartSwitchUp = 100.0;
    constexpr auto kPassengerEntryRequest = "pax_entree";
    constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    constexpr double kJetwayDocked = 5.0;
    constexpr int kTicksWithoutEcho = 5;
    constexpr auto kFwdEntryKey = "entry1_left";
    constexpr auto kMainCargoKey = "main_cargo";

    constexpr std::array kEfbDoorKeys =
        {"entry1_left", "entry1_right", "entry2_left", "entry2_right",
         "fwd_cargo", "aft_cargo", "main_cargo", "equipment_hatch"};

    struct Pmdg737Fixture
    {
        FakeVariableGateway gateway;
        AutomationStatus status;
        FakePmdg737DataGateway* data = nullptr;
        FakePmdgTabletGateway* tablet = nullptr;
        std::unique_ptr<Pmdg737> aircraft;

        explicit Pmdg737Fixture(const Pmdg737Variant variant = Pmdg737Variant::Pax800)
        {
            auto dataGateway = std::make_unique<FakePmdg737DataGateway>();
            auto tabletGateway = std::make_unique<FakePmdgTabletGateway>();
            data = dataGateway.get();
            tablet = tabletGateway.get();
            aircraft = std::make_unique<Pmdg737>(&gateway, &status, variant,
                                                 std::move(dataGateway), std::move(tabletGateway));
        }

        void SeeEfbWeights(const double zfwKg, const double cargoLbs = 0.0)
        {
            tablet->weightEcho = PmdgWeightEcho{zfwKg * 2.20462262185 + cargoLbs, cargoLbs};
        }

        void SeedEnginesOff()
        {
            gateway.avars[kSimEng1Combustion] = 0.0;
            gateway.avars[kSimEng2Combustion] = 0.0;
        }
    };
}

class Pmdg737Test final : public QObject
{
    Q_OBJECT

private slots:
    static void nameAndCargoFlagFollowTheVariant();
    static void onTickPollsBothGateways();
    static void observationTickWritesNothing();
    static void drivingTickWritesWhatObservationHeldBack();
    static void groundPowerUnknownUntilData();
    static void groundPowerFollowsTheSingleAnnunciator();
    static void groundPowerIsPresentWhileTheAircraftIsUnpowered();
    static void poweredByMainBusOrRunningEngine();
    static void engineRunningConservativeUntilReceived();
    static void parkingBrakeReadsTheSdkBlockAndIgnoresTheSimVar();
    static void doorStatusUnknownUntilTheEfbAnswers();
    static void doorStatusOpenWhenTheEfbReportsADoorOpen();
    static void doorStatusUnknownWhileTheAirstairHasNoEfbReading();
    static void airstairReadsTheAnnunciatorOnceTheBusIsLive();
    static void airstairSaysNothingWhileTheBusIsDead();
    static void readyToDeboardAcceptsChocksInsteadOfBrake();
    static void mapsOnlyTheDoorsTheSevenThirtySevenHas();
    static void closingDoorsThatWereNeverOpenedCommandsNothing();
    static void serviceDoorFollowsTheCateringVehicle();
    static void openDoorIsLeftAloneWhenTheServiceArrives();
    static void entryDoorIsCommandedOnceWhileTheEfbStateIsUnknown();
    static void entryDoorRetriesWithCapWhileTheEfbStateDisagrees();
    static void entryDoorStopsAsSoonAsTheEfbStateAgrees();
    static void mainCargoDoorClosesWhenTheLoaderLeavesThePosition();
    static void mainCargoDoorClosesOnceTheLoaderStartsFinishing();
    static void doorInMotionIsNotCommanded();
    static void groundStateIsQueriedWhileTheAircraftRuns();
    static void mainCargoIsCommandedOnEdgeBecauseItCannotBeRead();
    static void paxVariantNeverTouchesMainCargo();
    static void mainDeckCargoDoorStuckOnlyWhenTheDoorRefuses();
    static void chocksReadFromTheLVarAndRetryWithCap();
    static void groundPowerRequestStopsWhenAvailable();
    static void setFuelSendsRoundedLbsOnce();
    static void cargoVariantSendsNoPassengers();
    static void progressiveWriterDoesNotUndoTheTrim();
    static void smartSwitchAtRestIsNotAPress();
    static void smartSwitchAnswersEveryPressOfTheSession();
    static void smartSwitchIgnoresTheLatchingIcSide();
    static void entryMethodIsLeftAloneUntilTheTabletReportsIt();
    static void ownStairsAreClearedByTakingTheEntryMethodToJetway();
    static void ownStairsAreReleasedByNameWhereNoJetwayIsOffered();
    static void vehicleStateInheritedFromBeforeACouatlRestartIsNotBelieved();
    static void keepsTheGsxDoorAutomationOffAndWaitsForTheEcho();
    static void writesTheGsxDoorAutomationAgainWhenGsxTurnsItBackOn();
    static void leavesTheGsxDoorAutomationAloneBeforeClientData();
    static void theDoorAutomationRuleRunsRightBeforeTheDoorRule();
    static void reachableOnlyOnceEverythingHasArrived();
    static void anOpenDoorIsNotToggledWhileTheTabletHasNotAnswered();
    static void aTabletThatAnswersLateStillSavesTheOpenDoor();
    static void aSilentTabletDoesNotHoldTheDoorBeyondTheBudget();
    static void theEndOfTheWaitIsLoggedOnce();
    static void withoutTheBridgeNothingIsAskedAndTheWaitEndsAtTheCeiling();
    static void aBridgeThatComesUpLateGetsTheWholeAnswerBudget();
    static void aTabletThatAnswersRightAfterTheBridgeComesUpIsNotWaitedFor();
    static void aTabletThatAnswersIsNotWaitedForAndAnswersAreNotLogged();
    static void aTabletSilentOnOneDoorDoesNotHoldIt();
    static void theMainCargoDoorWaitsForTheTabletToo();
    static void resumingNeitherClosesDoorsNorMakesTheHoldsCloseThem();
    static void observingAResumedAircraftOnlyAsksTheTabletForItsState();
    static void thePlanImportSeenCrossesTheRestart();
    static void aPlanImportedThroughTheRouteFileIsForgottenWhenTheNextFlightStartsLoading();
    static void aPlanImportedBeforeTheTurnaroundStartedSurvivesItsStart();
    static void aRestoredPlaceRequestWaitsForTheLiveChocksReading();
    static void aRestoredRemoveRequestWaitsForTheLiveChocksReading();
};

void Pmdg737Test::nameAndCargoFlagFollowTheVariant()
{
    QCOMPARE(QString(Pmdg737Fixture(Pmdg737Variant::Pax800).aircraft->GetName()),
             QString("PMDG 737-800"));
    QCOMPARE(QString(Pmdg737Fixture(Pmdg737Variant::Bcf800).aircraft->GetName()),
             QString("PMDG 737-800BCF"));
    QCOMPARE(QString(Pmdg737Fixture(Pmdg737Variant::Bdsf800).aircraft->GetName()),
             QString("PMDG 737-800BDSF"));
    QCOMPARE(QString(Pmdg737Fixture(Pmdg737Variant::Bbj2).aircraft->GetName()),
             QString("PMDG 737 BBJ2"));

    QVERIFY(!Pmdg737Fixture(Pmdg737Variant::Pax800).aircraft->IsCargoVariant());
    QVERIFY(Pmdg737Fixture(Pmdg737Variant::Bcf800).aircraft->IsCargoVariant());
    QVERIFY(Pmdg737Fixture(Pmdg737Variant::Bdsf800).aircraft->IsCargoVariant());
    QVERIFY(!Pmdg737Fixture(Pmdg737Variant::Bbj2).aircraft->IsCargoVariant());
}

void Pmdg737Test::onTickPollsBothGateways()
{
    Pmdg737Fixture fixture;

    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.data->pollCalls, 1);
    QCOMPARE(fixture.tablet->pollCalls, 1);
}

void Pmdg737Test::groundPowerUnknownUntilData()
{
    Pmdg737Fixture fixture;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Unknown));
}

void Pmdg737Test::groundPowerFollowsTheSingleAnnunciator()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Disconnected));

    fixture.data->groundPowerAvailable = true;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Connected));
}

void Pmdg737Test::groundPowerIsPresentWhileTheAircraftIsUnpowered()
{
    Pmdg737Fixture fixture;

    fixture.SeedEnginesOff();
    fixture.data->hasData = true;
    fixture.data->groundPowerAvailable = true;
    fixture.data->anyMainBusPowered = false;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Connected));
    QVERIFY(!fixture.aircraft->IsPowered());
}

void Pmdg737Test::poweredByMainBusOrRunningEngine()
{
    Pmdg737Fixture fixture;

    fixture.SeedEnginesOff();
    fixture.data->hasData = true;

    QVERIFY(!fixture.aircraft->IsPowered());

    fixture.data->anyMainBusPowered = true;
    QVERIFY(fixture.aircraft->IsPowered());

    fixture.data->anyMainBusPowered = false;
    fixture.gateway.avars[kSimEng1Combustion] = 1.0;
    QVERIFY(fixture.aircraft->IsPowered());
}

void Pmdg737Test::engineRunningConservativeUntilReceived()
{
    Pmdg737Fixture fixture;

    QVERIFY(fixture.aircraft->IsEngineRunning());
}

void Pmdg737Test::doorStatusUnknownUntilTheEfbAnswers()
{
    Pmdg737Fixture fixture;

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::Unknown);
}

void Pmdg737Test::doorStatusOpenWhenTheEfbReportsADoorOpen()
{
    Pmdg737Fixture fixture;

    for (const char* doorKey : kEfbDoorKeys)
    {
        fixture.tablet->doorOpen[doorKey] = false;
    }

    fixture.tablet->doorOpen[kFwdEntryKey] = true;

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::AnyOpen);
}

void Pmdg737Test::doorStatusUnknownWhileTheAirstairHasNoEfbReading()
{
    Pmdg737Fixture fixture;

    for (const char* doorKey : kEfbDoorKeys)
    {
        fixture.tablet->doorOpen[doorKey] = false;
    }

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::Unknown);
}

void Pmdg737Test::parkingBrakeReadsTheSdkBlockAndIgnoresTheSimVar()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    QVERIFY(!fixture.aircraft->IsParkingBrakeSet());

    fixture.gateway.avars[kSimParkingBrake] = 1.0;
    QVERIFY(!fixture.aircraft->IsParkingBrakeSet());

    fixture.gateway.avars[kSimParkingBrake] = 0.0;
    fixture.data->parkingBrakeOn = true;
    QVERIFY(fixture.aircraft->IsParkingBrakeSet());
}

void Pmdg737Test::readyToDeboardAcceptsChocksInsteadOfBrake()
{
    Pmdg737Fixture fixture;

    fixture.SeedEnginesOff();
    fixture.data->hasData = true;

    QVERIFY(!fixture.aircraft->IsReadyToDeboard());

    fixture.gateway.lvars[kChocksLVar] = 1.0;
    QVERIFY(fixture.aircraft->IsReadyToDeboard());

    fixture.data->beaconOn = true;
    QVERIFY(!fixture.aircraft->IsReadyToDeboard());
}

void Pmdg737Test::mapsOnlyTheDoorsTheSevenThirtySevenHas()
{
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::FwdPax), std::optional(Pmdg737Door::FwdEntry));
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::FwdCatering), std::optional(Pmdg737Door::FwdService));
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::AftPax), std::optional(Pmdg737Door::AftEntry));
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::AftCatering), std::optional(Pmdg737Door::AftService));
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::FwdCargo), std::optional(Pmdg737Door::FwdCargo));
    QCOMPARE(Pmdg737::DoorFor(GsxDoor::AftCargo), std::optional(Pmdg737Door::AftCargo));

    QCOMPARE(Pmdg737::DoorFor(GsxDoor::MidPax), std::nullopt);
}

namespace
{
    int EntryToggles(const Pmdg737Fixture& fixture)
    {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::FwdEntry));
    }

    void DockJetway(Pmdg737Fixture& fixture)
    {
        fixture.data->hasData = true;
        fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
        fixture.gateway.lvars[gsx::lvars::kJetway] = kJetwayDocked;
    }

    int PassengerEntryRequests(const Pmdg737Fixture& fixture)
    {
        return static_cast<int>(
            std::ranges::count(fixture.tablet->groundConnRequests, kPassengerEntryRequest));
    }

    void Tick(Pmdg737Fixture& fixture, const int times)
    {
        for (int i = 0; i < times; ++i)
        {
            TickAircraft(*fixture.aircraft, fixture.gateway);
        }
    }
}

void Pmdg737Test::closingDoorsThatWereNeverOpenedCommandsNothing()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    fixture.aircraft->CloseAllDoors();
    Tick(fixture, 10);

    QCOMPARE(static_cast<int>(fixture.data->toggledDoors.size()), 0);
}

void Pmdg737Test::serviceDoorFollowsTheCateringVehicle()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;

    fixture.aircraft->CloseAllDoors();
    Tick(fixture, 10);

    const auto serviceToggles = [&fixture] {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::FwdService));
    };

    QCOMPARE(serviceToggles(), 0);

    fixture.gateway.lvars[gsx::lvars::kCateringFrontState] = gsx::states::kCateringWaitingForDoor;
    Tick(fixture, 10);

    QCOMPARE(serviceToggles(), 1);

    fixture.gateway.lvars[gsx::lvars::kCateringFrontState] = 0.0;
    Tick(fixture, 10);

    QCOMPARE(serviceToggles(), 2);
}

void Pmdg737Test::openDoorIsLeftAloneWhenTheServiceArrives()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->doorOpen[kFwdEntryKey] = true;

    Tick(fixture, 20);

    QCOMPARE(EntryToggles(fixture), 0);
}

void Pmdg737Test::entryDoorIsCommandedOnceWhileTheEfbStateIsUnknown()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);

    Tick(fixture, 20);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::entryDoorRetriesWithCapWhileTheEfbStateDisagrees()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->doorOpen[kFwdEntryKey] = false;

    Tick(fixture, 60);

    QCOMPARE(EntryToggles(fixture), 3);
}

void Pmdg737Test::entryDoorStopsAsSoonAsTheEfbStateAgrees()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->doorOpen[kFwdEntryKey] = false;

    Tick(fixture, 1);
    fixture.tablet->doorOpen[kFwdEntryKey] = true;
    Tick(fixture, 20);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::mainCargoDoorClosesWhenTheLoaderLeavesThePosition()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;

    const auto mainToggles = [&fixture] {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::MainCargo));
    };

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderLoading;
    Tick(fixture, 10);

    QCOMPARE(mainToggles(), 1);

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderRetracting;
    Tick(fixture, 10);

    QCOMPARE(mainToggles(), 2);
}

void Pmdg737Test::mainCargoDoorClosesOnceTheLoaderStartsFinishing()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;

    const auto mainToggles = [&fixture] {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::MainCargo));
    };

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderLoading;
    Tick(fixture, 10);

    QCOMPARE(mainToggles(), 1);

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderFinishing;
    Tick(fixture, 10);

    QCOMPARE(mainToggles(), 2);
}

void Pmdg737Test::doorInMotionIsNotCommanded()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->moving.emplace(kFwdEntryKey);

    Tick(fixture, 30);

    QCOMPARE(EntryToggles(fixture), 0);

    fixture.tablet->moving.clear();
    fixture.tablet->doorOpen[kFwdEntryKey] = false;
    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::groundStateIsQueriedWhileTheAircraftRuns()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    Tick(fixture, 9);

    QCOMPARE(fixture.tablet->stateRequests, 3);
}

void Pmdg737Test::mainCargoIsCommandedOnEdgeBecauseItCannotBeRead()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.data->hasData = true;

    const auto toggles = [&fixture] {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::MainCargo));
    };
    const auto tick = [&fixture](const int times) {
        for (int i = 0; i < times; ++i)
        {
            TickAircraft(*fixture.aircraft, fixture.gateway);
        }
    };

    tick(10);
    const int afterSettling = toggles();

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;
    tick(1);
    QCOMPARE(toggles(), afterSettling + 1);

    tick(10);
    QCOMPARE(toggles(), afterSettling + 1);

    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = 0.0;
    tick(1);
    QCOMPARE(toggles(), afterSettling + 2);

    tick(10);
    QCOMPARE(toggles(), afterSettling + 2);
}

void Pmdg737Test::paxVariantNeverTouchesMainCargo()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Pax800);

    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    for (int tick = 0; tick < 10; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QVERIFY(std::ranges::find(fixture.data->toggledDoors, Pmdg737Door::MainCargo)
        == fixture.data->toggledDoors.end());
}

void Pmdg737Test::mainDeckCargoDoorStuckOnlyWhenTheDoorRefuses()
{
    Pmdg737Fixture cargo(Pmdg737Variant::Bcf800);

    cargo.data->hasData = true;
    cargo.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    cargo.tablet->doorOpen[kMainCargoKey] = false;
    cargo.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    Tick(cargo, 1);

    QVERIFY(!cargo.aircraft->IsMainDeckCargoDoorStuck());

    Tick(cargo, 12);

    QVERIFY(cargo.aircraft->IsMainDeckCargoDoorStuck());

    cargo.tablet->doorOpen[kMainCargoKey] = true;
    Tick(cargo, 1);

    QVERIFY(!cargo.aircraft->IsMainDeckCargoDoorStuck());

    Pmdg737Fixture pax(Pmdg737Variant::Pax800);

    pax.data->hasData = true;
    pax.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    pax.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    Tick(pax, 30);

    QVERIFY(!pax.aircraft->IsMainDeckCargoDoorStuck());
}

void Pmdg737Test::chocksReadFromTheLVarAndRetryWithCap()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    QVERIFY(fixture.aircraft->SetChocks(true));
    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("wheel_chocks"));

    for (int tick = 0; tick < 9; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));

    TickAircraft(*fixture.aircraft, fixture.gateway);
    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));

    fixture.gateway.lvars[kChocksLVar] = 1.0;
    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));
}

void Pmdg737Test::groundPowerRequestStopsWhenAvailable()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->SetGroundPower(true);
    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("ground_power"));

    fixture.data->groundPowerAvailable = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
}

void Pmdg737Test::setFuelSendsRoundedLbsOnce()
{
    Pmdg737Fixture fixture;

    fixture.aircraft->SetCurrentFuelKg(1000.0);
    fixture.aircraft->SetCurrentFuelKg(1000.0);

    QCOMPARE(fixture.tablet->fuelSends.size(), static_cast<std::size_t>(1));
    QCOMPARE(fixture.tablet->fuelSends[0], 2205);
}

void Pmdg737Test::cargoVariantSendsNoPassengers()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.gateway.avars["EMPTY WEIGHT"] = 40000.0;
    fixture.gateway.avars["TOTAL WEIGHT"] = 40000.0;
    fixture.status.plannedZfwKg = 60000.0;
    fixture.status.plannedPassengers = 100;
    fixture.SeeEfbWeights(40000.0);

    fixture.aircraft->SetCurrentZfwKg(50000.0);

    QVERIFY(fixture.tablet->paxSends.empty());
    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));
}

void Pmdg737Test::progressiveWriterDoesNotUndoTheTrim()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.data->hasData = true;
    fixture.gateway.avars["EMPTY WEIGHT"] = 40000.0;
    fixture.gateway.avars["TOTAL WEIGHT"] = 61200.0;
    fixture.gateway.avars["FUEL TOTAL QUANTITY WEIGHT"] = 0.0;
    fixture.status.plannedZfwKg = 60000.0;
    fixture.SeeEfbWeights(40000.0);

    fixture.aircraft->SetCurrentZfwKg(60000.0);

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));

    for (int tick = 0; tick < 5; ++tick)
    {
        fixture.aircraft->SetCurrentZfwKg(60000.0);
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));

    const int trimmedCargo = fixture.tablet->cargoSends[1];

    for (int tick = 0; tick < 3; ++tick)
    {
        fixture.aircraft->SetCurrentZfwKg(60000.0);
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));
    QCOMPARE(fixture.tablet->cargoSends.back(), trimmedCargo);
}

void Pmdg737Test::smartSwitchAtRestIsNotAPress()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;
    TickAircraft(*fixture.aircraft, fixture.gateway);

    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg737Test::smartSwitchAnswersEveryPressOfTheSession()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;
    TickAircraft(*fixture.aircraft, fixture.gateway);

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchDown, kSmartSwitchNeutral, true};
    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchNeutral, kSmartSwitchNeutral, true};
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchDown, kSmartSwitchNeutral, true};
    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg737Test::smartSwitchIgnoresTheLatchingIcSide()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;
    TickAircraft(*fixture.aircraft, fixture.gateway);

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchNeutral, kSmartSwitchUp, true};
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchUp, kSmartSwitchUp, true};
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchDown, kSmartSwitchUp, true};
    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg737Test::entryMethodIsLeftAloneUntilTheTabletReportsIt()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->ClearOwnGroundEquipment();

    Tick(fixture, 20);

    QVERIFY(fixture.tablet->groundConnRequests.empty());
}

void Pmdg737Test::ownStairsAreClearedByTakingTheEntryMethodToJetway()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.tablet->jetwayInhibited = false;
    fixture.tablet->passengerEntryJetway = false;
    fixture.aircraft->ClearOwnGroundEquipment();

    Tick(fixture, 1);

    QCOMPARE(PassengerEntryRequests(fixture), 1);

    fixture.tablet->passengerEntryJetway = true;
    Tick(fixture, 20);

    QCOMPARE(PassengerEntryRequests(fixture), 1);
}

void Pmdg737Test::ownStairsAreReleasedByNameWhereNoJetwayIsOffered()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);

    fixture.data->hasData = true;
    fixture.tablet->jetwayInhibited = true;
    fixture.tablet->ownStairsDeployed = true;
    fixture.aircraft->ClearOwnGroundEquipment();

    Tick(fixture, 1);

    QCOMPARE(PassengerEntryRequests(fixture), 0);
    QCOMPARE(static_cast<int>(fixture.tablet->groundVehicleRequests.size()), 1);
    QCOMPARE(QString::fromStdString(fixture.tablet->groundVehicleRequests[0]), QString("stairs_1l"));

    fixture.tablet->ownStairsDeployed = false;
    Tick(fixture, 20);

    QCOMPARE(static_cast<int>(fixture.tablet->groundVehicleRequests.size()), 1);
}

void Pmdg737Test::vehicleStateInheritedFromBeforeACouatlRestartIsNotBelieved()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;

    fixture.aircraft->CloseAllDoors();
    Tick(fixture, 6);

    const auto entryToggles = [&fixture] {
        return static_cast<int>(std::ranges::count(fixture.data->toggledDoors, Pmdg737Door::FwdEntry));
    };

    fixture.gateway.lvars[gsx::lvars::kCateringFrontState] = gsx::states::kCateringWaitingForDoor;
    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = gsx::states::kStairsFinalPosition;
    Tick(fixture, 2);

    QCOMPARE(entryToggles(), 1);

    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 0.0;
    Tick(fixture, 2);
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    Tick(fixture, 2);

    QCOMPARE(entryToggles(), 2);

    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = 1.0;
    Tick(fixture, 2);
    fixture.gateway.lvars[gsx::lvars::kPassengerStairsFrontState] = gsx::states::kStairsFinalPosition;
    Tick(fixture, 2);

    QCOMPARE(entryToggles(), 3);
}

void Pmdg737Test::observationTickWritesNothing()
{
    Pmdg737Fixture f;
    f.data->hasData = true;
    f.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;

    const int lvarWrites = f.gateway.setLVarCalls;
    const int avarWrites = f.gateway.setAVarCalls;

    f.aircraft->Observe();

    QCOMPARE(f.gateway.setLVarCalls, lvarWrites);
    QCOMPARE(f.gateway.setAVarCalls, avarWrites);
    QVERIFY(f.data->toggledDoors.empty());
    QVERIFY(f.tablet->groundConnRequests.empty());
    QVERIFY(f.tablet->fuelSends.empty());
    QVERIFY(f.tablet->paxSends.empty());
    QVERIFY(f.tablet->cargoSends.empty());
}

void Pmdg737Test::drivingTickWritesWhatObservationHeldBack()
{
    Pmdg737Fixture f;
    f.data->hasData = true;

    f.aircraft->Observe();
    const int afterObservation = f.gateway.setLVarCalls;

    TickAircraft(*f.aircraft, f.gateway);

    QVERIFY(f.gateway.setLVarCalls > afterObservation);
}

void Pmdg737Test::airstairReadsTheAnnunciatorOnceTheBusIsLive()
{
    Pmdg737Fixture f;

    f.data->hasData = true;
    f.data->anyMainBusPowered = true;
    for (const char* key : kEfbDoorKeys)
    {
        f.tablet->doorOpen[key] = false;
    }

    QVERIFY(f.aircraft->GetDoorStatus() == DoorStatus::AllClosed);

    f.data->airstairAnnunciator = true;

    QVERIFY(f.aircraft->GetDoorStatus() == DoorStatus::AnyOpen);
}

void Pmdg737Test::airstairSaysNothingWhileTheBusIsDead()
{
    Pmdg737Fixture f;

    f.data->hasData = true;
    f.data->anyMainBusPowered = false;
    for (const char* key : kEfbDoorKeys)
    {
        f.tablet->doorOpen[key] = false;
    }

    QVERIFY(f.aircraft->GetDoorStatus() == DoorStatus::Unknown);
}

void Pmdg737Test::keepsTheGsxDoorAutomationOffAndWaitsForTheEcho()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    for (int tick = 0; tick < kTicksWithoutEcho + 1; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);

        QCOMPARE(fixture.gateway.WriteCount(gsx::lvars::kAutomationDoors), 1);

        fixture.gateway.lvars[gsx::lvars::kAutomationDoors] = 1.0;
    }

    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.gateway.WriteCount(gsx::lvars::kAutomationDoors), 2);
    QCOMPARE(fixture.gateway.Written(gsx::lvars::kAutomationDoors), 0.0);
}

void Pmdg737Test::writesTheGsxDoorAutomationAgainWhenGsxTurnsItBackOn()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    TickAircraft(*fixture.aircraft, fixture.gateway);
    TickAircraft(*fixture.aircraft, fixture.gateway);
    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.gateway.WriteCount(gsx::lvars::kAutomationDoors), 1);

    fixture.gateway.lvars[gsx::lvars::kAutomationDoors] = 1.0;
    TickAircraft(*fixture.aircraft, fixture.gateway);

    QCOMPARE(fixture.gateway.WriteCount(gsx::lvars::kAutomationDoors), 2);
}

void Pmdg737Test::leavesTheGsxDoorAutomationAloneBeforeClientData()
{
    Pmdg737Fixture fixture;

    for (int tick = 0; tick < kTicksWithoutEcho + 2; ++tick)
    {
        TickAircraft(*fixture.aircraft, fixture.gateway);
    }

    QCOMPARE(fixture.gateway.WriteCount(gsx::lvars::kAutomationDoors), 0);
}

void Pmdg737Test::theDoorAutomationRuleRunsRightBeforeTheDoorRule()
{
    Pmdg737Fixture fixture;

    std::vector<std::string> names;
    for (AircraftRule* const rule : fixture.aircraft->Rules())
    {
        names.emplace_back(rule->Name());
    }

    const auto automation = std::ranges::find(names, std::string("pmdg-keep-gsx-door-automation-off"));

    QVERIFY(automation != names.end());
    QVERIFY(std::next(automation) != names.end());
    QVERIFY(*std::next(automation) == "pmdg-doors-follow-gsx");
}

namespace
{
    constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";
    constexpr auto kSimFuelWeight = "FUEL TOTAL QUANTITY WEIGHT";
    constexpr auto kNoReadingMessage = "no door reading";
    constexpr auto kNoBridgeMessage = "bridge never came up";
    constexpr int kAnswerBudgetTicks = 6;
    constexpr int kBridgeCeilingTicks = 4;
    constexpr double kEmptyKg = 41000.0;

    void LetEverythingArrive(Pmdg737Fixture& fixture)
    {
        fixture.data->hasData = true;
        fixture.gateway.avars[kSimEmptyWeight] = kEmptyKg;
        fixture.gateway.avars[kSimFuelWeight] = 500.0;
        fixture.SeeEfbWeights(kEmptyKg);
    }

    TurnaroundFacts ResumedFacts()
    {
        TurnaroundFacts facts;
        facts.phase = TurnaroundPhase::Loading;
        facts.loadingStarted = true;
        facts.emptyZfwKg = kEmptyKg;
        facts.plannedZfwKg = kEmptyKg + 15000.0;
        facts.plannedPassengers = 150;

        return facts;
    }

    std::vector<QString>& CapturedMessages()
    {
        static std::vector<QString> messages;

        return messages;
    }

    void CaptureMessage(QtMsgType, const QMessageLogContext&, const QString& message)
    {
        CapturedMessages().push_back(message);
    }

    class MessageCapture
    {
    public:
        MessageCapture()
        {
            CapturedMessages().clear();
            previous_ = qInstallMessageHandler(&CaptureMessage);
        }

        ~MessageCapture()
        {
            qInstallMessageHandler(previous_);
        }

        MessageCapture(const MessageCapture&) = delete;
        MessageCapture& operator=(const MessageCapture&) = delete;

        [[nodiscard]] static int Count(const char* fragment)
        {
            return static_cast<int>(std::ranges::count_if(CapturedMessages(), [fragment](const QString& message)
            {
                return message.contains(QLatin1String(fragment));
            }));
        }

    private:
        QtMessageHandler previous_ = nullptr;
    };
}

void Pmdg737Test::reachableOnlyOnceEverythingHasArrived()
{
    Pmdg737Fixture fixture;

    QVERIFY(!fixture.aircraft->IsReachable());

    LetEverythingArrive(fixture);

    QVERIFY(fixture.aircraft->IsReachable());

    fixture.data->hasData = false;
    QVERIFY(!fixture.aircraft->IsReachable());
    fixture.data->hasData = true;

    fixture.tablet->available = false;
    QVERIFY(!fixture.aircraft->IsReachable());
    fixture.tablet->available = true;

    fixture.tablet->weightEcho.reset();
    QVERIFY(!fixture.aircraft->IsReachable());
    fixture.SeeEfbWeights(kEmptyKg);

    fixture.gateway.avars.erase(kSimEmptyWeight);
    QVERIFY(!fixture.aircraft->IsReachable());
    fixture.gateway.avars[kSimEmptyWeight] = kEmptyKg;

    fixture.gateway.avars.erase(kSimFuelWeight);
    QVERIFY(!fixture.aircraft->IsReachable());
    fixture.gateway.avars[kSimFuelWeight] = 500.0;

    QVERIFY(fixture.aircraft->IsReachable());
}

void Pmdg737Test::anOpenDoorIsNotToggledWhileTheTabletHasNotAnswered()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);

    Tick(fixture, 2);

    QCOMPARE(EntryToggles(fixture), 0);

    fixture.tablet->doorOpen[kFwdEntryKey] = true;
    Tick(fixture, 40);

    QCOMPARE(EntryToggles(fixture), 0);
}

void Pmdg737Test::aTabletThatAnswersLateStillSavesTheOpenDoor()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);

    Tick(fixture, kAnswerBudgetTicks - 1);

    QCOMPARE(fixture.tablet->stateRequests, 2);
    QCOMPARE(EntryToggles(fixture), 0);

    fixture.tablet->doorOpen[kFwdEntryKey] = true;
    Tick(fixture, 40);

    QCOMPARE(EntryToggles(fixture), 0);
}

void Pmdg737Test::aSilentTabletDoesNotHoldTheDoorBeyondTheBudget()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);

    Tick(fixture, kAnswerBudgetTicks - 1);

    QCOMPARE(EntryToggles(fixture), 0);

    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);

    Tick(fixture, 40);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::theEndOfTheWaitIsLoggedOnce()
{
    const MessageCapture capture;
    Pmdg737Fixture fixture;
    DockJetway(fixture);

    Tick(fixture, kAnswerBudgetTicks - 1);

    QCOMPARE(MessageCapture::Count(kNoReadingMessage), 0);

    Tick(fixture, 40);

    QCOMPARE(MessageCapture::Count(kNoReadingMessage), 1);
}

void Pmdg737Test::withoutTheBridgeNothingIsAskedAndTheWaitEndsAtTheCeiling()
{
    const MessageCapture capture;
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->available = false;

    Tick(fixture, kBridgeCeilingTicks - 1);

    QCOMPARE(fixture.tablet->stateRequests, 0);
    QCOMPARE(EntryToggles(fixture), 0);
    QCOMPARE(MessageCapture::Count(kNoBridgeMessage), 0);

    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);

    Tick(fixture, 40);

    QCOMPARE(EntryToggles(fixture), 1);
    QCOMPARE(MessageCapture::Count(kNoBridgeMessage), 1);
    QCOMPARE(MessageCapture::Count(kNoReadingMessage), 0);
}

void Pmdg737Test::aBridgeThatComesUpLateGetsTheWholeAnswerBudget()
{
    const MessageCapture capture;
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->available = false;

    Tick(fixture, kBridgeCeilingTicks / 2);

    QCOMPARE(EntryToggles(fixture), 0);

    fixture.tablet->available = true;
    Tick(fixture, kAnswerBudgetTicks - 1);

    QCOMPARE(fixture.tablet->stateRequests, 2);
    QCOMPARE(EntryToggles(fixture), 0);

    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);
    QCOMPARE(MessageCapture::Count(kNoBridgeMessage), 0);
}

void Pmdg737Test::aTabletThatAnswersRightAfterTheBridgeComesUpIsNotWaitedFor()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->available = false;

    Tick(fixture, kBridgeCeilingTicks - 1);

    QCOMPARE(EntryToggles(fixture), 0);

    fixture.tablet->available = true;
    fixture.tablet->doorOpen[kFwdEntryKey] = false;
    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::aTabletThatAnswersIsNotWaitedForAndAnswersAreNotLogged()
{
    const MessageCapture capture;
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->doorOpen[kFwdEntryKey] = false;

    Tick(fixture, 40);

    QCOMPARE(EntryToggles(fixture), 3);
    QCOMPARE(MessageCapture::Count(kNoReadingMessage), 0);
}

void Pmdg737Test::aTabletSilentOnOneDoorDoesNotHoldIt()
{
    Pmdg737Fixture fixture;
    DockJetway(fixture);
    fixture.tablet->doorOpen["fwd_cargo"] = false;

    Tick(fixture, 1);

    QCOMPARE(EntryToggles(fixture), 1);
}

void Pmdg737Test::theMainCargoDoorWaitsForTheTabletToo()
{
    Pmdg737Fixture fixture(Pmdg737Variant::Bcf800);
    fixture.data->hasData = true;
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    fixture.gateway.lvars[gsx::lvars::kBaggageLoaderMainState] = gsx::states::kLoaderLoading;

    Tick(fixture, 2);

    QVERIFY(fixture.data->toggledDoors.empty());

    fixture.tablet->doorOpen[kMainCargoKey] = true;
    Tick(fixture, 40);

    QVERIFY(fixture.data->toggledDoors.empty());
}

void Pmdg737Test::resumingNeitherClosesDoorsNorMakesTheHoldsCloseThem()
{
    Pmdg737Fixture fixture;
    LetEverythingArrive(fixture);
    fixture.gateway.lvars[gsx::lvars::kCouatlStarted] = 1.0;
    for (const char* key : kEfbDoorKeys)
    {
        fixture.tablet->doorOpen[key] = true;
    }

    fixture.aircraft->OnTurnaroundResumed(ResumedFacts(), MemoryBag{});
    fixture.aircraft->HoldDoorsClosed(true);
    fixture.aircraft->HoldPassengerDoorsClosed(true);
    Tick(fixture, 30);

    QVERIFY(fixture.data->toggledDoors.empty());
}

void Pmdg737Test::observingAResumedAircraftOnlyAsksTheTabletForItsState()
{
    Pmdg737Fixture fixture;
    LetEverythingArrive(fixture);
    DockJetway(fixture);
    fixture.aircraft->OnTurnaroundResumed(ResumedFacts(), MemoryBag{});
    const int lvarWrites = fixture.gateway.setLVarCalls;
    const int avarWrites = fixture.gateway.setAVarCalls;

    for (int tick = 0; tick < 30; ++tick)
    {
        fixture.gateway.MarkTick();
        fixture.aircraft->Observe();
    }

    QVERIFY(fixture.data->toggledDoors.empty());
    QCOMPARE(fixture.gateway.setLVarCalls, lvarWrites);
    QCOMPARE(fixture.gateway.setAVarCalls, avarWrites);
    QVERIFY(fixture.tablet->groundConnRequests.empty());
    QVERIFY(fixture.tablet->groundVehicleRequests.empty());
    QVERIFY(fixture.tablet->fuelSends.empty());
    QVERIFY(fixture.tablet->paxSends.empty());
    QVERIFY(fixture.tablet->cargoSends.empty());
    QCOMPARE(fixture.tablet->stateRequests, 10);
}

void Pmdg737Test::thePlanImportSeenCrossesTheRestart()
{
    Pmdg737Fixture dead;
    dead.tablet->efbPlanImported = true;
    const MemoryBag memory = dead.aircraft->TurnaroundMemory();

    Pmdg737Fixture born;
    born.status.flightPlanStatus = FlightPlanStatus::Ready;

    QVERIFY(!born.aircraft->IsFlightPlanLoaded());

    born.aircraft->OnTurnaroundResumed(ResumedFacts(), memory);

    QVERIFY(born.aircraft->IsFlightPlanLoaded());
}

namespace
{
    constexpr long long kSecondsBeforeNow = 100;

    long long NowEpoch()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    class RouteDirectory
    {
    public:
        explicit RouteDirectory(const char* aircraftName)
            : previousAppData_(qgetenv("APPDATA"))
        {
            qputenv("APPDATA", appData_.path().toUtf8());
            directory_ = *PmdgRouteFile::DirectoryFor(aircraftName);
            std::filesystem::create_directories(directory_);
        }

        ~RouteDirectory()
        {
            qputenv("APPDATA", previousAppData_);
        }

        RouteDirectory(const RouteDirectory&) = delete;
        RouteDirectory& operator=(const RouteDirectory&) = delete;

        void Import(const long long secondsAgo) const
        {
            const std::filesystem::path file = directory_ / "SBFZSBTE.rte";
            std::ofstream(file) << "Generated by SimBrief\n";
            const auto stamp = std::chrono::system_clock::time_point(std::chrono::seconds(NowEpoch() - secondsAgo));
            std::filesystem::last_write_time(file, std::chrono::clock_cast<std::chrono::file_clock>(stamp));
        }

    private:
        QTemporaryDir appData_;
        QByteArray previousAppData_;
        std::filesystem::path directory_;
    };

    void ExpectAPlan(Pmdg737Fixture& fixture)
    {
        fixture.status.flightPlanStatus = FlightPlanStatus::Ready;
        fixture.status.plannedOrigin = "SBFZ";
        fixture.status.plannedDestination = "SBTE";
        fixture.status.planGeneratedEpoch = 1000;
    }

    MemoryBag MemoryWithChocksPending(const bool placed)
    {
        Pmdg737Fixture dead;
        dead.aircraft->SetChocks(placed);

        return dead.aircraft->TurnaroundMemory();
    }

    void ResumeBeforeTheReadingsArrive(Pmdg737Fixture& born, const MemoryBag& memory)
    {
        LetEverythingArrive(born);
        born.gateway.arrivesATickAfterItIsAsked = true;
        born.aircraft->OnTurnaroundResumed(ResumedFacts(), memory);
    }
}

void Pmdg737Test::aPlanImportedThroughTheRouteFileIsForgottenWhenTheNextFlightStartsLoading()
{
    const RouteDirectory routes(Pmdg737::kNamePax800);
    Pmdg737Fixture fixture;
    ExpectAPlan(fixture);

    routes.Import(kSecondsBeforeNow);
    fixture.aircraft->Observe();

    QVERIFY(!fixture.aircraft->IsFlightPlanLoaded());

    routes.Import(kSecondsBeforeNow / 2);
    fixture.aircraft->Observe();

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());

    fixture.aircraft->OnLoadingStarted();

    QVERIFY(!fixture.aircraft->IsFlightPlanLoaded());

    fixture.aircraft->Observe();
    fixture.aircraft->OnTurnaroundStarted();
    fixture.aircraft->Observe();

    QVERIFY(!fixture.aircraft->IsFlightPlanLoaded());

    routes.Import(kSecondsBeforeNow / 10);
    fixture.aircraft->Observe();

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());
}

void Pmdg737Test::aPlanImportedBeforeTheTurnaroundStartedSurvivesItsStart()
{
    const RouteDirectory routes(Pmdg737::kNamePax800);
    Pmdg737Fixture fixture;
    ExpectAPlan(fixture);

    routes.Import(kSecondsBeforeNow);
    fixture.aircraft->Observe();
    routes.Import(kSecondsBeforeNow / 2);
    fixture.aircraft->Observe();

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());

    fixture.aircraft->OnTurnaroundStarted();
    fixture.aircraft->Observe();

    QVERIFY(fixture.aircraft->IsFlightPlanLoaded());
}

void Pmdg737Test::aRestoredPlaceRequestWaitsForTheLiveChocksReading()
{
    const MemoryBag memory = MemoryWithChocksPending(true);
    Pmdg737Fixture born;
    ResumeBeforeTheReadingsArrive(born, memory);
    born.gateway.lvars[kChocksLVar] = 1.0;

    Tick(born, 15);

    QVERIFY(born.tablet->groundConnRequests.empty());

    born.gateway.DeliverWhatWasAsked();
    Tick(born, 15);

    QVERIFY(born.tablet->groundConnRequests.empty());
}

void Pmdg737Test::aRestoredRemoveRequestWaitsForTheLiveChocksReading()
{
    const MemoryBag memory = MemoryWithChocksPending(false);
    Pmdg737Fixture born;
    ResumeBeforeTheReadingsArrive(born, memory);
    born.gateway.lvars[kChocksLVar] = 1.0;

    Tick(born, 15);

    QVERIFY(born.tablet->groundConnRequests.empty());

    born.gateway.DeliverWhatWasAsked();
    Tick(born, 1);

    QCOMPARE(born.tablet->groundConnRequests, std::vector<std::string>{"wheel_chocks"});
}

QTEST_APPLESS_MAIN(Pmdg737Test)

#include "tst_pmdg737.moc"
