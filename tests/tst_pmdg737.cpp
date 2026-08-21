#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <memory>
#include "doubles/FakePmdg737DataGateway.h"
#include "doubles/FakePmdgTabletGateway.h"
#include "doubles/FakeVariableGateway.h"
#include "../src/domain/model/AutomationStatus.h"
#include "../src/infrastructure/aircraft/Pmdg737.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

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
    static void groundPowerUnknownUntilData();
    static void groundPowerFollowsTheSingleAnnunciator();
    static void poweredByMainBusOrRunningEngine();
    static void engineRunningConservativeUntilReceived();
    static void parkingBrakeFallsBackToTheSimVar();
    static void doorStatusUnknownUntilTheEfbAnswers();
    static void doorStatusOpenWhenTheEfbReportsADoorOpen();
    static void doorStatusUnknownWhileTheAirstairHasNoEfbReading();
    static void readyToDeboardAcceptsChocksInsteadOfBrake();
    static void mapsOnlyTheDoorsTheSevenThirtySevenHas();
    static void closingDoorsThatWereNeverOpenedCommandsNothing();
    static void serviceDoorFollowsTheCateringVehicle();
    static void openDoorIsLeftAloneWhenTheServiceArrives();
    static void entryDoorIsCommandedOnceWhileTheEfbStateIsUnknown();
    static void entryDoorRetriesWithCapWhileTheEfbStateDisagrees();
    static void entryDoorStopsAsSoonAsTheEfbStateAgrees();
    static void mainCargoDoorClosesWhenTheLoaderLeavesThePosition();
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
    static void entryMethodIsLeftAloneUntilTheTabletReportsIt();
    static void ownStairsAreClearedByTakingTheEntryMethodToJetway();
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
    const Pmdg737Fixture fixture;

    fixture.aircraft->OnTick();

    QCOMPARE(fixture.data->pollCalls, 1);
    QCOMPARE(fixture.tablet->pollCalls, 1);
}

void Pmdg737Test::groundPowerUnknownUntilData()
{
    const Pmdg737Fixture fixture;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Unknown));
}

void Pmdg737Test::groundPowerFollowsTheSingleAnnunciator()
{
    const Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Disconnected));

    fixture.data->groundPowerAvailable = true;

    QCOMPARE(fixture.aircraft->GetGroundPowerStatus(), std::optional(GroundPowerStatus::Connected));
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
    const Pmdg737Fixture fixture;

    QVERIFY(fixture.aircraft->IsEngineRunning());
}

void Pmdg737Test::doorStatusUnknownUntilTheEfbAnswers()
{
    const Pmdg737Fixture fixture;

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::Unknown);
}

void Pmdg737Test::doorStatusOpenWhenTheEfbReportsADoorOpen()
{
    const Pmdg737Fixture fixture;

    for (const char* doorKey : kEfbDoorKeys)
    {
        fixture.tablet->doorOpen[doorKey] = false;
    }

    fixture.tablet->doorOpen[kFwdEntryKey] = true;

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::AnyOpen);
}

void Pmdg737Test::doorStatusUnknownWhileTheAirstairHasNoEfbReading()
{
    const Pmdg737Fixture fixture;

    for (const char* doorKey : kEfbDoorKeys)
    {
        fixture.tablet->doorOpen[doorKey] = false;
    }

    QVERIFY(fixture.aircraft->GetDoorStatus() == DoorStatus::Unknown);
}

void Pmdg737Test::parkingBrakeFallsBackToTheSimVar()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;

    QVERIFY(!fixture.aircraft->IsParkingBrakeSet());

    fixture.gateway.avars[kSimParkingBrake] = 1.0;
    QVERIFY(fixture.aircraft->IsParkingBrakeSet());

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

    void Tick(const Pmdg737Fixture& fixture, const int times)
    {
        for (int i = 0; i < times; ++i)
        {
            fixture.aircraft->OnTick();
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
            fixture.aircraft->OnTick();
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
        fixture.aircraft->OnTick();
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
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("wheel_chocks"));

    for (int tick = 0; tick < 4; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));

    fixture.aircraft->OnTick();
    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));

    fixture.gateway.lvars[kChocksLVar] = 1.0;
    for (int tick = 0; tick < 20; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(2));
}

void Pmdg737Test::groundPowerRequestStopsWhenAvailable()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.aircraft->SetGroundPower(true);
    fixture.aircraft->OnTick();

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(fixture.tablet->groundConnRequests[0]), QString("ground_power"));

    fixture.data->groundPowerAvailable = true;
    for (int tick = 0; tick < 20; ++tick)
    {
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->groundConnRequests.size(), static_cast<std::size_t>(1));
}

void Pmdg737Test::setFuelSendsRoundedLbsOnce()
{
    const Pmdg737Fixture fixture;

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

    fixture.aircraft->SetCurrentZfwKg(60000.0);

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(1));

    for (int tick = 0; tick < 5; ++tick)
    {
        fixture.aircraft->SetCurrentZfwKg(60000.0);
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));

    const int trimmedCargo = fixture.tablet->cargoSends[1];

    for (int tick = 0; tick < 3; ++tick)
    {
        fixture.aircraft->SetCurrentZfwKg(60000.0);
        fixture.aircraft->OnTick();
    }

    QCOMPARE(fixture.tablet->cargoSends.size(), static_cast<std::size_t>(2));
    QCOMPARE(fixture.tablet->cargoSends.back(), trimmedCargo);
}

void Pmdg737Test::smartSwitchAtRestIsNotAPress()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;
    fixture.aircraft->OnTick();

    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());
}

void Pmdg737Test::smartSwitchAnswersEveryPressOfTheSession()
{
    Pmdg737Fixture fixture;

    fixture.data->hasData = true;
    fixture.gateway.lvars[kSmartSwitchLVar] = kSmartSwitchNeutral;
    fixture.aircraft->OnTick();

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchDown, kSmartSwitchNeutral, true};
    QVERIFY(fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchNeutral, kSmartSwitchNeutral, true};
    QVERIFY(!fixture.aircraft->ConsumeSmartSwitch());

    fixture.gateway.lvarSpans[kSmartSwitchLVar] = {kSmartSwitchNeutral, kSmartSwitchUp, true};
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
    fixture.tablet->passengerEntryJetway = false;
    fixture.aircraft->ClearOwnGroundEquipment();

    Tick(fixture, 1);

    QCOMPARE(PassengerEntryRequests(fixture), 1);

    fixture.tablet->passengerEntryJetway = true;
    Tick(fixture, 20);

    QCOMPARE(PassengerEntryRequests(fixture), 1);
}

QTEST_APPLESS_MAIN(Pmdg737Test)

#include "tst_pmdg737.moc"
